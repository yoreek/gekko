#include "config/MemoryConfigStorage.h"
#include "devices/display/DisplayDeviceBase.h"
#include "devices/display/DisplayLayoutRenderer.h"
#include "devices/dummy/DummyDevice.h"
#include "devices/registry/DeviceRegistry.h"
#include "devices/registry/DeviceRegistryStore.h"
#include "metrics/MetricValueResolver.h"
#include "wifi/WifiDriver.h"

#include <cstdio>
#include <string>
#include <unity.h>
#include <vector>

using namespace ewfm;

namespace {

class FakeWifiDriver final : public IWifiDriver {
public:
    bool begin() override {
        return true;
    }
    bool beginStation(const WiFiCredentials& credentials) override {
        (void)credentials;
        return true;
    }
    void disconnect() override {}
    void clearStationCredentials() override {}
    bool startSetupAp(const std::string& ssid, const std::string& password) override {
        (void)ssid;
        (void)password;
        return true;
    }
    void stopSetupAp() override {}
    WifiDriverStatus status() const override {
        return WifiDriverStatus::Connected;
    }
    bool networkStackReady() const override {
        return true;
    }
    bool stationReady() const override {
        return true;
    }
    bool setupApReady() const override {
        return false;
    }
    std::string stationIp() const override {
        return "192.168.1.50";
    }
    std::string setupApIp() const override {
        return "";
    }
    bool startScan() override {
        return true;
    }
    bool scanComplete(std::vector<WifiNetwork>& networks, size_t maxResults) override {
        (void)networks;
        (void)maxResults;
        return false;
    }
    std::string macSuffix() const override {
        return "abcd";
    }
};

DeviceId createDummy(DeviceRegistry& registry, const char* name, uint32_t now) {
    DummyDeviceConfigV1 config{};
    config.enabled = 1;
    std::snprintf(config.name, sizeof(config.name), "%s", name);
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    TEST_ASSERT_TRUE(encodeDummyDeviceConfig(config, buffer, dummyDeviceConfigSize(config)));

    DeviceCreateRequest request{};
    request.typeId = DummyDevice::descriptor().typeId;
    request.name = name;
    request.configVersion = DummyDevice::descriptor().currentConfigVersion;
    TEST_ASSERT_TRUE(request.configBlob.assign(buffer, dummyDeviceConfigSize(config)));
    const DeviceCreateResult result = registry.create(request, now);
    TEST_ASSERT_TRUE(result.ok());
    registry.tickFastLoop(now + 1U);
    return result.deviceId;
}

class FakeDisplaySurface final : public IDisplayRenderSurface {
public:
    void clear() override {
        ops.emplace_back("clear");
    }

    void drawText(const DisplayLayoutWidgetV1& widget, const DisplayTextEvaluationResult& text) override {
        char buffer[128]{};
        std::snprintf(buffer, sizeof(buffer), "text:%s:%s", widget.id, text.text);
        ops.emplace_back(buffer);
    }

    void drawRect(const DisplayLayoutWidgetV1& widget) override {
        char buffer[64]{};
        std::snprintf(buffer, sizeof(buffer), "rect:%s", widget.id);
        ops.emplace_back(buffer);
    }

    void drawLine(const DisplayLayoutWidgetV1& widget) override {
        char buffer[64]{};
        std::snprintf(buffer, sizeof(buffer), "line:%s", widget.id);
        ops.emplace_back(buffer);
    }

    void drawCircle(const DisplayLayoutWidgetV1& widget) override {
        char buffer[64]{};
        std::snprintf(buffer, sizeof(buffer), "circle:%s", widget.id);
        ops.emplace_back(buffer);
    }

    void drawEllipse(const DisplayLayoutWidgetV1& widget) override {
        char buffer[64]{};
        std::snprintf(buffer, sizeof(buffer), "ellipse:%s", widget.id);
        ops.emplace_back(buffer);
    }

    void drawBitmap(const DisplayLayoutWidgetV1& widget) override {
        char buffer[64]{};
        std::snprintf(buffer, sizeof(buffer), "bitmap:%s", widget.id);
        ops.emplace_back(buffer);
    }

    std::vector<std::string> ops;
};

class TestDisplayDevice final : public DisplayDeviceBase {
public:
    explicit TestDisplayDevice(FakeDisplaySurface& surface) : DisplayDeviceBase(DisplayDeviceBase::initialState()), surface_(surface) {}

    void enableReadyState() {
        status_ = DeviceStatus::Ready;
    }

protected:
    void writeDisplayConfigJson(JsonObject output) const override {
        (void)output;
    }

    IDisplayRenderSurface* renderSurface() const override {
        return &surface_;
    }

private:
    FakeDisplaySurface& surface_;
};

DisplayLayoutWidgetV1 makeTextWidget(const char* id, const char* text, uint16_t refreshIntervalMs) {
    DisplayLayoutWidgetV1 widget{};
    std::snprintf(widget.id, sizeof(widget.id), "%s", id);
    widget.type = static_cast<uint8_t>(DisplayLayoutWidgetType::Text);
    widget.bindingKind = static_cast<uint8_t>(DisplayLayoutBindingKind::ConstantText);
    std::snprintf(widget.text, sizeof(widget.text), "%s", text);
    widget.refreshIntervalMs = refreshIntervalMs;
    return widget;
}

DisplayLayoutWidgetV1 makeMetricWidget(const char* id, const char* text, uint16_t refreshIntervalMs) {
    DisplayLayoutWidgetV1 widget = makeTextWidget(id, text, refreshIntervalMs);
    widget.bindingKind = static_cast<uint8_t>(DisplayLayoutBindingKind::Metric);
    return widget;
}

DisplayLayoutWidgetV1 makeShapeWidget(const char* id, DisplayLayoutWidgetType type) {
    DisplayLayoutWidgetV1 widget{};
    std::snprintf(widget.id, sizeof(widget.id), "%s", id);
    widget.type = static_cast<uint8_t>(type);
    widget.width = 10;
    widget.height = 10;
    return widget;
}

} // namespace

void test_display_layout_renderer_clears_once_and_uses_min_refresh_interval() {
    MemoryConfigStorage registryStorage;
    DeviceRegistryStore registryStore(registryStorage);
    TEST_ASSERT_TRUE(registryStore.begin(false));
    SequentialDeviceIdSource idSource(40);
    DeviceTypeRegistry types = DeviceTypeRegistry::withDefaults();
    DeviceRegistry registry(registryStore, types, idSource);
    TEST_ASSERT_TRUE(registry.begin(1000).ok());
    const DeviceId dummyId = createDummy(registry, "living-room-sensor", 1000);
    char devicePlaceholder[48]{};
    std::snprintf(devicePlaceholder, sizeof(devicePlaceholder), "Device {{dev.%lu.status}}", static_cast<unsigned long>(dummyId));

    DisplayLayoutRecordV1 layout{};
    layout.activePageIndex = 0;
    DisplayLayoutPageV1 page{};
    std::snprintf(page.id, sizeof(page.id), "%s", "main");
    std::snprintf(page.name, sizeof(page.name), "%s", "Main");
    page.widgets.push_back(makeTextWidget("static", "Hello", kDisplayLayoutRefreshIntervalDisabled));
    page.widgets.push_back(makeMetricWidget("device_status", devicePlaceholder, kDisplayLayoutRefreshIntervalDisabled));
    page.widgets.push_back(makeMetricWidget("system_time", "Time {{system.time}}", kDisplayLayoutRefreshIntervalDisabled));
    page.widgets.push_back(makeMetricWidget("wifi_status", "WiFi {{wifi.status}}", 1000));
    page.widgets.push_back(makeShapeWidget("box", DisplayLayoutWidgetType::Rect));
    layout.pages.push_back(page);

    FakeWifiDriver wifi;
    MetricValueResolver resolver(&registry, wifi, 3723000U);
    FakeDisplaySurface surface;
    DisplayLayoutRenderSession session;

    const DisplayLayoutRenderResult first = session.render(layout, resolver, surface, 3723000U);
    TEST_ASSERT_TRUE(first.rendered);
    TEST_ASSERT_TRUE(first.cleared);
    TEST_ASSERT_TRUE(first.hasDynamicWidgets);
    TEST_ASSERT_EQUAL_UINT16(250U, first.refreshIntervalMs);
    TEST_ASSERT_EQUAL_UINT8(5U, first.renderedWidgetCount);
    TEST_ASSERT_EQUAL_UINT32(6U, static_cast<uint32_t>(surface.ops.size()));
    TEST_ASSERT_EQUAL_STRING("clear", surface.ops[0].c_str());
    TEST_ASSERT_EQUAL_STRING("text:static:Hello", surface.ops[1].c_str());
    TEST_ASSERT_EQUAL_STRING("text:device_status:Device ready", surface.ops[2].c_str());
    TEST_ASSERT_EQUAL_STRING("text:system_time:Time 1:02:03", surface.ops[3].c_str());
    TEST_ASSERT_EQUAL_STRING("text:wifi_status:WiFi connected", surface.ops[4].c_str());
    TEST_ASSERT_EQUAL_STRING("rect:box", surface.ops[5].c_str());

    const DisplayLayoutRenderResult notDue = session.render(layout, resolver, surface, 3723100U);
    TEST_ASSERT_FALSE(notDue.rendered);
    TEST_ASSERT_EQUAL_UINT32(6U, static_cast<uint32_t>(surface.ops.size()));

    const DisplayLayoutRenderResult second = session.render(layout, resolver, surface, 3723250U);
    TEST_ASSERT_TRUE(second.rendered);
    TEST_ASSERT_TRUE(second.cleared);
    TEST_ASSERT_EQUAL_UINT32(12U, static_cast<uint32_t>(surface.ops.size()));
    TEST_ASSERT_EQUAL_STRING("clear", surface.ops[6].c_str());
}

void test_display_layout_renderer_rerenders_when_page_changes() {
    DisplayLayoutRecordV1 layout{};
    layout.activePageIndex = 0;

    DisplayLayoutPageV1 firstPage{};
    std::snprintf(firstPage.id, sizeof(firstPage.id), "%s", "first");
    std::snprintf(firstPage.name, sizeof(firstPage.name), "%s", "First");
    firstPage.widgets.push_back(makeTextWidget("first_text", "First", kDisplayLayoutRefreshIntervalDisabled));

    DisplayLayoutPageV1 secondPage{};
    std::snprintf(secondPage.id, sizeof(secondPage.id), "%s", "second");
    std::snprintf(secondPage.name, sizeof(secondPage.name), "%s", "Second");
    secondPage.widgets.push_back(makeTextWidget("second_text", "Second", kDisplayLayoutRefreshIntervalDisabled));

    layout.pages.push_back(firstPage);
    layout.pages.push_back(secondPage);

    FakeWifiDriver wifi;
    MetricValueResolver resolver(nullptr, wifi, 0U);
    FakeDisplaySurface surface;
    DisplayLayoutRenderSession session;

    TEST_ASSERT_TRUE(session.render(layout, resolver, surface, 0U).rendered);
    TEST_ASSERT_EQUAL_UINT32(2U, static_cast<uint32_t>(surface.ops.size()));

    layout.activePageIndex = 1;
    const DisplayLayoutRenderResult rerendered = session.render(layout, resolver, surface, 1U);
    TEST_ASSERT_TRUE(rerendered.rendered);
    TEST_ASSERT_TRUE(rerendered.pageChanged);
    TEST_ASSERT_EQUAL_UINT32(4U, static_cast<uint32_t>(surface.ops.size()));
    TEST_ASSERT_EQUAL_STRING("clear", surface.ops[2].c_str());
    TEST_ASSERT_EQUAL_STRING("text:second_text:Second", surface.ops[3].c_str());
}

void test_display_device_clears_once_when_layout_becomes_empty() {
    FakeWifiDriver wifi;
    MetricValueResolver resolver(nullptr, wifi, 0U);
    FakeDisplaySurface surface;
    TestDisplayDevice device(surface);
    device.enableReadyState();

    DisplayLayoutRecordV1 layout{};
    layout.activePageIndex = 0;
    DisplayLayoutPageV1 page{};
    std::snprintf(page.id, sizeof(page.id), "%s", "main");
    std::snprintf(page.name, sizeof(page.name), "%s", "Main");
    page.widgets.push_back(makeTextWidget("static", "Hello", kDisplayLayoutRefreshIntervalDisabled));
    layout.pages.push_back(page);

    device.setLayout(layout);
    TEST_ASSERT_TRUE(device.renderDisplay(resolver, 0U));
    TEST_ASSERT_EQUAL_UINT32(2U, static_cast<uint32_t>(surface.ops.size()));
    TEST_ASSERT_EQUAL_STRING("clear", surface.ops[0].c_str());
    TEST_ASSERT_EQUAL_STRING("text:static:Hello", surface.ops[1].c_str());

    DisplayLayoutRecordV1 emptyLayout{};
    device.setLayout(emptyLayout);
    TEST_ASSERT_TRUE(device.renderDisplay(resolver, 1U));
    TEST_ASSERT_EQUAL_UINT32(3U, static_cast<uint32_t>(surface.ops.size()));
    TEST_ASSERT_EQUAL_STRING("clear", surface.ops[2].c_str());

    TEST_ASSERT_FALSE(device.renderDisplay(resolver, 2U));
    TEST_ASSERT_EQUAL_UINT32(3U, static_cast<uint32_t>(surface.ops.size()));
}
