#include "devices/display/st7735/St7735Device.h"

#if defined(ARDUINO) && !defined(UNIT_TEST)
#include <Adafruit_ST7735.h>
#include <Arduino.h>
#endif

#include "devices/bus/spi/SpiBusDevice.h"

#include <cstring>
#include <type_traits>

namespace ewfm {

namespace {
constexpr DeviceTypeId kSt7735DeviceTypeId = 9;
constexpr uint32_t kSt7735DeviceConfigVersion = 5;
} // namespace

#if defined(ARDUINO) && !defined(UNIT_TEST)
namespace {
uint8_t st7735InitrOptionForPanel(St7735Panel panel) {
    switch (panel) {
    case St7735Panel::Green18:
        return INITR_GREENTAB;
    case St7735Panel::Green144:
        return INITR_144GREENTAB;
    case St7735Panel::Mini096:
        return INITR_MINI160x80;
    case St7735Panel::Mini096Plugin:
        return INITR_MINI160x80_PLUGIN;
    case St7735Panel::Black18:
        return INITR_BLACKTAB;
    }
    return INITR_BLACKTAB;
}
} // namespace

class St7735CanvasSurface final : public IPixelDisplayRenderSurface {
public:
    explicit St7735CanvasSurface(::Adafruit_ST7735& display) : display_(display) {}

    void clear(const uint16_t color) override {
        backgroundColor_ = color;
        display_.fillScreen(color);
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
        if ((widget.styleFlags & 0x02U) != 0U) {
            blit(widget, canvas.getBuffer(), backgroundColor_, widget.color);
        } else {
            blit(widget, canvas.getBuffer(), widget.color);
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
            drawRgb565Bitmap(widget);
            return;
        }
        }
        blit(widget, canvas.getBuffer(), ST7735_WHITE);
    }

private:
    void drawRgb565Bitmap(const DisplayLayoutWidgetV1& widget) {
        if (widget.width == 0U || widget.height == 0U || widget.bitmapData.empty()) {
            return;
        }
        uint16_t row[255]{};
        for (uint8_t y = 0U; y < widget.height; ++y) {
            for (uint8_t x = 0U; x < widget.width; ++x) {
                const size_t pixelIndex = static_cast<size_t>(y) * static_cast<size_t>(widget.width) + x;
                const size_t byteIndex = pixelIndex * 2U;
                row[x] = byteIndex + 1U < widget.bitmapData.size()
                             ? static_cast<uint16_t>((static_cast<uint16_t>(widget.bitmapData[byteIndex]) << 8U) |
                                                     static_cast<uint16_t>(widget.bitmapData[byteIndex + 1U]))
                             : 0U;
            }
            display_.drawRGBBitmap(static_cast<int16_t>(widget.x), static_cast<int16_t>(widget.y + y), row, widget.width, 1);
        }
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
        return (widget.styleFlags & 0x02U) != 0U ? backgroundColor_ : widget.color;
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

    ::Adafruit_ST7735& display_;
    uint16_t backgroundColor_{ST7735_BLACK};
};
#endif

static_assert(std::is_trivially_copyable<St7735DeviceConfigV5>::value, "St7735DeviceConfigV5 must be POD");
static_assert(sizeof(St7735DeviceConfigV5::kMagic) - 1U + sizeof(St7735DeviceConfigV5) <= kMaxDeviceConfigBytes,
              "St7735DeviceConfigV5 exceeds device config bound");

St7735Device::St7735Device(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob)
    : DisplayDeviceBase(DisplayDeviceBase::initialState()) {
    bindDeviceIdentity(record, configBlob);
    (void)decodeSt7735DeviceConfig(configBlob.data(), configBlob.size(), config_);
}

St7735Device::~St7735Device() = default;

const St7735DeviceConfigV5& St7735Device::config() const {
    return config_;
}

DisplayLayoutProfile St7735Device::displayProfile() const {
    uint16_t width = config_.width;
    uint16_t height = config_.height;
    if ((config_.rotation % 2U) != 0U) {
        const uint16_t swapped = width;
        width = height;
        height = swapped;
    }
    return pixelDisplayLayoutProfile(width, height, true, true);
}

const DeviceBaseConfigV1& St7735Device::baseConfig() const {
    return config_;
}

bool St7735Device::spiChipSelectPin(uint8_t& pin) const {
    pin = config_.chipSelectPin;
    return true;
}

bool St7735Device::serializeConfigBlob(DeviceConfigBlob& configBlob) const {
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    const size_t size = st7735DeviceConfigSize(config_);
    return encodeFixedConfigBlob(St7735DeviceConfigV5::kMagic, config_, buffer, size) && configBlob.assign(buffer, size);
}

DeviceConfigUpdatePlan St7735Device::planConfigUpdate(const DeviceConfigBlob& configBlob) const {
    St7735DeviceConfigV5 config{};
    if (!decodeSt7735DeviceConfig(configBlob.data(), configBlob.size(), config)) {
        return {};
    }
    DeviceConfigUpdatePlan plan{};
    plan.endOldConfig = config.spiBusDeviceId != config_.spiBusDeviceId || config.chipSelectPin != config_.chipSelectPin ||
                        config.dcPin != config_.dcPin || config.resetPin != config_.resetPin || config.rotation != config_.rotation ||
                        config.panel != config_.panel || config.width != config_.width || config.height != config_.height;
    plan.resetStateMachine = plan.endOldConfig;
    return plan;
}

bool St7735Device::applyConfig(const DeviceConfigBlob& configBlob, uint32_t now) {
    (void)now;
    St7735DeviceConfigV5 config{};
    if (!decodeSt7735DeviceConfig(configBlob.data(), configBlob.size(), config)) {
        return false;
    }
    config_ = config;
    return true;
}

void St7735Device::writeDisplayConfigJson(JsonObject output) const {
    config_.writeJson(output);
}

bool St7735Device::initializeDisplayHardware(uint32_t now) {
    (void)now;
#if defined(ARDUINO) && !defined(UNIT_TEST)
    SpiBusDevice* bus = static_cast<SpiBusDevice*>(dependencyRuntime(DeviceRole::SpiBus));
    if (bus == nullptr) {
        return false;
    }
    SpiBusDevice::DependencyTransaction transaction = bus->beginDependencyTransaction();
    if (!transaction) {
        return false;
    }
    ISpiBusDriver* driver = transaction.driver();
    SPIClass* spi = driver != nullptr ? driver->nativeSpi() : nullptr;
    if (spi == nullptr) {
        return false;
    }
    std::unique_ptr<::Adafruit_ST7735> display(
        new ::Adafruit_ST7735(spi, static_cast<int8_t>(config_.chipSelectPin), static_cast<int8_t>(config_.dcPin), config_.resetPin));
    if (display == nullptr) {
        return false;
    }
    display->initR(st7735InitrOptionForPanel(static_cast<St7735Panel>(config_.panel)));
    display->setRotation(config_.rotation);
    display->fillScreen(ST7735_BLACK);
    surface_ = new St7735CanvasSurface(*display);
    display_ = display.release();
    return true;
#else
    return true;
#endif
}

bool St7735Device::clearDisplay(const uint16_t color) {
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

DisplayLayoutRenderResult St7735Device::renderDisplayFrame(const MetricValueResolver& resolver, const uint32_t now) {
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

void St7735Device::releaseDisplayHardware(uint32_t now) {
    (void)now;
#if defined(ARDUINO) && !defined(UNIT_TEST)
    if (surface_ != nullptr) {
        delete surface_;
        surface_ = nullptr;
    }
    if (display_ != nullptr) {
        display_->fillScreen(ST7735_BLACK);
        delete display_;
        display_ = nullptr;
    }
#endif
}

void St7735Device::onDisplayFrameRendered(const DisplayLayoutRenderResult& result) {
    (void)result;
}

DeviceTypeDescriptor St7735Device::descriptor() {
    DeviceTypeDescriptor descriptor;
    descriptor.typeId = kSt7735DeviceTypeId;
    descriptor.name = "St7735Device";
    descriptor.currentConfigVersion = kSt7735DeviceConfigVersion;
    descriptor.maxDependents = 16;
    descriptor.supportsCommands = false;
    descriptor.supportsRetainedState = false;
    descriptor.defaultPersistencePolicy = DevicePersistencePolicy::Delayed;
    descriptor.ticks100ms = true;
    descriptor.dependencyRequirements = {
        {DeviceRole::SpiBus, true},
        {DeviceRole::MetricSource, false},
    };
    descriptor.createRuntime = &St7735Device::createRuntime;
    descriptor.validateConfig = &St7735Device::validateConfig;
    return descriptor;
}

std::unique_ptr<IDeviceRuntime> St7735Device::createRuntime(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob) {
    return std::unique_ptr<IDeviceRuntime>(new St7735Device(record, configBlob));
}

DeviceValidationResult St7735Device::validateConfig(const DeviceRegistryEntry& record, const DeviceConfigBlob& configBlob) {
    (void)record;
    if (configBlob.size() > kMaxDeviceConfigBytes) {
        return {DeviceError::BoundsExceeded, "st7735 config exceeds supported size"};
    }
    St7735DeviceConfigV5 config{};
    if (!decodeSt7735DeviceConfig(configBlob.data(), configBlob.size(), config)) {
        return {DeviceError::InvalidConfig, "st7735 config is invalid"};
    }
    return {};
}

} // namespace ewfm
