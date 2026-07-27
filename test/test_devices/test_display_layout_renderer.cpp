#include "config/MemoryConfigStorage.h"
#include "devices/display/DisplayDeviceBase.h"
#include "devices/display/DisplayLayoutProfile.h"
#include "devices/display/DisplayLayoutRenderer.h"
#include "devices/display/DisplayLayoutValidator.h"
#include "devices/display/render/CharacterDisplayLayoutRenderer.h"
#include "devices/display/render/DisplayRenderSurface.h"
#include "devices/display/render/PixelDisplayLayoutRenderer.h"
#include "devices/display/render/SegmentDisplayLayoutRenderer.h"
#include "devices/dummy/DummyDevice.h"
#include "devices/registry/DeviceRegistry.h"
#include "devices/registry/DeviceRegistryStore.h"
#include "metrics/MetricValueResolver.h"
#include "time/DateTime.h"
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

class FakePixelDisplaySurface final : public IPixelDisplayRenderSurface {
public:
    void clear(uint16_t color) override {
        record("clear", color);
    }

    void drawText(const DisplayLayoutWidgetV1& widget, const DisplayTextEvaluationResult& text) override {
        recordWidget("text", widget.id, text.text);
    }

    void drawRect(const DisplayLayoutWidgetV1& widget) override {
        recordWidget("rect", widget.id);
    }

    void drawLine(const DisplayLayoutWidgetV1& widget) override {
        recordWidget("line", widget.id);
    }

    void drawCircle(const DisplayLayoutWidgetV1& widget) override {
        recordWidget("circle", widget.id);
    }

    void drawEllipse(const DisplayLayoutWidgetV1& widget) override {
        recordWidget("ellipse", widget.id);
    }

    void drawBitmap(const DisplayLayoutWidgetV1& widget) override {
        recordWidget("bitmap", widget.id);
    }

    std::vector<std::string> ops;

private:
    void record(const char* op, uint16_t value) {
        char buffer[32]{};
        std::snprintf(buffer, sizeof(buffer), "%s:%04X", op, value);
        ops.emplace_back(buffer);
    }

    void recordWidget(const char* op, const char* id, const char* value = nullptr) {
        char buffer[128]{};
        if (value != nullptr) {
            std::snprintf(buffer, sizeof(buffer), "%s:%s:%s", op, id, value);
        } else {
            std::snprintf(buffer, sizeof(buffer), "%s:%s", op, id);
        }
        ops.emplace_back(buffer);
    }
};

class FakeCharacterDisplaySurface final : public ICharacterDisplayRenderSurface {
public:
    void clear(uint16_t color) override {
        record("clear", color);
    }

    void drawText(const DisplayLayoutWidgetV1& widget, const DisplayTextEvaluationResult& text) override {
        char buffer[128]{};
        std::snprintf(buffer, sizeof(buffer), "text:%s:%s", widget.id, text.text);
        ops.emplace_back(buffer);
    }

    std::vector<std::string> ops;

private:
    void record(const char* op, uint16_t value) {
        char buffer[32]{};
        std::snprintf(buffer, sizeof(buffer), "%s:%04X", op, value);
        ops.emplace_back(buffer);
    }
};

class FakeSegmentDisplaySurface final : public ISegmentDisplayRenderSurface {
public:
    void clear(uint16_t color) override {
        record("clear", color);
    }

    void drawDigital(const DisplayLayoutWidgetV1& widget, const DisplayDigitalFrame& frame) override {
        digitalWidgetId = widget.id;
        digitalFrame = frame;
        char buffer[128]{};
        std::snprintf(buffer, sizeof(buffer), "digital:%s:%u", widget.id, static_cast<unsigned>(frame.cellCount));
        ops.emplace_back(buffer);
    }

    std::vector<std::string> ops;
    std::string digitalWidgetId{};
    DisplayDigitalFrame digitalFrame{};

private:
    void record(const char* op, uint16_t value) {
        char buffer[32]{};
        std::snprintf(buffer, sizeof(buffer), "%s:%04X", op, value);
        ops.emplace_back(buffer);
    }
};

class TestCharacterDisplayDevice final : public DisplayDeviceBase {
public:
    explicit TestCharacterDisplayDevice(FakeCharacterDisplaySurface& surface)
        : DisplayDeviceBase(DisplayDeviceBase::initialState()), surface_(surface) {}

    void enableReadyState() {
        status_ = DeviceStatus::Ready;
    }

protected:
    void writeDisplayConfigJson(JsonObject output) const override {
        (void)output;
    }

    bool clearDisplay(uint16_t color) override {
        surface_.clear(color);
        return true;
    }

    DisplayLayoutRenderResult renderDisplayFrame(const MetricValueResolver& resolver, uint32_t now) override {
        CharacterDisplayLayoutRenderer renderer(surface_);
        return renderSession_.render(layout_, resolver, renderer, now);
    }

private:
    FakeCharacterDisplaySurface& surface_;
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

DisplayLayoutWidgetV1 makeShapeWidget(const char* id, DisplayLayoutWidgetType type) {
    DisplayLayoutWidgetV1 widget{};
    std::snprintf(widget.id, sizeof(widget.id), "%s", id);
    widget.type = static_cast<uint8_t>(type);
    widget.width = 10;
    widget.height = 10;
    return widget;
}

DisplayLayoutWidgetV1 makeDigitalWidget(const char* id, const char* text) {
    DisplayLayoutWidgetV1 widget{};
    std::snprintf(widget.id, sizeof(widget.id), "%s", id);
    widget.type = static_cast<uint8_t>(DisplayLayoutWidgetType::Digital);
    widget.bindingKind = static_cast<uint8_t>(DisplayLayoutBindingKind::ConstantText);
    widget.width = 4;
    widget.height = 1;
    std::snprintf(widget.text, sizeof(widget.text), "%s", text);
    std::snprintf(widget.digitalOverflow, sizeof(widget.digitalOverflow), "%s", "----");
    std::snprintf(widget.digitalMissing, sizeof(widget.digitalMissing), "%s", "----");
    widget.digitalAlign = static_cast<uint8_t>(DisplayDigitalAlign::Right);
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
    char devicePlaceholder[48]{};
    std::snprintf(devicePlaceholder, sizeof(devicePlaceholder), "%s", "Device {{system.wifi.station_ip}}");

    DisplayLayoutRecordV1 layout{};
    layout.backgroundColor = 0x1234U;
    layout.activePageIndex = 0;
    DisplayLayoutPageV1 page{};
    std::snprintf(page.id, sizeof(page.id), "%s", "main");
    std::snprintf(page.name, sizeof(page.name), "%s", "Main");
    page.widgets.push_back(makeTextWidget("static", "Hello", kDisplayLayoutRefreshIntervalDisabled));
    page.widgets.push_back(makeTextWidget("device_status", devicePlaceholder, kDisplayLayoutRefreshIntervalDisabled));
    page.widgets.push_back(makeTextWidget("system_time", "Time {{system.time}}", kDisplayLayoutRefreshIntervalDisabled));
    page.widgets.push_back(makeTextWidget("wifi_status", "WiFi {{system.wifi.station_ip}}", 1000));
    page.widgets.push_back(makeShapeWidget("box", DisplayLayoutWidgetType::Rect));
    layout.pages.push_back(page);

    FakeWifiDriver wifi;
    const DateTime fixedTime(2026, 7, 11, 20, 15, 0);
    MetricValueResolver resolver(&registry, wifi, 3723000U, fixedTime);
    FakePixelDisplaySurface surface;
    DisplayLayoutRenderSession session;
    PixelDisplayLayoutRenderer renderer(surface);

    const DisplayLayoutRenderResult first = session.render(layout, resolver, renderer, 3723000U);
    TEST_ASSERT_TRUE(first.rendered);
    TEST_ASSERT_TRUE(first.cleared);
    TEST_ASSERT_TRUE(first.hasDynamicWidgets);
    TEST_ASSERT_EQUAL_UINT16(250U, first.refreshIntervalMs);
    TEST_ASSERT_EQUAL_UINT8(5U, first.renderedWidgetCount);
    TEST_ASSERT_EQUAL_UINT32(6U, static_cast<uint32_t>(surface.ops.size()));
    TEST_ASSERT_EQUAL_STRING("clear:1234", surface.ops[0].c_str());
    TEST_ASSERT_EQUAL_STRING("text:static:Hello", surface.ops[1].c_str());
    TEST_ASSERT_EQUAL_STRING("text:device_status:Device 192.168.1.50", surface.ops[2].c_str());
    TEST_ASSERT_EQUAL_STRING("text:system_time:Time 20:15:00", surface.ops[3].c_str());
    TEST_ASSERT_EQUAL_STRING("text:wifi_status:WiFi 192.168.1.50", surface.ops[4].c_str());
    TEST_ASSERT_EQUAL_STRING("rect:box", surface.ops[5].c_str());

    const DisplayLayoutRenderResult notDue = session.render(layout, resolver, renderer, 3723100U);
    TEST_ASSERT_FALSE(notDue.rendered);
    TEST_ASSERT_EQUAL_UINT32(6U, static_cast<uint32_t>(surface.ops.size()));

    const DisplayLayoutRenderResult second = session.render(layout, resolver, renderer, 3723250U);
    TEST_ASSERT_TRUE(second.rendered);
    TEST_ASSERT_TRUE(second.cleared);
    TEST_ASSERT_EQUAL_UINT32(12U, static_cast<uint32_t>(surface.ops.size()));
    TEST_ASSERT_EQUAL_STRING("clear:1234", surface.ops[6].c_str());
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
    FakeCharacterDisplaySurface surface;
    DisplayLayoutRenderSession session;
    CharacterDisplayLayoutRenderer renderer(surface);

    TEST_ASSERT_TRUE(session.render(layout, resolver, renderer, 0U).rendered);
    TEST_ASSERT_EQUAL_UINT32(2U, static_cast<uint32_t>(surface.ops.size()));

    layout.activePageIndex = 1;
    const DisplayLayoutRenderResult rerendered = session.render(layout, resolver, renderer, 1U);
    TEST_ASSERT_TRUE(rerendered.rendered);
    TEST_ASSERT_TRUE(rerendered.pageChanged);
    TEST_ASSERT_EQUAL_UINT32(4U, static_cast<uint32_t>(surface.ops.size()));
    TEST_ASSERT_EQUAL_STRING("clear:0000", surface.ops[2].c_str());
    TEST_ASSERT_EQUAL_STRING("text:second_text:Second", surface.ops[3].c_str());
}

void test_display_layout_renderer_renders_digital_widget_with_decimal_point() {
    DisplayLayoutRecordV1 layout{};
    layout.activePageIndex = 0;
    layout.backgroundColor = 0U;

    DisplayLayoutPageV1 page{};
    std::snprintf(page.id, sizeof(page.id), "%s", "main");
    std::snprintf(page.name, sizeof(page.name), "%s", "Main");
    page.widgets.push_back(makeDigitalWidget("digital", "12.34"));
    layout.pages.push_back(page);

    FakeWifiDriver wifi;
    MetricValueResolver resolver(nullptr, wifi, 0U);
    FakeSegmentDisplaySurface surface;
    DisplayLayoutRenderSession session;
    SegmentDisplayLayoutRenderer renderer(surface);

    TEST_ASSERT_TRUE(session.render(layout, resolver, renderer, 0U).rendered);
    TEST_ASSERT_EQUAL_STRING("digital", surface.digitalWidgetId.c_str());
    TEST_ASSERT_EQUAL_UINT8(4U, surface.digitalFrame.cellCount);
    TEST_ASSERT_EQUAL_CHAR('1', surface.digitalFrame.cells[0].glyph);
    TEST_ASSERT_EQUAL_CHAR('2', surface.digitalFrame.cells[1].glyph);
    TEST_ASSERT_EQUAL_CHAR('3', surface.digitalFrame.cells[2].glyph);
    TEST_ASSERT_EQUAL_CHAR('4', surface.digitalFrame.cells[3].glyph);
    TEST_ASSERT_EQUAL_UINT8(0U, surface.digitalFrame.cells[0].decimalPoint);
    TEST_ASSERT_EQUAL_UINT8(1U, surface.digitalFrame.cells[1].decimalPoint);
    TEST_ASSERT_EQUAL_UINT8(0U, surface.digitalFrame.cells[2].decimalPoint);
    TEST_ASSERT_EQUAL_UINT8(0U, surface.digitalFrame.cells[3].decimalPoint);
}

void test_display_layout_validator_handles_logical_cell_bounds() {
    DisplayLayoutProfile profile = segmentDisplayLayoutProfile(4U);

    DisplayLayoutWidgetV1 widget{};
    widget.type = static_cast<uint8_t>(DisplayLayoutWidgetType::Digital);
    widget.width = 4U;
    widget.height = 1U;
    widget.digitalAlign = static_cast<uint8_t>(DisplayDigitalAlign::Right);
    std::snprintf(widget.text, sizeof(widget.text), "%s", "12.34");

    TEST_ASSERT_TRUE(validateDisplayLayoutWidget(widget, profile).ok());

    widget.x = 3U;
    TEST_ASSERT_FALSE(validateDisplayLayoutWidget(widget, profile).ok());
}

void test_display_device_clears_once_when_layout_becomes_empty() {
    FakeWifiDriver wifi;
    MetricValueResolver resolver(nullptr, wifi, 0U);
    FakeCharacterDisplaySurface surface;
    TestCharacterDisplayDevice device(surface);
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
    TEST_ASSERT_EQUAL_STRING("clear:0000", surface.ops[0].c_str());
    TEST_ASSERT_EQUAL_STRING("text:static:Hello", surface.ops[1].c_str());

    DisplayLayoutRecordV1 emptyLayout{};
    device.setLayout(emptyLayout);
    TEST_ASSERT_TRUE(device.renderDisplay(resolver, 1U));
    TEST_ASSERT_EQUAL_UINT32(3U, static_cast<uint32_t>(surface.ops.size()));
    TEST_ASSERT_EQUAL_STRING("clear:0000", surface.ops[2].c_str());

    TEST_ASSERT_FALSE(device.renderDisplay(resolver, 2U));
    TEST_ASSERT_EQUAL_UINT32(3U, static_cast<uint32_t>(surface.ops.size()));
}
