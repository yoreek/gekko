#include "devices/display/ssd1306/Ssd1306Device.h"

#if defined(ARDUINO) && !defined(UNIT_TEST)
#include <Adafruit_SSD1306.h>
#include <Arduino.h>
#endif

#include "devices/bus/i2c/I2cBusDevice.h"
#include "devices/bus/i2c/I2cDeviceValidation.h"
#include "devices/display/DisplayBitmapCache.h"

#include <cstring>
#include <type_traits>

namespace ewfm {

namespace {
constexpr DeviceTypeId kSsd1306DeviceTypeId = 7;
constexpr uint32_t kSsd1306DeviceConfigVersion = 6;
} // namespace

#if defined(ARDUINO) && !defined(UNIT_TEST)
class Ssd1306CanvasSurface final : public IPixelDisplayRenderSurface {
public:
    explicit Ssd1306CanvasSurface(::Adafruit_SSD1306& display) : display_(display) {}

    void clear(const uint16_t color) override {
        backgroundOn_ = color != 0U;
        display_.fillScreen(backgroundOn_ ? SSD1306_WHITE : SSD1306_BLACK);
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
        const uint16_t widgetColor = widget.color != 0U ? SSD1306_WHITE : SSD1306_BLACK;
        if ((widget.styleFlags & 0x02U) != 0U) {
            blit(widget, canvas.getBuffer(), backgroundOn_ ? SSD1306_WHITE : SSD1306_BLACK, widgetColor);
        } else {
            blit(widget, canvas.getBuffer(), widgetColor);
        }
    }

    void drawRect(const DisplayLayoutWidgetV1& widget) override {
        drawBox(widget, [&](GFXcanvas1& canvas) {
            if (isFilledShape(widget)) {
                canvas.fillRect(0, 0, widget.width, widget.height, 1);
            } else {
                canvas.drawRect(0, 0, widget.width, widget.height, 1);
            }
        });
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
            const int16_t cx = static_cast<int16_t>(widget.width / 2U);
            const int16_t cy = static_cast<int16_t>(widget.height / 2U);
            // Inset by 1px: a radius reaching exactly to the canvas edge (e.g. width/2 on an
            // even width) lands the outline on the one-past-the-end row/column, which the
            // canvas silently clips - producing visible gaps in the outline.
            const int16_t diameter =
                widget.width < widget.height ? static_cast<int16_t>(widget.width) : static_cast<int16_t>(widget.height);
            const int16_t radius = diameter > 1 ? static_cast<int16_t>((diameter - 1) / 2) : 0;
            if (isFilledShape(widget)) {
                canvas.fillCircle(cx, cy, radius, 1);
            } else {
                canvas.drawCircle(cx, cy, radius, 1);
            }
        });
    }

    void drawEllipse(const DisplayLayoutWidgetV1& widget) override {
        drawBox(widget, [&](GFXcanvas1& canvas) {
            const int16_t cx = static_cast<int16_t>(widget.width / 2U);
            const int16_t cy = static_cast<int16_t>(widget.height / 2U);
            const int16_t radiusX = widget.width > 1U ? static_cast<int16_t>((widget.width - 1U) / 2U) : 0;
            const int16_t radiusY = widget.height > 1U ? static_cast<int16_t>((widget.height - 1U) / 2U) : 0;
            if (isFilledShape(widget)) {
                canvas.fillEllipse(cx, cy, radiusX, radiusY, 1);
            } else {
                canvas.drawEllipse(cx, cy, radiusX, radiusY, 1);
            }
        });
    }

    void drawBitmap(const DisplayLayoutWidgetV1& widget) override {
        if (widget.width == 0U || widget.height == 0U) {
            return;
        }
        const std::vector<uint8_t>* bitmapData = ensureDisplayBitmapBytes(widget);
        if (bitmapData == nullptr || bitmapData->empty()) {
            // Missing/unreadable blob (dangling key, blob store unavailable, ...) - draw a
            // recognizable placeholder instead of leaving the widget's area blank or crashing.
            drawMissingBitmapPlaceholder(widget);
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
                if (byteIndex < bitmapData->size() && ((*bitmapData)[byteIndex] & mask) != 0U) {
                    const int16_t x = static_cast<int16_t>(index % widget.width);
                    const int16_t y = static_cast<int16_t>(index / widget.width);
                    canvas.drawPixel(x, y, 1);
                }
            }
            break;
        }
        case DisplayLayoutBitmapFormat::Gray8: {
            for (size_t index = 0U; index < pixelCount && index < bitmapData->size(); ++index) {
                if ((*bitmapData)[index] >= 128U) {
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
                if (byteIndex + 1U >= bitmapData->size()) {
                    break;
                }
                const uint16_t pixel =
                    static_cast<uint16_t>((*bitmapData)[byteIndex]) << 8U | static_cast<uint16_t>((*bitmapData)[byteIndex + 1U]);
                if (pixel != 0U) {
                    const int16_t x = static_cast<int16_t>(index % widget.width);
                    const int16_t y = static_cast<int16_t>(index / widget.width);
                    canvas.drawPixel(x, y, 1);
                }
            }
            break;
        }
        }
        blit(widget, canvas.getBuffer(), SSD1306_WHITE);
    }

private:
    // Rectangle outline + diagonal cross ("broken image" glyph), scaled to the widget's own bounds.
    void drawMissingBitmapPlaceholder(const DisplayLayoutWidgetV1& widget) {
        drawBox(widget, [&widget](GFXcanvas1& canvas) {
            const int16_t w = static_cast<int16_t>(widget.width);
            const int16_t h = static_cast<int16_t>(widget.height);
            canvas.drawRect(0, 0, w, h, 1);
            canvas.drawLine(0, 0, static_cast<int16_t>(w - 1), static_cast<int16_t>(h - 1), 1);
            canvas.drawLine(static_cast<int16_t>(w - 1), 0, 0, static_cast<int16_t>(h - 1), 1);
        });
    }

    template <typename DrawFn> void drawBox(const DisplayLayoutWidgetV1& widget, DrawFn&& drawFn) {
        if (widget.width == 0U || widget.height == 0U) {
            return;
        }
        GFXcanvas1 canvas(widget.width, widget.height);
        canvas.fillScreen(0);
        drawFn(canvas);
        blit(widget, canvas.getBuffer(), effectiveShapeColor(widget));
    }

    uint16_t effectiveShapeColor(const DisplayLayoutWidgetV1& widget) const {
        if ((widget.styleFlags & 0x02U) != 0U) {
            return backgroundOn_ ? SSD1306_WHITE : SSD1306_BLACK;
        }
        return widget.color != 0U ? SSD1306_WHITE : SSD1306_BLACK;
    }

    static bool isFilledShape(const DisplayLayoutWidgetV1& widget) {
        return (widget.styleFlags & 0x01U) != 0U;
    }

    void blit(const DisplayLayoutWidgetV1& widget, const uint8_t* buffer, const uint16_t color) {
        if (buffer == nullptr) {
            return;
        }
        display_.drawBitmap(widget.x, widget.y, buffer, widget.width, widget.height, color);
    }

    void blit(const DisplayLayoutWidgetV1& widget, const uint8_t* buffer, const uint16_t color, const uint16_t backgroundColor) {
        if (buffer == nullptr) {
            return;
        }
        display_.drawBitmap(widget.x, widget.y, buffer, widget.width, widget.height, color, backgroundColor);
    }

    ::Adafruit_SSD1306& display_;
    bool backgroundOn_{false};
};
#endif

static_assert(std::is_trivially_copyable<Ssd1306DeviceConfigV6>::value, "Ssd1306DeviceConfigV6 must be POD");
static_assert(sizeof(Ssd1306DeviceConfigV6::kMagic) - 1U + sizeof(Ssd1306DeviceConfigV6) <= kMaxDeviceConfigBytes,
              "Ssd1306DeviceConfigV6 exceeds device config bound");

Ssd1306Device::Ssd1306Device(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob)
    : Ssd1306DeviceBase(DisplayDeviceBase::initialState()) {
    bindDeviceIdentity(record, configBlob);
    (void)decodeSsd1306DeviceConfig(configBlob.data(), configBlob.size(), config_);
}

Ssd1306Device::~Ssd1306Device() = default;

const Ssd1306DeviceConfigV6& Ssd1306Device::config() const {
    return config_;
}

DisplayLayoutProfile Ssd1306Device::displayProfile() const {
    uint16_t width = config_.width;
    uint16_t height = config_.height;
    if ((config_.rotation % 2U) != 0U) {
        const uint16_t swapped = width;
        width = height;
        height = swapped;
    }
    return pixelDisplayLayoutProfile(width, height, false, true);
}

const DeviceBaseConfigV1& Ssd1306Device::baseConfig() const {
    return config_;
}

bool Ssd1306Device::serializeConfigBlob(DeviceConfigBlob& configBlob) const {
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    const size_t size = ssd1306DeviceConfigSize(config_);
    return encodeFixedConfigBlob(Ssd1306DeviceConfigV6::kMagic, config_, buffer, size) && configBlob.assign(buffer, size);
}

DeviceConfigUpdatePlan Ssd1306Device::planConfigUpdate(const DeviceConfigBlob& configBlob) const {
    Ssd1306DeviceConfigV6 config{};
    if (!decodeSsd1306DeviceConfig(configBlob.data(), configBlob.size(), config)) {
        return {};
    }
    DeviceConfigUpdatePlan plan{};
    plan.endOldConfig = config.i2cAddress != config_.i2cAddress || config.rotation != config_.rotation || config.panel != config_.panel ||
                        config.width != config_.width || config.height != config_.height;
    plan.resetStateMachine = plan.endOldConfig;
    return plan;
}

bool Ssd1306Device::applyConfig(const DeviceConfigBlob& configBlob, uint32_t now) {
    (void)now;
    Ssd1306DeviceConfigV6 config{};
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
    I2cBusDevice* bus = static_cast<I2cBusDevice*>(dependencyRuntime(DeviceRole::I2CBus));
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
        new ::Adafruit_SSD1306(static_cast<uint8_t>(config_.width), static_cast<uint8_t>(config_.height), wire, -1));
    if (display == nullptr) {
        return false;
    }
    if (!display->begin(SSD1306_SWITCHCAPVCC, config_.i2cAddress, true, true)) {
        return false;
    }
    display->setRotation(config_.rotation);
    display->clearDisplay();
    display->display();
    surface_ = std::unique_ptr<Ssd1306CanvasSurface>(new Ssd1306CanvasSurface(*display));
    display_ = std::move(display);
    return true;
#else
    return true;
#endif
}

bool Ssd1306Device::clearDisplay(const uint16_t color) {
    (void)color;
#if defined(ARDUINO) && !defined(UNIT_TEST)
    if (surface_ == nullptr) {
        return false;
    }
    surface_->clear(color);
    return true;
#else
    return true;
#endif
}

DisplayLayoutRenderResult Ssd1306Device::renderDisplayFrame(const MetricValueResolver& resolver, const uint32_t now) {
    (void)resolver;
    (void)now;
#if defined(ARDUINO) && !defined(UNIT_TEST)
    if (surface_ == nullptr) {
        return {};
    }
    PixelDisplayLayoutRenderer renderer(*surface_);
    return renderSession_.render(layout_, resolver, renderer, now);
#else
    return {};
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
        {DeviceRole::I2CBus, true},
        {DeviceRole::MetricSource, false},
    };
    descriptor.createRuntime = &Ssd1306Device::createRuntime;
    descriptor.validateConfig = &Ssd1306Device::validateConfig;
    return descriptor;
}

std::unique_ptr<IDeviceRuntime> Ssd1306Device::createRuntime(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob) {
    return std::unique_ptr<IDeviceRuntime>(new Ssd1306Device(record, configBlob));
}

DeviceValidationResult Ssd1306Device::validateConfig(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob) {
    return validateI2cConfig<Ssd1306DeviceConfigV6>(record, configBlob, decodeSsd1306DeviceConfig);
}

} // namespace ewfm
