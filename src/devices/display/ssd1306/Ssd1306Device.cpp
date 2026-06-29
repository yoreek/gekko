#include "devices/display/ssd1306/Ssd1306Device.h"

#if defined(ARDUINO) && !defined(UNIT_TEST)
#include <Adafruit_SSD1306.h>
#include <Arduino.h>
#endif

#include "devices/bus/i2c/I2cBusDevice.h"

#include <cstring>
#include <type_traits>

namespace ewfm {

namespace {
constexpr DeviceTypeId kSsd1306DeviceTypeId = 7;
constexpr uint32_t kSsd1306DeviceConfigVersion = 1;
} // namespace

#if defined(ARDUINO) && !defined(UNIT_TEST)
class Ssd1306CanvasSurface final : public IDisplayRenderSurface {
public:
    explicit Ssd1306CanvasSurface(::Adafruit_SSD1306& display) : display_(display) {}

    void clear() override {
        display_.clearDisplay();
    }

    void drawText(const DisplayLayoutWidgetV1& widget, const DisplayTextEvaluationResult& text) override {
        if (widget.width == 0U || widget.height == 0U) {
            return;
        }
        GFXcanvas1 canvas(widget.width, widget.height);
        canvas.fillScreen(0);
        canvas.setTextSize(widget.fontSize == 0U ? 1U : widget.fontSize);
        canvas.setTextWrap((widget.styleFlags & 0x04U) != 0U);
        canvas.setTextColor(1);
        int16_t x1 = 0;
        int16_t y1 = 0;
        uint16_t boundsWidth = 0;
        uint16_t boundsHeight = 0;
        canvas.getTextBounds(text.text, 0, 0, &x1, &y1, &boundsWidth, &boundsHeight);
        canvas.setCursor(static_cast<int16_t>(-x1), static_cast<int16_t>(-y1));
        canvas.print(text.text);
        blit(widget, canvas.getBuffer());
    }

    void drawRect(const DisplayLayoutWidgetV1& widget) override {
        drawBox(widget, [&](GFXcanvas1& canvas) { canvas.drawRect(0, 0, widget.width, widget.height, 1); });
    }

    void drawLine(const DisplayLayoutWidgetV1& widget) override {
        drawBox(widget, [&](GFXcanvas1& canvas) {
            const int16_t x2 = widget.width > 0U ? static_cast<int16_t>(widget.width - 1U) : 0;
            const int16_t y2 = widget.height > 0U ? static_cast<int16_t>(widget.height - 1U) : 0;
            canvas.drawLine(0, 0, x2, y2, 1);
        });
    }

    void drawCircle(const DisplayLayoutWidgetV1& widget) override {
        drawBox(widget, [&](GFXcanvas1& canvas) {
            const int16_t radius = static_cast<int16_t>((widget.width < widget.height ? widget.width : widget.height) / 2U);
            canvas.drawCircle(static_cast<int16_t>(widget.width / 2U), static_cast<int16_t>(widget.height / 2U), radius, 1);
        });
    }

    void drawEllipse(const DisplayLayoutWidgetV1& widget) override {
        drawBox(widget, [&](GFXcanvas1& canvas) {
            const int16_t radiusX = static_cast<int16_t>(widget.width / 2U);
            const int16_t radiusY = static_cast<int16_t>(widget.height / 2U);
            canvas.drawEllipse(static_cast<int16_t>(widget.width / 2U), static_cast<int16_t>(widget.height / 2U), radiusX, radiusY, 1);
        });
    }

    void drawBitmap(const DisplayLayoutWidgetV1& widget) override {
        if (widget.width == 0U || widget.height == 0U || widget.bitmapData.empty()) {
            return;
        }
        GFXcanvas1 canvas(widget.width, widget.height);
        canvas.fillScreen(0);
        const size_t pixelCount = static_cast<size_t>(widget.width) * static_cast<size_t>(widget.height);
        switch (static_cast<DisplayLayoutBitmapFormat>(widget.bitmapFormat)) {
        case DisplayLayoutBitmapFormat::Mono1: {
            for (size_t index = 0U; index < pixelCount; ++index) {
                const size_t byteIndex = index / 8U;
                const uint8_t mask = static_cast<uint8_t>(0x80U >> (index % 8U));
                if (byteIndex < widget.bitmapData.size() && (widget.bitmapData[byteIndex] & mask) != 0U) {
                    const int16_t x = static_cast<int16_t>(index % widget.width);
                    const int16_t y = static_cast<int16_t>(index / widget.width);
                    canvas.drawPixel(x, y, 1);
                }
            }
            break;
        }
        case DisplayLayoutBitmapFormat::Gray8: {
            for (size_t index = 0U; index < pixelCount && index < widget.bitmapData.size(); ++index) {
                if (widget.bitmapData[index] >= 128U) {
                    const int16_t x = static_cast<int16_t>(index % widget.width);
                    const int16_t y = static_cast<int16_t>(index / widget.width);
                    canvas.drawPixel(x, y, 1);
                }
            }
            break;
        }
        case DisplayLayoutBitmapFormat::Rgb565: {
            for (size_t index = 0U; index < pixelCount; ++index) {
                const size_t byteIndex = index * 2U;
                if (byteIndex + 1U >= widget.bitmapData.size()) {
                    break;
                }
                const uint16_t pixel = static_cast<uint16_t>(widget.bitmapData[byteIndex]) << 8U |
                                       static_cast<uint16_t>(widget.bitmapData[byteIndex + 1U]);
                if (pixel != 0U) {
                    const int16_t x = static_cast<int16_t>(index % widget.width);
                    const int16_t y = static_cast<int16_t>(index / widget.width);
                    canvas.drawPixel(x, y, 1);
                }
            }
            break;
        }
        }
        blit(widget, canvas.getBuffer());
    }

private:
    template <typename DrawFn> void drawBox(const DisplayLayoutWidgetV1& widget, DrawFn&& drawFn) {
        if (widget.width == 0U || widget.height == 0U) {
            return;
        }
        GFXcanvas1 canvas(widget.width, widget.height);
        canvas.fillScreen(0);
        drawFn(canvas);
        blit(widget, canvas.getBuffer());
    }

    void blit(const DisplayLayoutWidgetV1& widget, const uint8_t* buffer) {
        if (buffer == nullptr) {
            return;
        }
        display_.drawBitmap(widget.x, widget.y, buffer, widget.width, widget.height, 1);
    }

    ::Adafruit_SSD1306& display_;
};
#endif

static_assert(std::is_trivially_copyable<Ssd1306DeviceConfigV1>::value, "Ssd1306DeviceConfigV1 must be POD");
static_assert(sizeof(Ssd1306DeviceConfigV1::kMagic) - 1U + sizeof(Ssd1306DeviceConfigV1) <= kMaxDeviceConfigBytes,
              "Ssd1306DeviceConfigV1 exceeds device config bound");

Ssd1306Device::Ssd1306Device(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob)
    : DisplayDeviceBase(DisplayDeviceBase::initialState()) {
    bindDeviceIdentity(record, configBlob);
    (void)decodeSsd1306DeviceConfig(configBlob.data(), configBlob.size(), config_);
}

Ssd1306Device::~Ssd1306Device() = default;

const Ssd1306DeviceConfigV1& Ssd1306Device::config() const {
    return config_;
}

bool Ssd1306Device::enabled() const {
    return config_.enabled != 0U;
}

const char* Ssd1306Device::name() const {
    return config_.name;
}

bool Ssd1306Device::i2cAddress(uint8_t& address) const {
    address = config_.i2cAddress;
    return true;
}

bool Ssd1306Device::serializeConfigBlob(DeviceConfigBlob& configBlob) const {
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    const size_t size = ssd1306DeviceConfigSize(config_);
    return encodeSsd1306DeviceConfig(config_, buffer, size) && configBlob.assign(buffer, size);
}

bool Ssd1306Device::replaceBaseConfig(DeviceConfigBlob& configBlob, const DeviceBaseConfigV1& baseConfig) const {
    Ssd1306DeviceConfigV1 config = config_;
    config.enabled = baseConfig.enabled;
    std::memcpy(config.name, baseConfig.name, sizeof(config.name));
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    const size_t size = ssd1306DeviceConfigSize(config);
    return encodeSsd1306DeviceConfig(config, buffer, size) && configBlob.assign(buffer, size);
}

DeviceConfigUpdatePlan Ssd1306Device::planConfigUpdate(const DeviceConfigBlob& configBlob) const {
    Ssd1306DeviceConfigV1 config{};
    if (!decodeSsd1306DeviceConfig(configBlob.data(), configBlob.size(), config)) {
        return {};
    }
    DeviceConfigUpdatePlan plan{};
    plan.endOldConfig = config.i2cBusDeviceId != config_.i2cBusDeviceId || config.i2cAddress != config_.i2cAddress ||
                        config.layoutWidth != config_.layoutWidth || config.layoutHeight != config_.layoutHeight;
    plan.resetStateMachine = plan.endOldConfig;
    return plan;
}

bool Ssd1306Device::applyConfig(const DeviceConfigBlob& configBlob, uint32_t now) {
    (void)now;
    Ssd1306DeviceConfigV1 config{};
    if (!decodeSsd1306DeviceConfig(configBlob.data(), configBlob.size(), config)) {
        return false;
    }
    config_ = config;
    return true;
}

void Ssd1306Device::writeDisplayConfigJson(JsonObject output) const {
    config_.writeJson(output);
}

bool Ssd1306Device::initializeDisplayHardware(uint32_t now) {
    (void)now;
#if defined(ARDUINO) && !defined(UNIT_TEST)
    I2cBusDevice* bus = static_cast<I2cBusDevice*>(dependencyRuntime(DeviceDependencyRole::I2CBus));
    if (bus == nullptr) {
        return false;
    }
    I2cBusDevice::DependencyTransaction transaction = bus->beginDependencyTransaction();
    if (!transaction) {
        return false;
    }
    II2cBusDriver* driver = transaction.driver();
    TwoWire* wire = driver != nullptr ? driver->nativeWire() : nullptr;
    if (wire == nullptr) {
        return false;
    }
    std::unique_ptr<::Adafruit_SSD1306> display(
        new ::Adafruit_SSD1306(static_cast<uint8_t>(config_.layoutWidth), static_cast<uint8_t>(config_.layoutHeight), wire, -1));
    if (display == nullptr) {
        return false;
    }
    if (!display->begin(SSD1306_SWITCHCAPVCC, config_.i2cAddress, true, true)) {
        return false;
    }
    display->clearDisplay();
    display->display();
    surface_ = std::unique_ptr<Ssd1306CanvasSurface>(new Ssd1306CanvasSurface(*display));
    display_ = std::move(display);
    return true;
#else
    return true;
#endif
}

void Ssd1306Device::releaseDisplayHardware(uint32_t now) {
    (void)now;
#if defined(ARDUINO) && !defined(UNIT_TEST)
    surface_.reset();
    if (display_ != nullptr) {
        display_->clearDisplay();
        display_->display();
        display_.reset();
    }
#endif
}

IDisplayRenderSurface* Ssd1306Device::renderSurface() const {
#if defined(ARDUINO) && !defined(UNIT_TEST)
    return surface_.get();
#else
    return nullptr;
#endif
}

void Ssd1306Device::onDisplayFrameRendered(const DisplayLayoutRenderResult& result) {
    (void)result;
#if defined(ARDUINO) && !defined(UNIT_TEST)
    if (display_ != nullptr) {
        display_->display();
    }
#endif
}

DeviceTypeDescriptor Ssd1306Device::descriptor() {
    DeviceTypeDescriptor descriptor;
    descriptor.typeId = kSsd1306DeviceTypeId;
    descriptor.name = "Ssd1306Device";
    descriptor.currentConfigVersion = kSsd1306DeviceConfigVersion;
    descriptor.maxDependents = 16;
    descriptor.supportsCommands = false;
    descriptor.supportsRetainedState = false;
    descriptor.defaultPersistencePolicy = DevicePersistencePolicy::Delayed;
    descriptor.ticks100ms = true;
    descriptor.dependencyRequirements = {
        {DeviceDependencyRole::I2CBus, true, {I2cBusDevice::descriptor().typeId}},
    };
    descriptor.createRuntime = &Ssd1306Device::createRuntime;
    descriptor.validateConfig = &Ssd1306Device::validateConfig;
    return descriptor;
}

std::unique_ptr<IDeviceRuntime> Ssd1306Device::createRuntime(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob) {
    return std::unique_ptr<IDeviceRuntime>(new Ssd1306Device(record, configBlob));
}

DeviceValidationResult Ssd1306Device::validateConfig(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob) {
    (void)record;
    if (configBlob.size() > kMaxDeviceConfigBytes) {
        return {DeviceError::BoundsExceeded, "ssd1306 config exceeds supported size"};
    }
    Ssd1306DeviceConfigV1 config{};
    if (!decodeSsd1306DeviceConfig(configBlob.data(), configBlob.size(), config)) {
        return {DeviceError::InvalidConfig, "ssd1306 config is invalid"};
    }
    return {};
}

} // namespace ewfm
