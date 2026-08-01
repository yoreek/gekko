// Characterization tests pinning the observable REST-adapter behavior (parsed fields, exact
// error strings, partial-update semantics) before the adapters migrate onto the shared
// TypedDeviceApiAdapter base. Any assertion change here means the refactor changed behavior.
#include "../test_devices/JsonSchemaSmokeValidator.h"
#include "devices/analog/AnalogOutputDeviceBase.h"
#include "devices/analog/adc/IAdcInputDriver.h"
#include "devices/analog/composer/AnalogOutputComposerDevice.h"
#include "devices/analog/fade/FadeAnalogOutputDevice.h"
#include "devices/analog/input/AnalogInputJson.h"
#include "devices/analog/input/ads1115/Ads1115HubDevice.h"
#include "devices/analog/input/cd74hc4067/Cd74hc4067HubDevice.h"
#include "devices/analog/input/channel/AnalogInputChannelDevice.h"
#include "devices/analog/input/channel/AnalogInputChannelDeviceConfig.h"
#include "devices/analog/input/port/AnalogPortInputDevice.h"
#include "devices/analog/input/port/AnalogPortInputDeviceConfig.h"
#include "devices/analog/ledc/LedcAnalogOutputDevice.h"
#include "devices/analog/scheduled/ScheduledAnalogOutputDevice.h"
#include "devices/analog/scheduled/ScheduledAnalogOutputDeviceConfig.h"
#include "devices/bus/i2c/I2cBusConfig.h"
#include "devices/bus/i2c/I2cBusDevice.h"
#include "devices/bus/onewire/OneWireRomAddress.h"
#include "devices/bus/spi/ISpiBusDriver.h"
#include "devices/bus/spi/ISpiCsProbeDriver.h"
#include "devices/bus/spi/SpiBusConfig.h"
#include "devices/bus/spi/SpiBusDevice.h"
#include "devices/display/st7735/St7735Device.h"
#include "devices/display/st7735/St7735DeviceConfig.h"
#include "devices/dosing/DosingPumpDevice.h"
#include "devices/expander/Pcf8574ExpanderDevice.h"
#include "devices/expander/Pcf8575ExpanderDevice.h"
#include "devices/rtc/ds3231/Ds3231RtcDevice.h"
#include "devices/rtc/ds3231/Ds3231RtcDeviceConfig.h"
#include "devices/schedule/ScheduleDevice.h"
#include "devices/sensors/binary/BinarySensorDevice.h"
#include "devices/sensors/ds18b20/Ds18b20TemperatureSensorDevice.h"
#include "devices/sensors/ntc_thermistor/NtcThermistorTemperatureSensorDevice.h"
#include "devices/switch/SwitchOutputState.h"
#include "devices/switch/auto/AutoSwitchDevice.h"
#include "devices/switch/expander/PortExpanderSwitchDevice.h"
#include "devices/switch/gpio/GpioSwitchDevice.h"
#include "devices/switch/gpio/GpioSwitchDeviceConfig.h"
#include "devices/switch/gpio/IGpioOutputDriver.h"
#include "devices/thermostat/ThermostatDevice.h"
#include "integrations/common/DeviceApiAdapter.h"
#include "integrations/rest/analog_input/Ads1115HubDeviceApiAdapter.h"
#include "integrations/rest/analog_input/AnalogInputChannelDeviceApiAdapter.h"
#include "integrations/rest/analog_input/AnalogPortInputDeviceApiAdapter.h"
#include "integrations/rest/analog_input/Cd74hc4067HubDeviceApiAdapter.h"
#include "integrations/rest/analog_output/AnalogOutputComposerDeviceApiAdapter.h"
#include "integrations/rest/analog_output/FadeAnalogOutputDeviceApiAdapter.h"
#include "integrations/rest/analog_output/LedcAnalogOutputDeviceApiAdapter.h"
#include "integrations/rest/analog_output/ScheduledAnalogOutputDeviceApiAdapter.h"
#include "integrations/rest/auto_switch/AutoSwitchDeviceApiAdapter.h"
#include "integrations/rest/binary_sensor/BinarySensorDeviceApiAdapter.h"
#include "integrations/rest/dosing_pump/DosingPumpDeviceApiAdapter.h"
#include "integrations/rest/ds18b20/Ds18b20TemperatureSensorDeviceApiAdapter.h"
#include "integrations/rest/expander/Pcf8574ExpanderDeviceApiAdapter.h"
#include "integrations/rest/expander/Pcf8575ExpanderDeviceApiAdapter.h"
#include "integrations/rest/expander/PortExpanderSwitchDeviceApiAdapter.h"
#include "integrations/rest/gpio_switch/GpioSwitchDeviceApiAdapter.h"
#include "integrations/rest/htu21/Htu21SensorDeviceApiAdapter.h"
#include "integrations/rest/i2c_bus/I2cBusDeviceApiAdapter.h"
#include "integrations/rest/ntc_thermistor/NtcThermistorTemperatureSensorDeviceApiAdapter.h"
#include "integrations/rest/rtc_ds3231/Ds3231RtcDeviceApiAdapter.h"
#include "integrations/rest/schedule/ScheduleDeviceApiAdapter.h"
#include "integrations/rest/spi_bus/SpiBusDeviceApiAdapter.h"
#include "integrations/rest/st7735/St7735DeviceApiAdapter.h"
#include "integrations/rest/thermostat/ThermostatDeviceApiAdapter.h"

#include <ArduinoJson.h>
#include <cstdio>
#include <memory>
#include <string>
#include <unity.h>

using namespace ewfm;

namespace {
void assertMatchesJsonSchema(const char* schemaPath, const JsonVariantConst& value) {
    std::string error;
    TEST_ASSERT_TRUE_MESSAGE(json_schema_smoke::validateFile(schemaPath, value, error), error.c_str());
}

class FakeBinarySensorDriver final : public IGpioInputDriver {
public:
    bool configureInput(uint8_t, GpioInputPullMode) override {
        return true;
    }
    bool read(uint8_t, bool& outLevel) override {
        outLevel = level;
        return true;
    }
    void release(uint8_t) override {}

    bool level{false};
};

class FakeAnalogOutputDevice final : public AnalogOutputDeviceBase, public IScheduledAnalogOutputRuntime {
public:
    explicit FakeAnalogOutputDevice(const char* name) : AnalogOutputDeviceBase(config_) {
        config_.enabled = 1U;
        std::snprintf(config_.name, sizeof(config_.name), "%s", name);
    }

    const IScheduledAnalogOutputRuntime* scheduledAnalogOutputRuntime() const override {
        return this;
    }

    AnalogOutputMode analogOutputMode() const override {
        return mode_;
    }

    bool requestAnalogOutputMode(const AnalogOutputMode mode, uint32_t now) override {
        if (status() != DeviceStatus::Ready) {
            return false;
        }
        mode_ = mode;
        return mode != AnalogOutputMode::Off || requestOutputState(0U, now);
    }

    uint16_t requestedAnalogOutputState() const override {
        return currentOutputState();
    }

    bool analogOutputTimeValid() const override {
        return true;
    }

private:
    const AnalogOutputDeviceConfigV1& config() const override {
        return config_;
    }

    uint16_t invertedState(uint16_t state) const override {
        return state;
    }

    bool parseOutputCommand(const DeviceCommand&, uint16_t&) const override {
        return false;
    }

    bool stateIsValid(uint16_t state) const override {
        return state <= kAnalogOutputLevelMax;
    }

    DeviceValidationResult configureHardware(uint32_t) override {
        return {};
    }

    DeviceValidationResult applyHardwareOutput(uint16_t, uint32_t) override {
        return {};
    }

    void releaseHardware(uint32_t) override {}

    AnalogOutputDeviceConfigV1 config_{};
    AnalogOutputMode mode_{AnalogOutputMode::Scheduled};
};

BinarySensorDeviceConfigV1 makeBinarySensorConfig() {
    BinarySensorDeviceConfigV1 config{};
    config.enabled = true;
    std::snprintf(config.name, sizeof(config.name), "%s", "float switch");
    config.gpioPin = 27U;
    config.pullMode = static_cast<uint8_t>(GpioInputPullMode::PullDown);
    config.inverted = 0U;
    config.debounceMs = 120U;
    return config;
}

AutoSwitchDeviceConfigV1 makeAutoSwitchConfig() {
    AutoSwitchDeviceConfigV1 config{};
    config.enabled = true;
    std::snprintf(config.name, sizeof(config.name), "%s", "auto switch");
    config.pauseDurationSeconds = 900U;
    return config;
}

DeviceConfigBlob encodeAutoSwitchBlob(const AutoSwitchDeviceConfigV1& config) {
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    DeviceConfigBlob blob{};
    TEST_ASSERT_TRUE(encodeFixedConfigBlob(AutoSwitchDeviceConfigV1::kMagic, config, buffer, autoSwitchDeviceConfigSize(config)));
    TEST_ASSERT_TRUE(blob.assign(buffer, autoSwitchDeviceConfigSize(config)));
    return blob;
}

DosingPumpDeviceConfigV1 makeDosingPumpConfig() {
    DosingPumpDeviceConfigV1 config{};
    config.enabled = true;
    std::snprintf(config.name, sizeof(config.name), "%s", "Calcium");
    config.speedMilliMlPerSec = 1250U;
    config.containerCapacityMl = 500U;
    config.thresholdPercent = 15U;
    config.blockAutoWhenEmpty = 1U;
    config.scheduleMode = static_cast<uint8_t>(DosingScheduleMode::Daily);
    config.everyDays = 2U;
    config.anchorDay = 20647U;
    config.doseCount = 1U;
    config.doses[0] = DosingPumpDoseV1{8U * 60U + 30U, 1230U};
    return config;
}

DeviceConfigBlob encodeDosingPumpBlob(const DosingPumpDeviceConfigV1& config) {
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    DeviceConfigBlob blob{};
    TEST_ASSERT_TRUE(encodeFixedConfigBlob(DosingPumpDeviceConfigV1::kMagic, config, buffer, dosingPumpDeviceConfigSize(config)));
    TEST_ASSERT_TRUE(blob.assign(buffer, dosingPumpDeviceConfigSize(config)));
    return blob;
}

ScheduleDeviceConfigV1 makeScheduleConfig() {
    ScheduleDeviceConfigV1 config{};
    config.enabled = true;
    std::snprintf(config.name, sizeof(config.name), "%s", "irrigation");
    config.ruleCount = 1U;
    config.rules[0].enabled = 1U;
    config.rules[0].weekDays = 0x3EU;
    config.rules[0].startMinuteOfDay = 480U;
    config.rules[0].endMinuteOfDay = 1200U;
    config.rules[0].mode = static_cast<uint8_t>(ScheduleRuleMode::Interval);
    config.rules[0].intervalsPerWindow = 3U;
    config.rules[0].durationMinutes = 10U;
    return config;
}

DeviceConfigBlob encodeScheduleBlob(const ScheduleDeviceConfigV1& config) {
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    DeviceConfigBlob blob{};
    TEST_ASSERT_TRUE(encodeFixedConfigBlob(ScheduleDeviceConfigV1::kMagic, config, buffer, scheduleDeviceConfigSize(config)));
    TEST_ASSERT_TRUE(blob.assign(buffer, scheduleDeviceConfigSize(config)));
    return blob;
}

ThermostatDeviceConfigV1 makeThermostatConfig() {
    ThermostatDeviceConfigV1 config{};
    config.enabled = true;
    std::snprintf(config.name, sizeof(config.name), "%s", "Tank");
    config.mode = static_cast<uint8_t>(ThermostatMode::Heat);
    config.algorithm = static_cast<uint8_t>(ThermostatAlgorithm::Hysteresis);
    config.targetMilliCelsius = 25000;
    config.minSafeMilliCelsius = 0;
    config.maxSafeMilliCelsius = 50000;
    config.hysteresisCentiCelsius = 50;
    config.checkIntervalMs = 1000U;
    config.sensorTimeoutMs = 3000U;
    config.retryAfterErrorMs = 30000U;
    config.minSwitchIntervalMs = 5000U;
    return config;
}

DeviceConfigBlob encodeThermostatBlob(const ThermostatDeviceConfigV1& config) {
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    DeviceConfigBlob blob{};
    TEST_ASSERT_TRUE(encodeFixedConfigBlob(ThermostatDeviceConfigV1::kMagic, config, buffer, thermostatDeviceConfigSize(config)));
    TEST_ASSERT_TRUE(blob.assign(buffer, thermostatDeviceConfigSize(config)));
    return blob;
}

Ads1115HubDeviceConfigV1 makeAds1115HubConfig() {
    Ads1115HubDeviceConfigV1 config{};
    config.enabled = true;
    std::snprintf(config.name, sizeof(config.name), "%s", "ADS1115");
    config.i2cAddress = 0x48U;
    config.gain = static_cast<uint8_t>(Ads1115Gain::Fsr4096);
    config.dataRateSps = static_cast<uint8_t>(Ads1115DataRate::Sps250);
    return config;
}

DeviceConfigBlob encodeAds1115HubBlob(const Ads1115HubDeviceConfigV1& config) {
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    DeviceConfigBlob blob{};
    TEST_ASSERT_TRUE(encodeFixedConfigBlob(Ads1115HubDeviceConfigV1::kMagic, config, buffer, ads1115HubDeviceConfigSize(config)));
    TEST_ASSERT_TRUE(blob.assign(buffer, ads1115HubDeviceConfigSize(config)));
    return blob;
}

Cd74hc4067HubDeviceConfigV1 makeCd74hc4067HubConfig() {
    Cd74hc4067HubDeviceConfigV1 config{};
    config.enabled = true;
    std::snprintf(config.name, sizeof(config.name), "%s", "MUX");
    config.selectPins[0] = 16U;
    config.selectPins[1] = 17U;
    config.selectPins[2] = 18U;
    config.selectPins[3] = 19U;
    config.enablePin = kGpioPinUnset;
    config.sigPin = 34U;
    config.sigAttenuation = static_cast<uint8_t>(AdcAttenuation::Db11);
    return config;
}

DeviceConfigBlob encodeCd74hc4067HubBlob(const Cd74hc4067HubDeviceConfigV1& config) {
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    DeviceConfigBlob blob{};
    TEST_ASSERT_TRUE(encodeFixedConfigBlob(Cd74hc4067HubDeviceConfigV1::kMagic, config, buffer, cd74hc4067HubDeviceConfigSize(config)));
    TEST_ASSERT_TRUE(blob.assign(buffer, cd74hc4067HubDeviceConfigSize(config)));
    return blob;
}

Pcf857xExpanderConfigV2 makePcf8574ExpanderConfig() {
    Pcf857xExpanderConfigV2 config{};
    config.enabled = true;
    std::snprintf(config.name, sizeof(config.name), "%s", "PCF8574");
    config.i2cAddress = 0x20U;
    config.inverted = 1U;
    return config;
}

DeviceConfigBlob encodePcf8574ExpanderBlob(const Pcf857xExpanderConfigV2& config) {
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    DeviceConfigBlob blob{};
    TEST_ASSERT_TRUE(encodeFixedConfigBlob(Pcf857xExpanderConfigV2::kMagic, config, buffer, pcf857xExpanderConfigSize(config)));
    TEST_ASSERT_TRUE(blob.assign(buffer, pcf857xExpanderConfigSize(config)));
    return blob;
}

Pcf857xExpanderConfigV2 makePcf8575ExpanderConfig() {
    Pcf857xExpanderConfigV2 config{};
    config.enabled = true;
    std::snprintf(config.name, sizeof(config.name), "%s", "PCF8575");
    config.i2cAddress = 0x20U;
    config.inverted = 0U;
    return config;
}

DeviceConfigBlob encodePcf8575ExpanderBlob(const Pcf857xExpanderConfigV2& config) {
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    DeviceConfigBlob blob{};
    TEST_ASSERT_TRUE(encodeFixedConfigBlob(Pcf857xExpanderConfigV2::kMagic, config, buffer, pcf857xExpanderConfigSize(config)));
    TEST_ASSERT_TRUE(blob.assign(buffer, pcf857xExpanderConfigSize(config)));
    return blob;
}

PortExpanderSwitchDeviceConfigV3 makePortExpanderSwitchConfig() {
    PortExpanderSwitchDeviceConfigV3 config{};
    config.enabled = true;
    std::snprintf(config.name, sizeof(config.name), "%s", "ch5");
    config.restorePreviousState = true;
    config.startupState = true;
    config.safeState = false;
    config.inverted = false;
    config.channel = 5U;
    return config;
}

DeviceConfigBlob encodePortExpanderSwitchBlob(const PortExpanderSwitchDeviceConfigV3& config) {
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    DeviceConfigBlob blob{};
    TEST_ASSERT_TRUE(
        encodeFixedConfigBlob(PortExpanderSwitchDeviceConfigV3::kMagic, config, buffer, portExpanderSwitchDeviceConfigSize(config)));
    TEST_ASSERT_TRUE(blob.assign(buffer, portExpanderSwitchDeviceConfigSize(config)));
    return blob;
}

DeviceRegistryEntry dependentRecord(const DeviceTypeId typeId, const std::initializer_list<DeviceId> dependencies) {
    DeviceRegistryEntry record{};
    record.header.deviceId = typeId + 100U;
    record.header.typeId = typeId;
    for (const DeviceId dependency : dependencies) {
        record.deps[record.depCount++] = DeviceDependencyLink{DeviceRole::AnalogOutput, dependency};
    }
    return record;
}

FadeAnalogOutputDeviceConfigV1 makeFadeAnalogOutputConfig() {
    FadeAnalogOutputDeviceConfigV1 config{};
    config.enabled = true;
    std::snprintf(config.name, sizeof(config.name), "%s", "fade");
    config.maxStep = percentToAnalogOutputState(10U);
    config.stepIntervalMs = 200U;
    return config;
}

DeviceConfigBlob encodeFadeAnalogOutputBlob(const FadeAnalogOutputDeviceConfigV1& config) {
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    DeviceConfigBlob blob{};
    TEST_ASSERT_TRUE(
        encodeFixedConfigBlob(FadeAnalogOutputDeviceConfigV1::kMagic, config, buffer, fadeAnalogOutputDeviceConfigSize(config)));
    TEST_ASSERT_TRUE(blob.assign(buffer, fadeAnalogOutputDeviceConfigSize(config)));
    return blob;
}

ScheduledAnalogOutputDeviceConfigV2 makeScheduledAnalogOutputConfig() {
    ScheduledAnalogOutputDeviceConfigV2 config{};
    config.enabled = true;
    std::snprintf(config.name, sizeof(config.name), "%s", "schedule");
    config.points[0] = ScheduledAnalogOutputPointV1{0U, 480U, percentToAnalogOutputState(100U)};
    config.points[1] = ScheduledAnalogOutputPointV1{0U, 1200U, percentToAnalogOutputState(25U)};
    return config;
}

DeviceConfigBlob encodeScheduledAnalogOutputBlob(const ScheduledAnalogOutputDeviceConfigV2& config) {
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    DeviceConfigBlob blob{};
    TEST_ASSERT_TRUE(
        encodeFixedConfigBlob(ScheduledAnalogOutputDeviceConfigV2::kMagic, config, buffer, scheduledAnalogOutputDeviceConfigSize(config)));
    TEST_ASSERT_TRUE(blob.assign(buffer, scheduledAnalogOutputDeviceConfigSize(config)));
    return blob;
}

Ds18b20TemperatureSensorConfigV2 makeDs18b20Config() {
    Ds18b20TemperatureSensorConfigV2 config{};
    config.enabled = true;
    std::snprintf(config.name, sizeof(config.name), "%s", "temperature");
    TEST_ASSERT_TRUE(parseOneWireRomAddress("28FF641D621603AD", config.address));
    config.resolution = 11U;
    config.outputUnit = temperatureUnitToByte(TemperatureUnit::Fahrenheit);
    config.reportAlways = 1U;
    config.reportDeltaCentiCelsius = 25U;
    config.pollMs = 2000U;
    return config;
}

DeviceConfigBlob encodeDs18b20Blob(const Ds18b20TemperatureSensorConfigV2& config) {
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    DeviceConfigBlob blob{};
    TEST_ASSERT_TRUE(
        encodeFixedConfigBlob(Ds18b20TemperatureSensorConfigV2::kMagic, config, buffer, ds18b20TemperatureSensorConfigSize(config)));
    TEST_ASSERT_TRUE(blob.assign(buffer, ds18b20TemperatureSensorConfigSize(config)));
    return blob;
}

Htu21SensorConfigV3 makeHtu21Config(uint8_t i2cAddress = kHtu21DefaultI2cAddress) {
    Htu21SensorConfigV3 config{};
    config.enabled = true;
    std::snprintf(config.name, sizeof(config.name), "%s", "climate");
    config.i2cAddress = i2cAddress;
    return config;
}

DeviceConfigBlob encodeHtu21Blob(const Htu21SensorConfigV3& config) {
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    DeviceConfigBlob blob{};
    TEST_ASSERT_TRUE(encodeFixedConfigBlob(Htu21SensorConfigV3::kMagic, config, buffer, htu21SensorConfigSize(config)));
    TEST_ASSERT_TRUE(blob.assign(buffer, htu21SensorConfigSize(config)));
    return blob;
}

class FakeGpioOutputDriver final : public IGpioOutputDriver {
public:
    bool configureOutput(uint8_t pin, bool level) override {
        lastPin = pin;
        lastLevel = level;
        return true;
    }

    bool write(uint8_t pin, bool level) override {
        lastPin = pin;
        lastLevel = level;
        return true;
    }

    void release(uint8_t pin) override {
        lastPin = pin;
    }

    uint8_t lastPin{0};
    bool lastLevel{false};
};

GpioSwitchDeviceConfigV3 makeGpioSwitchConfig() {
    GpioSwitchDeviceConfigV3 config{};
    config.enabled = true;
    std::snprintf(config.name, sizeof(config.name), "%s", "relay");
    config.restorePreviousState = true;
    config.startupState = kSwitchOutputOn;
    config.safeState = kSwitchOutputOff;
    config.inverted = true;
    config.gpioPin = 21U;
    return config;
}

DeviceConfigBlob encodeGpioSwitchBlob(const GpioSwitchDeviceConfigV3& config) {
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    DeviceConfigBlob blob{};
    TEST_ASSERT_TRUE(encodeFixedConfigBlob(GpioSwitchDeviceConfigV3::kMagic, config, buffer, gpioSwitchDeviceConfigSize(config)));
    TEST_ASSERT_TRUE(blob.assign(buffer, gpioSwitchDeviceConfigSize(config)));
    return blob;
}

std::unique_ptr<GpioSwitchDevice> makeGpioSwitchRuntime(FakeGpioOutputDriver& driver) {
    const DeviceRegistryEntry record = [] {
        DeviceRegistryEntry record{};
        record.header.deviceId = 7U;
        record.header.typeId = GpioSwitchDevice::descriptor().typeId;
        record.header.configVersion = GpioSwitchDevice::descriptor().currentConfigVersion;
        record.header.configRevision = 3U;
        record.header.payloadLength = static_cast<uint32_t>(gpioSwitchDeviceConfigSize(makeGpioSwitchConfig()));
        record.status = DeviceStatus::Ready;
        return record;
    }();
    const DeviceConfigBlob configBlob = encodeGpioSwitchBlob(makeGpioSwitchConfig());
    auto runtime = std::unique_ptr<GpioSwitchDevice>(new GpioSwitchDevice(makeGpioSwitchConfig(), driver));
    runtime->bindDeviceIdentity(record, configBlob);
    runtime->begin(0U);
    runtime->tickFastLoop(1U);
    return runtime;
}

class FakeAdcInputDriver final : public IAdcInputDriver {
public:
    bool configurePin(uint8_t pin, AdcAttenuation attenuation) override {
        lastPin = pin;
        lastAttenuation = attenuation;
        ++configureCalls;
        return configureOk;
    }

    uint32_t readMilliVolts(uint8_t) override {
        return milliVolts;
    }

    void release(uint8_t) override {
        released = true;
    }

    bool configureOk{true};
    uint8_t lastPin{0};
    AdcAttenuation lastAttenuation{AdcAttenuation::Db11};
    int configureCalls{0};
    uint32_t milliVolts{1650U};
    bool released{false};
};

class FakeAnalogInputHubRuntime final : public IDeviceRuntime, public IAnalogInputHubRuntime {
public:
    FakeAnalogInputHubRuntime() {
        reading.rawCode = 1234U;
        reading.milliVolts = 1660;
        reading.measuredAtMs = 77U;
        reading.valid = true;
    }

    void begin(uint32_t) override {}
    void tickFastLoop(uint32_t) override {}
    void tick100ms(uint32_t) override {}
    void tick1s(uint32_t) override {}
    void requestReconfigure() override {}
    void requestDisable() override {}
    void requestDelete() override {}
    DeviceStatus status() const override {
        return DeviceStatus::Ready;
    }
    bool handleCommand(const DeviceCommand&) override {
        return false;
    }
    DeviceId deviceId() const override {
        return deviceId_;
    }
    DeviceTypeId typeId() const override {
        return typeId_;
    }
    const char* name() const override {
        return name_;
    }
    const IAnalogInputHubRuntime* analogInputHubRuntime() const override {
        return this;
    }

    uint8_t channelCount() const override {
        return 16U;
    }
    uint32_t generation() const override {
        return generation_;
    }
    AnalogInputHubPollResult pollChannelReading(uint8_t channel, DeviceId requester, uint32_t now, AnalogInputReading& outReading,
                                                const char*& outStatus) override {
        lastChannel = channel;
        lastRequester = requester;
        lastNow = now;
        outReading = reading;
        outReading.valid = true;
        outStatus = "ok";
        return AnalogInputHubPollResult::Ready;
    }
    void releaseChannelRequest(uint8_t, DeviceId) override {}

    DeviceId deviceId_{0};
    DeviceTypeId typeId_{0};
    const char* name_{"analog hub"};
    uint32_t generation_{1U};
    uint8_t lastChannel{0};
    DeviceId lastRequester{0};
    uint32_t lastNow{0};
    AnalogInputReading reading{};
};

class FakeI2cBusDriver final : public II2cBusDriver {
public:
    bool begin(uint8_t sdaPin, uint8_t sclPin, uint32_t frequencyHz, bool internalPullup) override {
        lastSdaPin = sdaPin;
        lastSclPin = sclPin;
        lastFrequencyHz = frequencyHz;
        lastInternalPullup = internalPullup;
        ++beginCalls;
        return beginOk;
    }

    bool end() override {
        ended = true;
        return true;
    }

    bool setClock(uint32_t frequencyHz) override {
        lastClockHz = frequencyHz;
        ++setClockCalls;
        return setClockOk;
    }

    uint32_t getClock() const override {
        return lastClockHz;
    }

    void beginTransmission(uint8_t address) override {
        lastAddress = address;
    }

    uint8_t endTransmission(bool) override {
        return 0U;
    }

    size_t requestFrom(uint8_t, size_t size, bool) override {
        return size;
    }

    size_t write(uint8_t) override {
        return 1U;
    }

    size_t write(const uint8_t*, size_t quantity) override {
        return quantity;
    }

    int available() override {
        return 0;
    }

    int read() override {
        return 0;
    }

    void flush() override {}

    bool beginOk{true};
    bool setClockOk{true};
    bool ended{false};
    uint8_t lastSdaPin{0};
    uint8_t lastSclPin{0};
    uint32_t lastFrequencyHz{0};
    bool lastInternalPullup{false};
    uint32_t lastClockHz{0};
    uint8_t lastAddress{0};
    int beginCalls{0};
    int setClockCalls{0};
};

class FakeDs3231I2cBusDriver final : public II2cBusDriver {
public:
    bool begin(uint8_t, uint8_t, uint32_t, bool) override {
        return true;
    }

    bool end() override {
        return true;
    }

    bool setClock(uint32_t) override {
        return true;
    }

    uint32_t getClock() const override {
        return 0U;
    }

    void beginTransmission(uint8_t address) override {
        currentAddress = address;
        txCount = 0;
    }

    uint8_t endTransmission(bool) override {
        if (txCount == 1) {
            selectedRegister = txBytes[0];
        }
        return 0U;
    }

    size_t requestFrom(uint8_t, size_t size, bool) override {
        rxCount = 0;
        rxIndex = 0;
        if (selectedRegister == 0x00U && size == 7U) {
            rxBytes[0] = 0x05U; // 05s
            rxBytes[1] = 0x04U; // 04m
            rxBytes[2] = 0x03U; // 03h
            rxBytes[3] = 0x02U; // weekday
            rxBytes[4] = 0x02U; // day
            rxBytes[5] = 0x01U; // month
            rxBytes[6] = 0x24U; // 2024
            rxCount = 7U;
            return size;
        }
        if (selectedRegister == 0x0FU && size == 1U) {
            rxBytes[0] = 0x00U;
            rxCount = 1U;
            return size;
        }
        return 0U;
    }

    size_t write(uint8_t data) override {
        if (txCount < sizeof(txBytes)) {
            txBytes[txCount++] = data;
        }
        return 1U;
    }

    size_t write(const uint8_t* data, size_t quantity) override {
        for (size_t index = 0; index < quantity && txCount < sizeof(txBytes); ++index) {
            txBytes[txCount++] = data[index];
        }
        return quantity;
    }

    int available() override {
        return static_cast<int>(rxCount - rxIndex);
    }

    int read() override {
        if (rxIndex >= rxCount) {
            return 0;
        }
        return rxBytes[rxIndex++];
    }

    void flush() override {}

private:
    uint8_t currentAddress{0};
    uint8_t selectedRegister{0};
    uint8_t txBytes[8]{};
    size_t txCount{0};
    uint8_t rxBytes[8]{};
    size_t rxCount{0};
    size_t rxIndex{0};
};

class FakeSpiBusDriver final : public ISpiBusDriver {
public:
    bool begin(uint8_t host, uint8_t sckPin, uint8_t mosiPin, uint8_t misoPin) override {
        lastHost = host;
        lastSckPin = sckPin;
        lastMosiPin = mosiPin;
        lastMisoPin = misoPin;
        ++beginCalls;
        return beginOk;
    }

    bool end() override {
        ended = true;
        return true;
    }

    bool beginTransaction(uint32_t clockHz, uint8_t dataMode, uint8_t bitOrder) override {
        lastClockHz = clockHz;
        lastDataMode = dataMode;
        lastBitOrder = bitOrder;
        transactionActive = true;
        return transactionOk;
    }

    void endTransaction() override {
        transactionActive = false;
    }

    uint8_t transfer(uint8_t data) override {
        return data;
    }

    size_t transferBytes(const uint8_t*, uint8_t* rxData, size_t length) override {
        if (rxData != nullptr) {
            for (size_t index = 0; index < length; ++index) {
                rxData[index] = 0xFFU;
            }
        }
        return length;
    }

    bool beginOk{true};
    bool transactionOk{true};
    bool ended{false};
    bool transactionActive{false};
    uint8_t lastHost{0};
    uint8_t lastSckPin{0};
    uint8_t lastMosiPin{0};
    uint8_t lastMisoPin{kGpioPinUnset};
    uint32_t lastClockHz{0};
    uint8_t lastDataMode{0};
    uint8_t lastBitOrder{0};
    int beginCalls{0};
};

class FakeSpiCsProbeDriver final : public ISpiCsProbeDriver {
public:
    bool readCurrentState(uint8_t pin, GpioMode& mode, bool& level) override {
        lastPin = pin;
        mode = GpioMode::Input;
        level = false;
        return true;
    }

    bool configureOutput(uint8_t pin, bool initialLevel) override {
        lastPin = pin;
        lastLevel = initialLevel;
        return true;
    }

    bool configureInputPullup(uint8_t pin, bool& level) override {
        lastPin = pin;
        level = true;
        return true;
    }

    bool configureInputPulldown(uint8_t pin, bool& level) override {
        lastPin = pin;
        level = false;
        return true;
    }

    bool writeLevel(uint8_t pin, bool high) override {
        lastPin = pin;
        lastLevel = high;
        return true;
    }

    bool readLevel(uint8_t pin, bool& level) override {
        lastPin = pin;
        level = lastLevel;
        return true;
    }

    bool restoreState(uint8_t pin, GpioMode mode, bool level) override {
        lastPin = pin;
        lastMode = mode;
        lastLevel = level;
        return true;
    }

    bool release(uint8_t pin) override {
        lastPin = pin;
        return true;
    }

    uint8_t lastPin{0};
    GpioMode lastMode{GpioMode::Input};
    bool lastLevel{false};
};

AnalogPortInputDeviceConfigV1 makeAnalogPortInputConfig() {
    AnalogPortInputDeviceConfigV1 config{};
    config.enabled = true;
    std::snprintf(config.name, sizeof(config.name), "%s", "analog port");
    config.gpioPin = 34U;
    config.attenuation = static_cast<uint8_t>(AdcAttenuation::Db11);
    config.poll.adcSamples = 4U;
    config.poll.reportAlways = 0U;
    config.poll.reportDeltaMilliVolts = 10U;
    config.poll.pollMs = 100U;
    return config;
}

DeviceConfigBlob encodeAnalogPortInputBlob(const AnalogPortInputDeviceConfigV1& config) {
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    DeviceConfigBlob blob{};
    TEST_ASSERT_TRUE(encodeFixedConfigBlob(AnalogPortInputDeviceConfigV1::kMagic, config, buffer, analogPortInputDeviceConfigSize(config)));
    TEST_ASSERT_TRUE(blob.assign(buffer, analogPortInputDeviceConfigSize(config)));
    return blob;
}

void bindAnalogPortInputIdentity(AnalogPortInputDevice& device, DeviceId deviceId) {
    DeviceRegistryEntry record{};
    record.header.deviceId = deviceId;
    record.header.typeId = AnalogPortInputDevice::descriptor().typeId;
    record.header.configVersion = AnalogPortInputDevice::descriptor().currentConfigVersion;
    record.header.configRevision = 1U;
    record.header.payloadLength = static_cast<uint32_t>(encodeAnalogPortInputBlob(device.config()).size());
    record.status = DeviceStatus::Ready;
    device.bindDeviceIdentity(record, encodeAnalogPortInputBlob(device.config()));
}

void driveAnalogPortInputUntilReading(AnalogPortInputDevice& device, uint32_t startNow = 10U) {
    device.begin(startNow);
    for (uint32_t now = startNow + 1U; now < startNow + 40U && !device.reading().valid; ++now) {
        device.tick100ms(now);
    }
}

AnalogInputChannelDeviceConfigV1 makeAnalogInputChannelConfig() {
    AnalogInputChannelDeviceConfigV1 config{};
    config.enabled = true;
    std::snprintf(config.name, sizeof(config.name), "%s", "analog channel");
    config.channel = 3U;
    config.poll.adcSamples = 4U;
    config.poll.reportAlways = 0U;
    config.poll.reportDeltaMilliVolts = 15U;
    config.poll.pollMs = 500U;
    return config;
}

DeviceConfigBlob encodeAnalogInputChannelBlob(const AnalogInputChannelDeviceConfigV1& config) {
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    DeviceConfigBlob blob{};
    TEST_ASSERT_TRUE(
        encodeFixedConfigBlob(AnalogInputChannelDeviceConfigV1::kMagic, config, buffer, analogInputChannelDeviceConfigSize(config)));
    TEST_ASSERT_TRUE(blob.assign(buffer, analogInputChannelDeviceConfigSize(config)));
    return blob;
}

void bindAnalogInputChannelIdentity(AnalogInputChannelDevice& device, DeviceId deviceId, DeviceId hubDeviceId) {
    DeviceRegistryEntry record{};
    record.header.deviceId = deviceId;
    record.header.typeId = AnalogInputChannelDevice::descriptor().typeId;
    record.header.configVersion = AnalogInputChannelDevice::descriptor().currentConfigVersion;
    record.header.configRevision = 1U;
    record.header.payloadLength = static_cast<uint32_t>(encodeAnalogInputChannelBlob(device.config()).size());
    record.depCount = 1U;
    record.deps[0] = {DeviceRole::AnalogInputHub, hubDeviceId};
    record.status = DeviceStatus::Ready;
    device.bindDeviceIdentity(record, encodeAnalogInputChannelBlob(device.config()));
}

void driveAnalogInputChannelUntilReading(AnalogInputChannelDevice& device, FakeAnalogInputHubRuntime& hub, uint32_t startNow = 10U) {
    device.setDependencyRuntime(DeviceRole::AnalogInputHub, &hub);
    device.begin(startNow);
    for (uint32_t now = startNow + 1U; now < startNow + 20U && !device.reading().valid; ++now) {
        device.tick100ms(now);
    }
}

I2cBusDeviceConfigV1 makeI2cBusConfig() {
    I2cBusDeviceConfigV1 config{};
    config.enabled = true;
    std::snprintf(config.name, sizeof(config.name), "%s", "i2c bus");
    config.sdaPin = 21U;
    config.sclPin = 22U;
    config.internalPullup = 1U;
    config.frequencyHz = 400000U;
    return config;
}

DeviceConfigBlob encodeI2cBusBlob(const I2cBusDeviceConfigV1& config) {
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    DeviceConfigBlob blob{};
    TEST_ASSERT_TRUE(encodeFixedConfigBlob(I2cBusDeviceConfigV1::kMagic, config, buffer, i2cBusDeviceConfigSize(config)));
    TEST_ASSERT_TRUE(blob.assign(buffer, i2cBusDeviceConfigSize(config)));
    return blob;
}

void bindI2cBusIdentity(I2cBusDevice& device, DeviceId deviceId) {
    DeviceRegistryEntry record{};
    record.header.deviceId = deviceId;
    record.header.typeId = I2cBusDevice::descriptor().typeId;
    record.header.configVersion = I2cBusDevice::descriptor().currentConfigVersion;
    record.header.configRevision = 1U;
    record.header.payloadLength = static_cast<uint32_t>(encodeI2cBusBlob(device.config()).size());
    record.status = DeviceStatus::Ready;
    device.bindDeviceIdentity(record, encodeI2cBusBlob(device.config()));
}

void driveI2cBusToReady(I2cBusDevice& device, uint32_t startNow = 10U) {
    device.begin(startNow);
    for (uint32_t now = startNow + 1U; now < startNow + 20U && device.status() != DeviceStatus::Ready; ++now) {
        device.tick100ms(now);
    }
}

Ds3231RtcDeviceConfigV2 makeRtcConfig() {
    Ds3231RtcDeviceConfigV2 config{};
    config.enabled = true;
    std::snprintf(config.name, sizeof(config.name), "%s", "rtc");
    config.i2cAddress = 0x68U;
    config.useForSystemTimeSync = 1U;
    return config;
}

DeviceConfigBlob encodeRtcBlob(const Ds3231RtcDeviceConfigV2& config) {
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    DeviceConfigBlob blob{};
    TEST_ASSERT_TRUE(encodeFixedConfigBlob(Ds3231RtcDeviceConfigV2::kMagic, config, buffer, ds3231RtcDeviceConfigSize(config)));
    TEST_ASSERT_TRUE(blob.assign(buffer, ds3231RtcDeviceConfigSize(config)));
    return blob;
}

void bindRtcIdentity(Ds3231RtcDevice& device, DeviceId deviceId, DeviceId busDeviceId) {
    DeviceRegistryEntry record{};
    record.header.deviceId = deviceId;
    record.header.typeId = Ds3231RtcDevice::descriptor().typeId;
    record.header.configVersion = Ds3231RtcDevice::descriptor().currentConfigVersion;
    record.header.configRevision = 1U;
    record.header.payloadLength = static_cast<uint32_t>(encodeRtcBlob(device.config()).size());
    record.depCount = 1U;
    record.deps[0] = {DeviceRole::I2CBus, busDeviceId};
    record.status = DeviceStatus::Ready;
    device.bindDeviceIdentity(record, encodeRtcBlob(device.config()));
}

void driveRtcUntilReading(Ds3231RtcDevice& device, I2cBusDevice& bus, uint32_t startNow = 10U) {
    device.setDependencyRuntime(DeviceRole::I2CBus, &bus);
    device.begin(startNow);
    for (uint32_t now = startNow + 1U; now < startNow + 30U && !device.lastReadOk(); ++now) {
        device.tick100ms(now);
    }
}

SpiBusDeviceConfigV2 makeSpiBusConfig() {
    SpiBusDeviceConfigV2 config{};
    config.enabled = true;
    std::snprintf(config.name, sizeof(config.name), "%s", "spi bus");
    config.host = kSpiBusHostVspi;
    config.sckPin = 18U;
    config.mosiPin = 23U;
    config.misoPin = kGpioPinUnset;
    return config;
}

DeviceConfigBlob encodeSpiBusBlob(const SpiBusDeviceConfigV2& config) {
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    DeviceConfigBlob blob{};
    TEST_ASSERT_TRUE(encodeFixedConfigBlob(SpiBusDeviceConfigV2::kMagic, config, buffer, spiBusDeviceConfigSize(config)));
    TEST_ASSERT_TRUE(blob.assign(buffer, spiBusDeviceConfigSize(config)));
    return blob;
}

void bindSpiBusIdentity(SpiBusDevice& device, DeviceId deviceId) {
    DeviceRegistryEntry record{};
    record.header.deviceId = deviceId;
    record.header.typeId = SpiBusDevice::descriptor().typeId;
    record.header.configVersion = SpiBusDevice::descriptor().currentConfigVersion;
    record.header.configRevision = 1U;
    record.header.payloadLength = static_cast<uint32_t>(encodeSpiBusBlob(device.config()).size());
    record.status = DeviceStatus::Ready;
    device.bindDeviceIdentity(record, encodeSpiBusBlob(device.config()));
}

void driveSpiBusToReady(SpiBusDevice& device, uint32_t startNow = 10U) {
    device.begin(startNow);
    for (uint32_t now = startNow + 1U; now < startNow + 20U && device.status() != DeviceStatus::Ready; ++now) {
        device.tick100ms(now);
    }
}

St7735DeviceConfigV6 makeSt7735Config() {
    St7735DeviceConfigV6 config{};
    config.enabled = true;
    std::snprintf(config.name, sizeof(config.name), "%s", "display");
    config.spiBusDeviceId = 11U;
    config.chipSelectPin = 5U;
    config.dcPin = 2U;
    config.resetPin = kGpioPinUnset;
    config.rotation = 0U;
    config.panel = static_cast<uint8_t>(St7735Panel::Black18);
    config.width = 128U;
    config.height = 160U;
    return config;
}

DeviceConfigBlob encodeSt7735Blob(const St7735DeviceConfigV6& config) {
    uint8_t buffer[kMaxDeviceConfigBytes]{};
    DeviceConfigBlob blob{};
    TEST_ASSERT_TRUE(encodeFixedConfigBlob(St7735DeviceConfigV6::kMagic, config, buffer, st7735DeviceConfigSize(config)));
    TEST_ASSERT_TRUE(blob.assign(buffer, st7735DeviceConfigSize(config)));
    return blob;
}

DeviceRegistryEntry makeSt7735Record(DeviceId deviceId, DeviceId busDeviceId, const St7735DeviceConfigV6& config) {
    DeviceRegistryEntry record{};
    record.header.deviceId = deviceId;
    record.header.typeId = St7735Device::descriptor().typeId;
    record.header.configVersion = St7735Device::descriptor().currentConfigVersion;
    record.header.configRevision = 1U;
    record.header.payloadLength = static_cast<uint32_t>(encodeSt7735Blob(config).size());
    record.depCount = 1U;
    record.deps[0] = {DeviceRole::SpiBus, busDeviceId};
    record.status = DeviceStatus::Ready;
    return record;
}

} // namespace

void test_ledc_analog_output_api_adapter_parses_single_output_config() {
    StaticJsonDocument<512> doc;
    doc["typeName"] = "analog_output";
    JsonObject config = doc.createNestedObject("config");
    config["name"] = "dimmable output";
    config["enabled"] = true;
    config["restorePreviousState"] = true;
    config["startupState"] = 35;
    config["safeState"] = 10;
    config["pin"] = 13;
    config["ledcChannel"] = 2;
    config["frequencyHz"] = 5000;
    config["dutyBits"] = 12;
    config["inverted"] = false;

    DeviceCreateRequest request{};
    const char* error = nullptr;
    TEST_ASSERT_TRUE(LedcAnalogOutputDeviceApiAdapter::instance().parseCreateRequest(doc.as<JsonObjectConst>(), request, error));
    TEST_ASSERT_EQUAL_UINT32(kLedcAnalogOutputDeviceTypeId, request.typeId);
    TEST_ASSERT_EQUAL_UINT32(kLedcAnalogOutputDeviceConfigVersion, request.configVersion);

    LedcAnalogOutputDeviceConfigV1 parsed{};
    TEST_ASSERT_TRUE(decodeLedcAnalogOutputDeviceConfig(request.configBlob.data(), request.configBlob.size(), parsed));
    TEST_ASSERT_EQUAL_UINT16(percentToAnalogOutputState(35U), parsed.startupState);
    TEST_ASSERT_EQUAL_UINT16(percentToAnalogOutputState(10U), parsed.safeState);
    TEST_ASSERT_EQUAL_UINT8(13U, parsed.pin);
    TEST_ASSERT_EQUAL_UINT8(2U, parsed.ledcChannel);
    TEST_ASSERT_TRUE(parsed.restorePreviousState);
}

void test_ledc_analog_output_api_adapter_schema_smoke() {
    StaticJsonDocument<512> createDoc;
    createDoc["typeName"] = "analog_output";
    JsonObject createConfig = createDoc.createNestedObject("config");
    createConfig["name"] = "dimmable output";
    createConfig["enabled"] = true;
    createConfig["restorePreviousState"] = true;
    createConfig["startupState"] = 35;
    createConfig["safeState"] = 10;
    createConfig["pin"] = 13;
    createConfig["ledcChannel"] = 2;
    createConfig["frequencyHz"] = 5000;
    createConfig["dutyBits"] = 12;
    createConfig["inverted"] = false;
    assertMatchesJsonSchema("schemas/rest/v1/requests/devices-create-ledc_analog_output.request.schema.json",
                            createDoc.as<JsonVariantConst>());

    StaticJsonDocument<256> updateDoc;
    JsonObject updateConfig = updateDoc.createNestedObject("config");
    updateConfig["startupState"] = 40;
    updateConfig["pin"] = 12;
    assertMatchesJsonSchema("schemas/rest/v1/requests/devices-update-ledc_analog_output.request.schema.json",
                            updateDoc.as<JsonVariantConst>());
}

void test_analog_output_composer_api_adapter_parses_create_request() {
    StaticJsonDocument<512> doc;
    doc["typeName"] = "analog_output_composer";
    JsonObject config = doc.createNestedObject("config");
    config["name"] = "group mode";
    config["enabled"] = true;
    JsonArray deps = config.createNestedArray("deps");
    JsonObject firstDep = deps.createNestedObject();
    firstDep["role"] = "analog_output";
    firstDep["deviceId"] = 41;
    JsonObject secondDep = deps.createNestedObject();
    secondDep["role"] = "analog_output";
    secondDep["deviceId"] = 42;

    assertMatchesJsonSchema("schemas/rest/v1/requests/devices-create-analog_output_composer.request.schema.json",
                            doc.as<JsonVariantConst>());

    DeviceCreateRequest request{};
    const char* error = nullptr;
    TEST_ASSERT_TRUE(AnalogOutputComposerDeviceApiAdapter::instance().parseCreateRequest(doc.as<JsonObjectConst>(), request, error));
    TEST_ASSERT_EQUAL_UINT32(23U, request.typeId);
    TEST_ASSERT_EQUAL_UINT32(1U, request.configVersion);
    TEST_ASSERT_EQUAL_STRING("group mode", request.baseConfig.name);
    TEST_ASSERT_TRUE(request.isEnabled());
    TEST_ASSERT_EQUAL_UINT8(2U, request.dependencyCount());
    TEST_ASSERT_EQUAL_UINT32(41U, request.dependencyLinks()[0].deviceId);
    TEST_ASSERT_EQUAL_UINT32(42U, request.dependencyLinks()[1].deviceId);

    AnalogOutputComposerDeviceConfigV1 parsed{};
    TEST_ASSERT_TRUE(decodeAnalogOutputComposerDeviceConfig(reinterpret_cast<const uint8_t*>(request.configBlob.data()),
                                                            request.configBlob.size(), parsed));
    TEST_ASSERT_TRUE(parsed.enabled);
}

void test_analog_output_composer_api_adapter_partial_update_preserves_deps() {
    AnalogOutputComposerDeviceConfigV1 current{};
    current.enabled = true;
    std::snprintf(current.name, sizeof(current.name), "%s", "group mode");
    AnalogOutputComposerDevice runtime(current);

    StaticJsonDocument<256> doc;
    JsonObject config = doc.createNestedObject("config");
    config["name"] = "group mode 2";

    assertMatchesJsonSchema("schemas/rest/v1/requests/devices-update-analog_output_composer.request.schema.json",
                            doc.as<JsonVariantConst>());

    DeviceConfigUpdateRequest request{};
    const char* error = nullptr;
    TEST_ASSERT_TRUE(
        AnalogOutputComposerDeviceApiAdapter::instance().parseUpdateConfigRequest(doc.as<JsonObjectConst>(), runtime, request, error));
    TEST_ASSERT_EQUAL_UINT32(1U, request.configVersion);
    TEST_ASSERT_FALSE(request.depsProvided);

    AnalogOutputComposerDeviceConfigV1 parsed{};
    TEST_ASSERT_TRUE(decodeAnalogOutputComposerDeviceConfig(reinterpret_cast<const uint8_t*>(request.configBlob.data()),
                                                            request.configBlob.size(), parsed));
    TEST_ASSERT_EQUAL_STRING("group mode 2", parsed.name);
}

void test_analog_output_composer_api_adapter_serializes_record() {
    AnalogOutputComposerDeviceConfigV1 config{};
    config.enabled = true;
    std::snprintf(config.name, sizeof(config.name), "%s", "group mode");
    const DeviceConfigBlob blob = [&]() {
        uint8_t buffer[kMaxDeviceConfigBytes]{};
        DeviceConfigBlob out{};
        TEST_ASSERT_TRUE(encodeFixedConfigBlob(AnalogOutputComposerDeviceConfigV1::kMagic, config, buffer,
                                               analogOutputComposerDeviceConfigSize(config)));
        TEST_ASSERT_TRUE(out.assign(buffer, analogOutputComposerDeviceConfigSize(config)));
        return out;
    }();
    DeviceRegistryEntry record{};
    record.header.deviceId = 70U;
    record.header.typeId = 23U;
    record.header.configVersion = 1U;
    record.header.payloadLength = static_cast<uint32_t>(blob.size());

    AnalogOutputComposerDevice device(config);
    device.bindDeviceIdentity(record, blob);

    StaticJsonDocument<1024> doc;
    JsonObject output = doc.to<JsonObject>();
    AnalogOutputComposerDeviceApiAdapter::instance().writeDeviceJson(device, device.status(), output);

    TEST_ASSERT_FALSE(doc.overflowed());
    assertMatchesJsonSchema("schemas/rest/v1/responses/devices-analog_output_composer.response.schema.json", doc.as<JsonVariantConst>());
    TEST_ASSERT_EQUAL_UINT32(70U, output["record"]["id"].as<uint32_t>());
    TEST_ASSERT_EQUAL_STRING("analog_output_composer", output["record"]["typeName"].as<const char*>());
    TEST_ASSERT_EQUAL_STRING("group mode", output["config"]["name"].as<const char*>());
    TEST_ASSERT_EQUAL_STRING("scheduled", output["runtime"]["output"]["mode"].as<const char*>());
}

void test_fade_analog_output_api_adapter_parses_create_request() {
    StaticJsonDocument<512> doc;
    doc["typeName"] = "fade_analog_output";
    JsonObject config = doc.createNestedObject("config");
    config["name"] = "fade";
    config["enabled"] = true;
    config["maxStep"] = 10;
    config["stepIntervalMs"] = 200;
    JsonArray deps = config.createNestedArray("deps");
    JsonObject outputDep = deps.createNestedObject();
    outputDep["role"] = "analog_output";
    outputDep["deviceId"] = 51;

    assertMatchesJsonSchema("schemas/rest/v1/requests/devices-create-fade_analog_output.request.schema.json", doc.as<JsonVariantConst>());

    DeviceCreateRequest request{};
    const char* error = nullptr;
    TEST_ASSERT_TRUE(FadeAnalogOutputDeviceApiAdapter::instance().parseCreateRequest(doc.as<JsonObjectConst>(), request, error));
    TEST_ASSERT_EQUAL_UINT32(kFadeAnalogOutputDeviceTypeId, request.typeId);
    TEST_ASSERT_EQUAL_UINT32(kFadeAnalogOutputDeviceConfigVersion, request.configVersion);
    TEST_ASSERT_EQUAL_STRING("fade", request.baseConfig.name);
    TEST_ASSERT_TRUE(request.isEnabled());
    TEST_ASSERT_EQUAL_UINT8(1U, request.dependencyCount());
    TEST_ASSERT_EQUAL_UINT32(51U, request.dependencyLinks()[0].deviceId);

    FadeAnalogOutputDeviceConfigV1 parsed{};
    TEST_ASSERT_TRUE(
        decodeFadeAnalogOutputDeviceConfig(reinterpret_cast<const uint8_t*>(request.configBlob.data()), request.configBlob.size(), parsed));
    TEST_ASSERT_EQUAL_UINT16(percentToAnalogOutputState(10U), parsed.maxStep);
    TEST_ASSERT_EQUAL_UINT32(200U, parsed.stepIntervalMs);
}

void test_fade_analog_output_api_adapter_partial_update_preserves_step_interval() {
    FadeAnalogOutputDeviceConfigV1 current = makeFadeAnalogOutputConfig();
    FadeAnalogOutputDevice runtime(current);

    StaticJsonDocument<256> doc;
    JsonObject config = doc.createNestedObject("config");
    config["maxStep"] = 20;

    assertMatchesJsonSchema("schemas/rest/v1/requests/devices-update-fade_analog_output.request.schema.json", doc.as<JsonVariantConst>());

    DeviceConfigUpdateRequest request{};
    const char* error = nullptr;
    TEST_ASSERT_TRUE(
        FadeAnalogOutputDeviceApiAdapter::instance().parseUpdateConfigRequest(doc.as<JsonObjectConst>(), runtime, request, error));
    TEST_ASSERT_EQUAL_UINT32(kFadeAnalogOutputDeviceConfigVersion, request.configVersion);
    TEST_ASSERT_FALSE(request.depsProvided);

    FadeAnalogOutputDeviceConfigV1 parsed{};
    TEST_ASSERT_TRUE(
        decodeFadeAnalogOutputDeviceConfig(reinterpret_cast<const uint8_t*>(request.configBlob.data()), request.configBlob.size(), parsed));
    TEST_ASSERT_EQUAL_UINT16(percentToAnalogOutputState(20U), parsed.maxStep);
    TEST_ASSERT_EQUAL_UINT32(current.stepIntervalMs, parsed.stepIntervalMs);
}

void test_fade_analog_output_api_adapter_serializes_record() {
    FakeAnalogOutputDevice target("target");
    target.begin(1U);
    target.tickFastLoop(2U);

    FadeAnalogOutputDeviceConfigV1 config = makeFadeAnalogOutputConfig();
    const DeviceConfigBlob blob = encodeFadeAnalogOutputBlob(config);
    DeviceRegistryEntry record = dependentRecord(kFadeAnalogOutputDeviceTypeId, {501U});
    record.header.configVersion = kFadeAnalogOutputDeviceConfigVersion;
    record.header.payloadLength = static_cast<uint32_t>(blob.size());

    FadeAnalogOutputDevice fade(config);
    fade.bindDeviceIdentity(record, blob);
    fade.setDependencyRuntimeAt(0U, &target);
    fade.begin(10U);
    for (uint32_t now = 11U; now < 20U && fade.status() != DeviceStatus::Ready; ++now) {
        fade.tick100ms(now);
    }
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceStatus::Ready), static_cast<int>(fade.status()));
    TEST_ASSERT_TRUE(fade.requestOutputState(kAnalogOutputLevelMax, 20U));
    fade.tick100ms(212U);

    StaticJsonDocument<4096> doc;
    JsonObject output = doc.to<JsonObject>();
    FadeAnalogOutputDeviceApiAdapter::instance().writeDeviceJson(fade, fade.status(), output);

    TEST_ASSERT_FALSE(doc.overflowed());
    assertMatchesJsonSchema("schemas/rest/v1/responses/devices-fade_analog_output.response.schema.json", doc.as<JsonVariantConst>());
    TEST_ASSERT_EQUAL_UINT32(121U, output["record"]["id"].as<uint32_t>());
    TEST_ASSERT_EQUAL_STRING("fade_analog_output", output["record"]["typeName"].as<const char*>());
    TEST_ASSERT_EQUAL_STRING("fade", output["config"]["name"].as<const char*>());
    TEST_ASSERT_EQUAL_UINT8(10U, output["runtime"]["output"]["state"].as<uint8_t>());
    TEST_ASSERT_EQUAL_UINT8(100U, output["runtime"]["output"]["targetState"].as<uint8_t>());
    TEST_ASSERT_TRUE(output["runtime"]["output"]["transitioning"].as<bool>());
}

void test_scheduled_analog_output_api_adapter_parses_create_request() {
    StaticJsonDocument<4096> doc;
    doc["typeName"] = "scheduled_analog_output";
    JsonObject config = doc.createNestedObject("config");
    config["name"] = "schedule";
    config["enabled"] = true;
    JsonArray deps = config.createNestedArray("deps");
    JsonObject outputDep = deps.createNestedObject();
    outputDep["role"] = "analog_output";
    outputDep["deviceId"] = 61;
    JsonArray points = config.createNestedArray("points");
    JsonObject point0 = points.createNestedObject();
    point0["minuteOfDay"] = 480;
    point0["state"] = 100;
    JsonObject point1 = points.createNestedObject();
    point1["minuteOfDay"] = 1200;
    point1["state"] = 25;

    assertMatchesJsonSchema("schemas/rest/v1/requests/devices-create-scheduled_analog_output.request.schema.json",
                            doc.as<JsonVariantConst>());

    DeviceCreateRequest request{};
    const char* error = nullptr;
    TEST_ASSERT_TRUE_MESSAGE(
        ScheduledAnalogOutputDeviceApiAdapter::instance().parseCreateRequest(doc.as<JsonObjectConst>(), request, error), error);
    TEST_ASSERT_EQUAL_UINT32(kScheduledAnalogOutputDeviceTypeId, request.typeId);
    TEST_ASSERT_EQUAL_UINT32(kScheduledAnalogOutputDeviceConfigVersion, request.configVersion);
    TEST_ASSERT_EQUAL_STRING("schedule", request.baseConfig.name);
    TEST_ASSERT_TRUE(request.isEnabled());
    TEST_ASSERT_EQUAL_UINT8(1U, request.dependencyCount());
    TEST_ASSERT_EQUAL_UINT32(61U, request.dependencyLinks()[0].deviceId);

    ScheduledAnalogOutputDeviceConfigV2 parsed{};
    TEST_ASSERT_TRUE(decodeScheduledAnalogOutputDeviceConfig(reinterpret_cast<const uint8_t*>(request.configBlob.data()),
                                                             request.configBlob.size(), parsed));
    TEST_ASSERT_EQUAL_UINT8(0U, parsed.points[0].deleted);
    TEST_ASSERT_EQUAL_UINT16(480U, parsed.points[0].minuteOfDay);
    TEST_ASSERT_EQUAL_UINT16(percentToAnalogOutputState(100U), parsed.points[0].state);
    TEST_ASSERT_EQUAL_UINT8(0U, parsed.points[1].deleted);
    TEST_ASSERT_EQUAL_UINT16(1200U, parsed.points[1].minuteOfDay);
    TEST_ASSERT_EQUAL_UINT16(percentToAnalogOutputState(25U), parsed.points[1].state);
}

void test_scheduled_analog_output_api_adapter_partial_update_preserves_points() {
    ScheduledAnalogOutputDeviceConfigV2 current = makeScheduledAnalogOutputConfig();
    ScheduledAnalogOutputDevice runtime(current);

    StaticJsonDocument<512> doc;
    JsonObject config = doc.createNestedObject("config");
    config["enabled"] = false;

    assertMatchesJsonSchema("schemas/rest/v1/requests/devices-update-scheduled_analog_output.request.schema.json",
                            doc.as<JsonVariantConst>());

    DeviceConfigUpdateRequest request{};
    const char* error = nullptr;
    TEST_ASSERT_TRUE(
        ScheduledAnalogOutputDeviceApiAdapter::instance().parseUpdateConfigRequest(doc.as<JsonObjectConst>(), runtime, request, error));
    TEST_ASSERT_EQUAL_UINT32(kScheduledAnalogOutputDeviceConfigVersion, request.configVersion);
    TEST_ASSERT_FALSE(request.depsProvided);

    ScheduledAnalogOutputDeviceConfigV2 parsed{};
    TEST_ASSERT_TRUE(decodeScheduledAnalogOutputDeviceConfig(reinterpret_cast<const uint8_t*>(request.configBlob.data()),
                                                             request.configBlob.size(), parsed));
    TEST_ASSERT_FALSE(parsed.enabled);
    TEST_ASSERT_EQUAL_UINT16(current.points[0].minuteOfDay, parsed.points[0].minuteOfDay);
    TEST_ASSERT_EQUAL_UINT16(current.points[0].state, parsed.points[0].state);
    TEST_ASSERT_EQUAL_UINT16(current.points[1].minuteOfDay, parsed.points[1].minuteOfDay);
    TEST_ASSERT_EQUAL_UINT16(current.points[1].state, parsed.points[1].state);
}

void test_scheduled_analog_output_api_adapter_serializes_record() {
    FakeAnalogOutputDevice target("target");
    target.begin(1U);
    target.tickFastLoop(2U);

    ScheduledAnalogOutputDeviceConfigV2 config = makeScheduledAnalogOutputConfig();
    const DeviceConfigBlob blob = encodeScheduledAnalogOutputBlob(config);
    DeviceRegistryEntry record = dependentRecord(kScheduledAnalogOutputDeviceTypeId, {601U});
    record.header.configVersion = kScheduledAnalogOutputDeviceConfigVersion;
    record.header.payloadLength = static_cast<uint32_t>(blob.size());

    ScheduledAnalogOutputDevice device(config);
    device.bindDeviceIdentity(record, blob);
    device.setDependencyRuntimeAt(0U, &target);
    device.begin(10U);
    for (uint32_t now = 11U; now < 20U && device.status() != DeviceStatus::Ready; ++now) {
        device.tickFastLoop(now);
    }
    TEST_ASSERT_EQUAL(static_cast<int>(DeviceStatus::Ready), static_cast<int>(device.status()));
    device.tickFastLoop(21U);

    StaticJsonDocument<4096> doc;
    JsonObject output = doc.to<JsonObject>();
    ScheduledAnalogOutputDeviceApiAdapter::instance().writeDeviceJson(device, device.status(), output);

    TEST_ASSERT_FALSE(doc.overflowed());
    assertMatchesJsonSchema("schemas/rest/v1/responses/devices-scheduled_analog_output.response.schema.json", doc.as<JsonVariantConst>());
    TEST_ASSERT_EQUAL_UINT32(122U, output["record"]["id"].as<uint32_t>());
    TEST_ASSERT_EQUAL_STRING("scheduled_analog_output", output["record"]["typeName"].as<const char*>());
    TEST_ASSERT_EQUAL_STRING("schedule", output["config"]["name"].as<const char*>());
    TEST_ASSERT_EQUAL_UINT8(0U, output["runtime"]["output"]["state"].as<uint8_t>());
    TEST_ASSERT_EQUAL_UINT8(0U, output["runtime"]["output"]["requestedState"].as<uint8_t>());
    TEST_ASSERT_EQUAL_STRING("scheduled", output["runtime"]["output"]["mode"].as<const char*>());
    TEST_ASSERT_FALSE(output["runtime"]["output"]["timeValid"].as<bool>());
}

void test_ds18b20_api_adapter_parses_create_request() {
    StaticJsonDocument<512> doc;
    doc["typeName"] = "ds18b20_temperature_sensor";
    JsonObject config = doc.createNestedObject("config");
    config["name"] = "temperature";
    config["enabled"] = true;
    JsonArray deps = config.createNestedArray("deps");
    JsonObject dep = deps.createNestedObject();
    dep["role"] = "onewire_bus";
    dep["deviceId"] = 44;
    config["address"] = "28FF641D621603AD";
    config["resolution"] = 11;
    config["unit"] = "fahrenheit";
    config["pollMs"] = 2000;
    config["reportDeltaCelsius"] = 0.25;
    config["reportAlways"] = true;

    assertMatchesJsonSchema("schemas/rest/v1/requests/devices-create-ds18b20_temperature_sensor.request.schema.json",
                            doc.as<JsonVariantConst>());

    DeviceCreateRequest request{};
    const char* error = nullptr;
    TEST_ASSERT_TRUE_MESSAGE(
        Ds18b20TemperatureSensorDeviceApiAdapter::instance().parseCreateRequest(doc.as<JsonObjectConst>(), request, error), error);
    TEST_ASSERT_EQUAL_UINT32(kDs18b20TemperatureSensorTypeId, request.typeId);
    TEST_ASSERT_EQUAL_UINT32(kDs18b20TemperatureSensorConfigVersion, request.configVersion);
    TEST_ASSERT_EQUAL_STRING("temperature", request.baseConfig.name);
    TEST_ASSERT_TRUE(request.isEnabled());
    TEST_ASSERT_EQUAL_UINT8(1U, request.dependencyCount());
    TEST_ASSERT_EQUAL_UINT32(44U, request.dependencyLinks()[0].deviceId);

    Ds18b20TemperatureSensorConfigV2 parsed{};
    TEST_ASSERT_TRUE(decodeValidatedFixedConfigBlob(Ds18b20TemperatureSensorConfigV2::kMagic,
                                                    reinterpret_cast<const uint8_t*>(request.configBlob.data()), request.configBlob.size(),
                                                    parsed));
    TEST_ASSERT_EQUAL_UINT8(11U, parsed.resolution);
    TEST_ASSERT_EQUAL_UINT8(temperatureUnitToByte(TemperatureUnit::Fahrenheit), parsed.outputUnit);
    TEST_ASSERT_EQUAL_UINT16(25U, parsed.reportDeltaCentiCelsius);
    TEST_ASSERT_TRUE(parsed.reportAlways != 0U);
    TEST_ASSERT_EQUAL_UINT32(2000U, parsed.pollMs);
}

void test_ds18b20_api_adapter_partial_update_preserves_address_unit_and_report_always() {
    Ds18b20TemperatureSensorConfigV2 current = makeDs18b20Config();
    Ds18b20TemperatureSensorDevice runtime(current);

    StaticJsonDocument<256> doc;
    JsonObject config = doc.createNestedObject("config");
    config["pollMs"] = 3000;
    config["reportAlways"] = false;

    assertMatchesJsonSchema("schemas/rest/v1/requests/devices-update-ds18b20_temperature_sensor.request.schema.json",
                            doc.as<JsonVariantConst>());

    DeviceConfigUpdateRequest request{};
    const char* error = nullptr;
    TEST_ASSERT_TRUE_MESSAGE(
        Ds18b20TemperatureSensorDeviceApiAdapter::instance().parseUpdateConfigRequest(doc.as<JsonObjectConst>(), runtime, request, error),
        error);
    TEST_ASSERT_FALSE(request.depsProvided);

    Ds18b20TemperatureSensorConfigV2 parsed{};
    TEST_ASSERT_TRUE(decodeValidatedFixedConfigBlob(Ds18b20TemperatureSensorConfigV2::kMagic,
                                                    reinterpret_cast<const uint8_t*>(request.configBlob.data()), request.configBlob.size(),
                                                    parsed));
    TEST_ASSERT_EQUAL_UINT32(3000U, parsed.pollMs);
    TEST_ASSERT_FALSE(parsed.reportAlways != 0U);
    TEST_ASSERT_EQUAL_UINT8(current.resolution, parsed.resolution);
    TEST_ASSERT_EQUAL_MEMORY(&current.address, &parsed.address, sizeof(current.address));
    TEST_ASSERT_EQUAL_UINT8(current.outputUnit, parsed.outputUnit);
}

void test_ds18b20_api_adapter_serializes_record() {
    Ds18b20TemperatureSensorConfigV2 config = makeDs18b20Config();
    const DeviceConfigBlob blob = encodeDs18b20Blob(config);
    DeviceRegistryEntry record{};
    record.header.deviceId = 73U;
    record.header.typeId = kDs18b20TemperatureSensorTypeId;
    record.header.configVersion = kDs18b20TemperatureSensorConfigVersion;
    record.header.payloadLength = static_cast<uint32_t>(blob.size());
    record.depCount = 1U;
    record.deps[0] = {DeviceRole::OneWireBus, 44U};

    Ds18b20TemperatureSensorDevice sensor(config);
    sensor.bindDeviceIdentity(record, blob);
    sensor.begin(10U);
    sensor.tick100ms(11U);

    StaticJsonDocument<4096> doc;
    JsonObject output = doc.to<JsonObject>();
    Ds18b20TemperatureSensorDeviceApiAdapter::instance().writeDeviceJson(sensor, sensor.status(), output);

    TEST_ASSERT_FALSE(doc.overflowed());
    assertMatchesJsonSchema("schemas/rest/v1/responses/devices-ds18b20_temperature_sensor.response.schema.json",
                            doc.as<JsonVariantConst>());
    TEST_ASSERT_EQUAL_UINT32(73U, output["record"]["id"].as<uint32_t>());
    TEST_ASSERT_EQUAL_STRING("ds18b20_temperature_sensor", output["record"]["typeName"].as<const char*>());
    TEST_ASSERT_EQUAL_STRING("temperature", output["config"]["name"].as<const char*>());
    TEST_ASSERT_TRUE(output["runtime"]["output"]["temperature"].is<JsonObjectConst>());
    TEST_ASSERT_FALSE(output["runtime"]["output"]["temperature"]["valid"].as<bool>());
}

void test_htu21_api_adapter_parses_create_request() {
    StaticJsonDocument<512> doc;
    doc["typeName"] = "htu21";
    JsonObject config = doc.createNestedObject("config");
    config["name"] = "climate";
    config["enabled"] = true;
    JsonArray deps = config.createNestedArray("deps");
    JsonObject dep = deps.createNestedObject();
    dep["role"] = "i2c_bus";
    dep["deviceId"] = 44;
    config["i2cAddress"] = 0x41;

    assertMatchesJsonSchema("schemas/rest/v1/requests/devices-create-htu21.request.schema.json", doc.as<JsonVariantConst>());

    DeviceCreateRequest request{};
    const char* error = nullptr;
    TEST_ASSERT_TRUE_MESSAGE(Htu21SensorDeviceApiAdapter::instance().parseCreateRequest(doc.as<JsonObjectConst>(), request, error), error);
    TEST_ASSERT_EQUAL_UINT32(kHtu21SensorTypeId, request.typeId);
    TEST_ASSERT_EQUAL_UINT32(kHtu21SensorConfigVersion, request.configVersion);
    TEST_ASSERT_EQUAL_STRING("climate", request.baseConfig.name);
    TEST_ASSERT_TRUE(request.isEnabled());
    TEST_ASSERT_EQUAL_UINT8(1U, request.dependencyCount());
    TEST_ASSERT_EQUAL_UINT32(44U, request.dependencyLinks()[0].deviceId);

    Htu21SensorConfigV3 parsed{};
    TEST_ASSERT_TRUE(
        decodeHtu21SensorConfig(reinterpret_cast<const uint8_t*>(request.configBlob.data()), request.configBlob.size(), parsed));
    TEST_ASSERT_EQUAL_UINT8(0x41U, parsed.i2cAddress);
}

void test_htu21_api_adapter_partial_update_preserves_i2c_address() {
    Htu21SensorConfigV3 current = makeHtu21Config(0x41U);
    current.outputUnit = temperatureUnitToByte(TemperatureUnit::Fahrenheit);
    current.reportDeltaCentiCelsius = 25U;
    current.reportDeltaCentiPercent = 50U;
    current.temperatureFilter.smoothingWeight = 0.5F;
    Htu21SensorDevice runtime(current);

    StaticJsonDocument<256> doc;
    JsonObject config = doc.createNestedObject("config");
    config["name"] = "climate";
    config["pollMs"] = 60000U;

    assertMatchesJsonSchema("schemas/rest/v1/requests/devices-update-htu21.request.schema.json", doc.as<JsonVariantConst>());

    DeviceConfigUpdateRequest request{};
    const char* error = nullptr;
    TEST_ASSERT_TRUE_MESSAGE(
        Htu21SensorDeviceApiAdapter::instance().parseUpdateConfigRequest(doc.as<JsonObjectConst>(), runtime, request, error), error);
    TEST_ASSERT_FALSE(request.depsProvided);

    Htu21SensorConfigV3 parsed{};
    TEST_ASSERT_TRUE(
        decodeHtu21SensorConfig(reinterpret_cast<const uint8_t*>(request.configBlob.data()), request.configBlob.size(), parsed));
    TEST_ASSERT_EQUAL_UINT32(60000U, parsed.pollMs);
    TEST_ASSERT_EQUAL_UINT8(temperatureUnitToByte(TemperatureUnit::Fahrenheit), parsed.outputUnit);
    TEST_ASSERT_EQUAL_UINT16(25U, parsed.reportDeltaCentiCelsius);
    TEST_ASSERT_EQUAL_UINT16(50U, parsed.reportDeltaCentiPercent);
    TEST_ASSERT_EQUAL_FLOAT(0.5F, parsed.temperatureFilter.smoothingWeight);
    TEST_ASSERT_EQUAL_UINT8(0x41U, parsed.i2cAddress);
}

void test_htu21_api_adapter_serializes_record() {
    Htu21SensorConfigV3 config = makeHtu21Config(0x41U);
    const DeviceConfigBlob blob = encodeHtu21Blob(config);
    DeviceRegistryEntry record{};
    record.header.deviceId = 93U;
    record.header.typeId = kHtu21SensorTypeId;
    record.header.configVersion = kHtu21SensorConfigVersion;
    record.header.payloadLength = static_cast<uint32_t>(blob.size());
    record.depCount = 1U;
    record.deps[0] = {DeviceRole::I2CBus, 44U};

    Htu21SensorDevice device(config);
    device.bindDeviceIdentity(record, blob);

    StaticJsonDocument<4096> doc;
    JsonObject output = doc.to<JsonObject>();
    Htu21SensorDeviceApiAdapter::instance().writeDeviceJson(device, device.status(), output);

    TEST_ASSERT_FALSE(doc.overflowed());
    assertMatchesJsonSchema("schemas/rest/v1/responses/devices-htu21.response.schema.json", doc.as<JsonVariantConst>());
    TEST_ASSERT_EQUAL_UINT32(93U, output["record"]["id"].as<uint32_t>());
    TEST_ASSERT_EQUAL_STRING("htu21", output["record"]["typeName"].as<const char*>());
    TEST_ASSERT_EQUAL_STRING("climate", output["config"]["name"].as<const char*>());
    TEST_ASSERT_TRUE(output["runtime"]["output"]["temperature"].is<JsonObjectConst>());
    TEST_ASSERT_TRUE(output["runtime"]["output"]["humidity"].is<JsonObjectConst>());
}

void test_analog_input_channel_api_adapter_schema_smoke() {
    StaticJsonDocument<512> createDoc;
    createDoc["typeName"] = "analog_input_channel";
    JsonObject createConfig = createDoc.createNestedObject("config");
    makeAnalogInputChannelConfig().writeJson(createConfig);
    JsonArray deps = createConfig.createNestedArray("deps");
    JsonObject dep = deps.createNestedObject();
    dep["role"] = "analog_input_hub";
    dep["deviceId"] = 44;
    assertMatchesJsonSchema("schemas/rest/v1/requests/devices-create-analog_input_channel.request.schema.json",
                            createDoc.as<JsonVariantConst>());

    StaticJsonDocument<512> updateDoc;
    JsonObject updateConfig = updateDoc.createNestedObject("config");
    updateConfig["channel"] = 5;
    updateConfig["pollMs"] = 250U;
    assertMatchesJsonSchema("schemas/rest/v1/requests/devices-update-analog_input_channel.request.schema.json",
                            updateDoc.as<JsonVariantConst>());
}

void test_analog_input_channel_api_adapter_parses_create_request() {
    StaticJsonDocument<512> doc;
    doc["typeName"] = "analog_input_channel";
    JsonObject config = doc.createNestedObject("config");
    makeAnalogInputChannelConfig().writeJson(config);
    JsonArray deps = config.createNestedArray("deps");
    JsonObject dep = deps.createNestedObject();
    dep["role"] = "analog_input_hub";
    dep["deviceId"] = 44;

    DeviceCreateRequest request{};
    const char* error = nullptr;
    TEST_ASSERT_TRUE_MESSAGE(AnalogInputChannelDeviceApiAdapter::instance().parseCreateRequest(doc.as<JsonObjectConst>(), request, error),
                             error);
    TEST_ASSERT_EQUAL_UINT32(kAnalogInputChannelTypeId, request.typeId);
    TEST_ASSERT_EQUAL_STRING("analog channel", request.baseConfig.name);
    TEST_ASSERT_EQUAL_UINT8(1U, request.dependencyCount());
    TEST_ASSERT_EQUAL_UINT32(44U, request.dependencyLinks()[0].deviceId);

    AnalogInputChannelDeviceConfigV1 parsed{};
    TEST_ASSERT_TRUE(decodeValidatedFixedConfigBlob(AnalogInputChannelDeviceConfigV1::kMagic,
                                                    reinterpret_cast<const uint8_t*>(request.configBlob.data()), request.configBlob.size(),
                                                    parsed));
    TEST_ASSERT_EQUAL_UINT8(3U, parsed.channel);
    TEST_ASSERT_EQUAL_UINT8(4U, parsed.poll.adcSamples);
    TEST_ASSERT_EQUAL_UINT32(500U, parsed.poll.pollMs);
}

void test_analog_input_channel_api_adapter_partial_update_preserves_other_fields() {
    FakeAnalogInputHubRuntime hub;
    AnalogInputChannelDevice runtime(makeAnalogInputChannelConfig());
    bindAnalogInputChannelIdentity(runtime, 8022U, 44U);
    driveAnalogInputChannelUntilReading(runtime, hub);

    StaticJsonDocument<256> doc;
    JsonObject config = doc.createNestedObject("config");
    config["channel"] = 5;
    config["pollMs"] = 250U;

    assertMatchesJsonSchema("schemas/rest/v1/requests/devices-update-analog_input_channel.request.schema.json", doc.as<JsonVariantConst>());

    DeviceConfigUpdateRequest request{};
    const char* error = nullptr;
    TEST_ASSERT_TRUE_MESSAGE(
        AnalogInputChannelDeviceApiAdapter::instance().parseUpdateConfigRequest(doc.as<JsonObjectConst>(), runtime, request, error), error);
    TEST_ASSERT_FALSE(request.depsProvided);

    AnalogInputChannelDeviceConfigV1 parsed{};
    TEST_ASSERT_TRUE(decodeValidatedFixedConfigBlob(AnalogInputChannelDeviceConfigV1::kMagic,
                                                    reinterpret_cast<const uint8_t*>(request.configBlob.data()), request.configBlob.size(),
                                                    parsed));
    TEST_ASSERT_EQUAL_UINT8(5U, parsed.channel);
    TEST_ASSERT_EQUAL_UINT8(4U, parsed.poll.adcSamples);
    TEST_ASSERT_EQUAL_UINT32(250U, parsed.poll.pollMs);
    TEST_ASSERT_EQUAL_STRING("analog channel", parsed.name);
}

void test_analog_input_channel_api_adapter_serializes_record() {
    FakeAnalogInputHubRuntime hub;
    AnalogInputChannelDevice device(makeAnalogInputChannelConfig());
    bindAnalogInputChannelIdentity(device, 8023U, 44U);
    driveAnalogInputChannelUntilReading(device, hub);

    StaticJsonDocument<2048> doc;
    JsonObject output = doc.to<JsonObject>();
    AnalogInputChannelDeviceApiAdapter::instance().writeDeviceJson(device, device.status(), output);

    TEST_ASSERT_FALSE(doc.overflowed());
    assertMatchesJsonSchema("schemas/rest/v1/responses/devices-analog_input_channel.response.schema.json", doc.as<JsonVariantConst>());
    TEST_ASSERT_EQUAL_UINT32(8023U, output["record"]["id"].as<uint32_t>());
    TEST_ASSERT_EQUAL_STRING("analog_input_channel", output["record"]["typeName"].as<const char*>());
    TEST_ASSERT_EQUAL_STRING("analog channel", output["config"]["name"].as<const char*>());
    TEST_ASSERT_TRUE(output["runtime"]["output"]["analogInput"]["valid"].as<bool>());
    TEST_ASSERT_EQUAL_UINT32(1660U, output["runtime"]["output"]["analogInput"]["milliVolts"].as<uint32_t>());
}

void test_i2c_bus_api_adapter_schema_smoke() {
    StaticJsonDocument<512> createDoc;
    createDoc["typeName"] = "i2c_bus";
    JsonObject createConfig = createDoc.createNestedObject("config");
    makeI2cBusConfig().writeJson(createConfig);
    assertMatchesJsonSchema("schemas/rest/v1/requests/devices-create-i2c_bus.request.schema.json", createDoc.as<JsonVariantConst>());

    StaticJsonDocument<512> updateDoc;
    JsonObject updateConfig = updateDoc.createNestedObject("config");
    updateConfig["sdaPin"] = 19;
    updateConfig["frequencyHz"] = 100000U;
    assertMatchesJsonSchema("schemas/rest/v1/requests/devices-update-i2c_bus.request.schema.json", updateDoc.as<JsonVariantConst>());
}

void test_i2c_bus_api_adapter_parses_create_request() {
    StaticJsonDocument<512> doc;
    doc["typeName"] = "i2c_bus";
    JsonObject config = doc.createNestedObject("config");
    makeI2cBusConfig().writeJson(config);

    DeviceCreateRequest request{};
    const char* error = nullptr;
    TEST_ASSERT_TRUE_MESSAGE(I2cBusDeviceApiAdapter::instance().parseCreateRequest(doc.as<JsonObjectConst>(), request, error), error);
    TEST_ASSERT_EQUAL_UINT32(I2cBusDevice::descriptor().typeId, request.typeId);
    TEST_ASSERT_EQUAL_STRING("i2c bus", request.baseConfig.name);
    TEST_ASSERT_EQUAL_UINT8(0U, request.dependencyCount());

    I2cBusDeviceConfigV1 parsed{};
    TEST_ASSERT_TRUE(decodeValidatedFixedConfigBlob(
        I2cBusDeviceConfigV1::kMagic, reinterpret_cast<const uint8_t*>(request.configBlob.data()), request.configBlob.size(), parsed));
    TEST_ASSERT_EQUAL_UINT8(21U, parsed.sdaPin);
    TEST_ASSERT_EQUAL_UINT8(22U, parsed.sclPin);
    TEST_ASSERT_EQUAL_UINT8(1U, parsed.internalPullup);
    TEST_ASSERT_EQUAL_UINT32(400000U, parsed.frequencyHz);
}

void test_i2c_bus_api_adapter_partial_update_preserves_other_fields() {
    FakeI2cBusDriver driver;
    I2cBusDevice runtime(makeI2cBusConfig(), driver);
    bindI2cBusIdentity(runtime, 8024U);
    driveI2cBusToReady(runtime);

    StaticJsonDocument<256> doc;
    JsonObject config = doc.createNestedObject("config");
    config["sdaPin"] = 19U;
    config["frequencyHz"] = 100000U;

    assertMatchesJsonSchema("schemas/rest/v1/requests/devices-update-i2c_bus.request.schema.json", doc.as<JsonVariantConst>());

    DeviceConfigUpdateRequest request{};
    const char* error = nullptr;
    TEST_ASSERT_TRUE_MESSAGE(
        I2cBusDeviceApiAdapter::instance().parseUpdateConfigRequest(doc.as<JsonObjectConst>(), runtime, request, error), error);
    TEST_ASSERT_FALSE(request.depsProvided);

    I2cBusDeviceConfigV1 parsed{};
    TEST_ASSERT_TRUE(decodeValidatedFixedConfigBlob(
        I2cBusDeviceConfigV1::kMagic, reinterpret_cast<const uint8_t*>(request.configBlob.data()), request.configBlob.size(), parsed));
    TEST_ASSERT_EQUAL_UINT8(19U, parsed.sdaPin);
    TEST_ASSERT_EQUAL_UINT8(22U, parsed.sclPin);
    TEST_ASSERT_EQUAL_UINT8(1U, parsed.internalPullup);
    TEST_ASSERT_EQUAL_UINT32(100000U, parsed.frequencyHz);
}

void test_i2c_bus_api_adapter_serializes_record() {
    FakeI2cBusDriver driver;
    I2cBusDevice device(makeI2cBusConfig(), driver);
    bindI2cBusIdentity(device, 8025U);
    driveI2cBusToReady(device);

    StaticJsonDocument<2048> doc;
    JsonObject output = doc.to<JsonObject>();
    I2cBusDeviceApiAdapter::instance().writeDeviceJson(device, device.status(), output);

    TEST_ASSERT_FALSE(doc.overflowed());
    assertMatchesJsonSchema("schemas/rest/v1/responses/devices-i2c_bus.response.schema.json", doc.as<JsonVariantConst>());
    TEST_ASSERT_EQUAL_UINT32(8025U, output["record"]["id"].as<uint32_t>());
    TEST_ASSERT_EQUAL_STRING("i2c_bus", output["record"]["typeName"].as<const char*>());
    TEST_ASSERT_EQUAL_STRING("i2c bus", output["config"]["name"].as<const char*>());
    TEST_ASSERT_FALSE(output["runtime"]["transactionActive"].as<bool>());
}

void test_rtc_ds3231_api_adapter_schema_smoke() {
    StaticJsonDocument<512> createDoc;
    createDoc["typeName"] = "rtc_ds3231";
    JsonObject createConfig = createDoc.createNestedObject("config");
    makeRtcConfig().writeJson(createConfig);
    JsonArray deps = createConfig.createNestedArray("deps");
    JsonObject dep = deps.createNestedObject();
    dep["role"] = "i2c_bus";
    dep["deviceId"] = 44;
    assertMatchesJsonSchema("schemas/rest/v1/requests/devices-create-rtc_ds3231.request.schema.json", createDoc.as<JsonVariantConst>());

    StaticJsonDocument<512> updateDoc;
    JsonObject updateConfig = updateDoc.createNestedObject("config");
    updateConfig["i2cAddress"] = 0x69;
    updateConfig["useForSystemTimeSync"] = false;
    assertMatchesJsonSchema("schemas/rest/v1/requests/devices-update-rtc_ds3231.request.schema.json", updateDoc.as<JsonVariantConst>());
}

void test_rtc_ds3231_api_adapter_parses_create_request() {
    StaticJsonDocument<512> doc;
    doc["typeName"] = "rtc_ds3231";
    JsonObject config = doc.createNestedObject("config");
    makeRtcConfig().writeJson(config);
    JsonArray deps = config.createNestedArray("deps");
    JsonObject dep = deps.createNestedObject();
    dep["role"] = "i2c_bus";
    dep["deviceId"] = 44;

    DeviceCreateRequest request{};
    const char* error = nullptr;
    TEST_ASSERT_TRUE_MESSAGE(Ds3231RtcDeviceApiAdapter::instance().parseCreateRequest(doc.as<JsonObjectConst>(), request, error), error);
    TEST_ASSERT_EQUAL_UINT32(Ds3231RtcDevice::descriptor().typeId, request.typeId);
    TEST_ASSERT_EQUAL_STRING("rtc", request.baseConfig.name);
    TEST_ASSERT_EQUAL_UINT8(1U, request.dependencyCount());
    TEST_ASSERT_EQUAL_UINT32(44U, request.dependencyLinks()[0].deviceId);

    Ds3231RtcDeviceConfigV2 parsed{};
    TEST_ASSERT_TRUE(
        decodeDs3231RtcDeviceConfig(reinterpret_cast<const uint8_t*>(request.configBlob.data()), request.configBlob.size(), parsed));
    TEST_ASSERT_EQUAL_UINT8(0x68U, parsed.i2cAddress);
    TEST_ASSERT_TRUE(parsed.useForSystemTimeSync != 0U);
}

void test_rtc_ds3231_api_adapter_partial_update_preserves_other_fields() {
    FakeDs3231I2cBusDriver busDriver;
    I2cBusDevice bus(makeI2cBusConfig(), busDriver);
    bindI2cBusIdentity(bus, 9010U);
    driveI2cBusToReady(bus);

    Ds3231RtcDevice runtime(makeRtcConfig());
    bindRtcIdentity(runtime, 9011U, 9010U);
    driveRtcUntilReading(runtime, bus);

    StaticJsonDocument<256> doc;
    JsonObject config = doc.createNestedObject("config");
    config["i2cAddress"] = 0x69U;

    assertMatchesJsonSchema("schemas/rest/v1/requests/devices-update-rtc_ds3231.request.schema.json", doc.as<JsonVariantConst>());

    DeviceConfigUpdateRequest request{};
    const char* error = nullptr;
    TEST_ASSERT_TRUE_MESSAGE(
        Ds3231RtcDeviceApiAdapter::instance().parseUpdateConfigRequest(doc.as<JsonObjectConst>(), runtime, request, error), error);
    TEST_ASSERT_FALSE(request.depsProvided);

    Ds3231RtcDeviceConfigV2 parsed{};
    TEST_ASSERT_TRUE(
        decodeDs3231RtcDeviceConfig(reinterpret_cast<const uint8_t*>(request.configBlob.data()), request.configBlob.size(), parsed));
    TEST_ASSERT_EQUAL_UINT8(0x69U, parsed.i2cAddress);
    TEST_ASSERT_TRUE(parsed.useForSystemTimeSync != 0U);
}

void test_rtc_ds3231_api_adapter_serializes_record() {
    FakeDs3231I2cBusDriver busDriver;
    I2cBusDevice bus(makeI2cBusConfig(), busDriver);
    bindI2cBusIdentity(bus, 9012U);
    driveI2cBusToReady(bus);

    Ds3231RtcDevice device(makeRtcConfig());
    bindRtcIdentity(device, 9013U, 9012U);
    driveRtcUntilReading(device, bus);

    StaticJsonDocument<4096> doc;
    JsonObject output = doc.to<JsonObject>();
    Ds3231RtcDeviceApiAdapter::instance().writeDeviceJson(device, device.status(), output);

    TEST_ASSERT_FALSE(doc.overflowed());
    assertMatchesJsonSchema("schemas/rest/v1/responses/devices-rtc_ds3231.response.schema.json", doc.as<JsonVariantConst>());
    TEST_ASSERT_EQUAL_UINT32(9013U, output["record"]["id"].as<uint32_t>());
    TEST_ASSERT_EQUAL_STRING("rtc_ds3231", output["record"]["typeName"].as<const char*>());
    TEST_ASSERT_EQUAL_STRING("rtc", output["config"]["name"].as<const char*>());
    TEST_ASSERT_TRUE(output["runtime"]["lastReadOk"].as<bool>());
    TEST_ASSERT_EQUAL_UINT32(0U, output["runtime"]["oscillatorStopped"].as<uint32_t>());
    TEST_ASSERT_TRUE(output["runtime"]["currentEpochUtc"].as<uint32_t>() > 0U);
}

void test_spi_bus_api_adapter_schema_smoke() {
    StaticJsonDocument<512> createDoc;
    createDoc["typeName"] = "spi_bus";
    JsonObject createConfig = createDoc.createNestedObject("config");
    makeSpiBusConfig().writeJson(createConfig);
    assertMatchesJsonSchema("schemas/rest/v1/requests/devices-create-spi_bus.request.schema.json", createDoc.as<JsonVariantConst>());

    StaticJsonDocument<512> updateDoc;
    JsonObject updateConfig = updateDoc.createNestedObject("config");
    updateConfig["sckPin"] = 19;
    updateConfig["misoPin"] = 33;
    assertMatchesJsonSchema("schemas/rest/v1/requests/devices-update-spi_bus.request.schema.json", updateDoc.as<JsonVariantConst>());
}

void test_spi_bus_api_adapter_parses_create_request() {
    StaticJsonDocument<512> doc;
    doc["typeName"] = "spi_bus";
    JsonObject config = doc.createNestedObject("config");
    makeSpiBusConfig().writeJson(config);

    DeviceCreateRequest request{};
    const char* error = nullptr;
    TEST_ASSERT_TRUE_MESSAGE(SpiBusDeviceApiAdapter::instance().parseCreateRequest(doc.as<JsonObjectConst>(), request, error), error);
    TEST_ASSERT_EQUAL_UINT32(SpiBusDevice::descriptor().typeId, request.typeId);
    TEST_ASSERT_EQUAL_STRING("spi bus", request.baseConfig.name);

    SpiBusDeviceConfigV2 parsed{};
    TEST_ASSERT_TRUE(decodeValidatedFixedConfigBlob(
        SpiBusDeviceConfigV2::kMagic, reinterpret_cast<const uint8_t*>(request.configBlob.data()), request.configBlob.size(), parsed));
    TEST_ASSERT_EQUAL_UINT8(kSpiBusHostVspi, parsed.host);
    TEST_ASSERT_EQUAL_UINT8(18U, parsed.sckPin);
    TEST_ASSERT_EQUAL_UINT8(23U, parsed.mosiPin);
    TEST_ASSERT_EQUAL_UINT8(kGpioPinUnset, parsed.misoPin);
}

void test_spi_bus_api_adapter_partial_update_preserves_other_fields() {
    FakeSpiBusDriver busDriver;
    FakeSpiCsProbeDriver csProbeDriver;
    SpiBusDevice runtime(makeSpiBusConfig(), busDriver, csProbeDriver);
    bindSpiBusIdentity(runtime, 9020U);
    driveSpiBusToReady(runtime);

    StaticJsonDocument<256> doc;
    JsonObject config = doc.createNestedObject("config");
    config["sckPin"] = 19U;
    config["misoPin"] = 33U;

    assertMatchesJsonSchema("schemas/rest/v1/requests/devices-update-spi_bus.request.schema.json", doc.as<JsonVariantConst>());

    DeviceConfigUpdateRequest request{};
    const char* error = nullptr;
    TEST_ASSERT_TRUE_MESSAGE(
        SpiBusDeviceApiAdapter::instance().parseUpdateConfigRequest(doc.as<JsonObjectConst>(), runtime, request, error), error);
    TEST_ASSERT_FALSE(request.depsProvided);

    SpiBusDeviceConfigV2 parsed{};
    TEST_ASSERT_TRUE(decodeValidatedFixedConfigBlob(
        SpiBusDeviceConfigV2::kMagic, reinterpret_cast<const uint8_t*>(request.configBlob.data()), request.configBlob.size(), parsed));
    TEST_ASSERT_EQUAL_UINT8(kSpiBusHostVspi, parsed.host);
    TEST_ASSERT_EQUAL_UINT8(19U, parsed.sckPin);
    TEST_ASSERT_EQUAL_UINT8(23U, parsed.mosiPin);
    TEST_ASSERT_EQUAL_UINT8(33U, parsed.misoPin);
}

void test_spi_bus_api_adapter_serializes_record() {
    FakeSpiBusDriver busDriver;
    FakeSpiCsProbeDriver csProbeDriver;
    SpiBusDevice device(makeSpiBusConfig(), busDriver, csProbeDriver);
    bindSpiBusIdentity(device, 9021U);
    driveSpiBusToReady(device);
    TEST_ASSERT_TRUE(device.probeChipSelect(5U));

    StaticJsonDocument<2048> doc;
    JsonObject output = doc.to<JsonObject>();
    SpiBusDeviceApiAdapter::instance().writeDeviceJson(device, device.status(), output);

    TEST_ASSERT_FALSE(doc.overflowed());
    assertMatchesJsonSchema("schemas/rest/v1/responses/devices-spi_bus.response.schema.json", doc.as<JsonVariantConst>());
    TEST_ASSERT_EQUAL_UINT32(9021U, output["record"]["id"].as<uint32_t>());
    TEST_ASSERT_EQUAL_STRING("spi_bus", output["record"]["typeName"].as<const char*>());
    TEST_ASSERT_EQUAL_STRING("spi bus", output["config"]["name"].as<const char*>());
    TEST_ASSERT_TRUE(output["runtime"]["transactionActive"].as<bool>() == false);
    TEST_ASSERT_TRUE(output["runtime"]["probe"]["ready"].as<bool>());
    TEST_ASSERT_EQUAL_STRING("inconclusive", output["runtime"]["probe"]["outcome"].as<const char*>());
}

void test_st7735_api_adapter_schema_smoke() {
    StaticJsonDocument<512> createDoc;
    createDoc["typeName"] = "st7735";
    JsonObject createConfig = createDoc.createNestedObject("config");
    makeSt7735Config().writeJson(createConfig);
    assertMatchesJsonSchema("schemas/rest/v1/requests/devices-create-st7735.request.schema.json", createDoc.as<JsonVariantConst>());

    StaticJsonDocument<512> updateDoc;
    JsonObject updateConfig = updateDoc.createNestedObject("config");
    updateConfig["spiBusDeviceId"] = 11;
    updateConfig["chipSelectPin"] = 6;
    updateConfig["rotation"] = 1;
    assertMatchesJsonSchema("schemas/rest/v1/requests/devices-update-st7735.request.schema.json", updateDoc.as<JsonVariantConst>());
}

void test_st7735_api_adapter_parses_create_request() {
    StaticJsonDocument<512> doc;
    doc["typeName"] = "st7735";
    JsonObject config = doc.createNestedObject("config");
    makeSt7735Config().writeJson(config);

    DeviceCreateRequest request{};
    const char* error = nullptr;
    TEST_ASSERT_TRUE_MESSAGE(St7735DeviceApiAdapter::instance().parseCreateRequest(doc.as<JsonObjectConst>(), request, error), error);
    TEST_ASSERT_EQUAL_UINT32(St7735Device::descriptor().typeId, request.typeId);
    TEST_ASSERT_EQUAL_STRING("display", request.baseConfig.name);
    TEST_ASSERT_EQUAL_UINT8(1U, request.dependencyCount());
    TEST_ASSERT_EQUAL_UINT32(11U, request.dependencyLinks()[0].deviceId);

    St7735DeviceConfigV6 parsed{};
    TEST_ASSERT_TRUE(
        decodeSt7735DeviceConfig(reinterpret_cast<const uint8_t*>(request.configBlob.data()), request.configBlob.size(), parsed));
    TEST_ASSERT_EQUAL_UINT32(11U, parsed.spiBusDeviceId);
    TEST_ASSERT_EQUAL_UINT8(5U, parsed.chipSelectPin);
    TEST_ASSERT_EQUAL_UINT8(2U, parsed.dcPin);
    TEST_ASSERT_EQUAL_UINT8(kGpioPinUnset, parsed.resetPin);
    TEST_ASSERT_EQUAL_UINT8(128U, parsed.width);
    TEST_ASSERT_EQUAL_UINT8(160U, parsed.height);
}

void test_st7735_api_adapter_partial_update_preserves_other_fields() {
    const St7735DeviceConfigV6 current = makeSt7735Config();
    const DeviceConfigBlob blob = encodeSt7735Blob(current);
    const DeviceRegistryEntry record = makeSt7735Record(9030U, 11U, current);
    St7735Device runtime(record, blob);

    StaticJsonDocument<256> doc;
    JsonObject config = doc.createNestedObject("config");
    // 15, not 6 -- pins 6-11 are the flash-strapping range gpioSwitchPinIsValid() now rejects for
    // chipSelectPin (St7735DeviceConfigV6 added that check; V5 never validated this field at all).
    config["chipSelectPin"] = 15U;
    config["rotation"] = 1U;

    assertMatchesJsonSchema("schemas/rest/v1/requests/devices-update-st7735.request.schema.json", doc.as<JsonVariantConst>());

    DeviceConfigUpdateRequest request{};
    const char* error = nullptr;
    TEST_ASSERT_TRUE_MESSAGE(
        St7735DeviceApiAdapter::instance().parseUpdateConfigRequest(doc.as<JsonObjectConst>(), runtime, request, error), error);
    TEST_ASSERT_FALSE(request.depsProvided);

    St7735DeviceConfigV6 parsed{};
    TEST_ASSERT_TRUE(
        decodeSt7735DeviceConfig(reinterpret_cast<const uint8_t*>(request.configBlob.data()), request.configBlob.size(), parsed));
    TEST_ASSERT_EQUAL_UINT32(11U, parsed.spiBusDeviceId);
    TEST_ASSERT_EQUAL_UINT8(15U, parsed.chipSelectPin);
    TEST_ASSERT_EQUAL_UINT8(2U, parsed.dcPin);
    TEST_ASSERT_EQUAL_UINT8(kGpioPinUnset, parsed.resetPin);
    TEST_ASSERT_EQUAL_UINT8(1U, parsed.rotation);
    TEST_ASSERT_EQUAL_UINT16(128U, parsed.width);
    TEST_ASSERT_EQUAL_UINT16(160U, parsed.height);
}

void test_st7735_api_adapter_serializes_record() {
    const St7735DeviceConfigV6 config = makeSt7735Config();
    const DeviceConfigBlob blob = encodeSt7735Blob(config);
    const DeviceRegistryEntry record = makeSt7735Record(9031U, 11U, config);
    St7735Device device(record, blob);

    StaticJsonDocument<4096> doc;
    JsonObject output = doc.to<JsonObject>();
    St7735DeviceApiAdapter::instance().writeDeviceJson(device, device.status(), output);

    TEST_ASSERT_FALSE(doc.overflowed());
    assertMatchesJsonSchema("schemas/rest/v1/responses/devices-st7735.response.schema.json", doc.as<JsonVariantConst>());
    TEST_ASSERT_EQUAL_UINT32(9031U, output["record"]["id"].as<uint32_t>());
    TEST_ASSERT_EQUAL_STRING("st7735", output["record"]["typeName"].as<const char*>());
    TEST_ASSERT_EQUAL_STRING("display", output["config"]["name"].as<const char*>());
    TEST_ASSERT_EQUAL_UINT32(11U, output["config"]["spiBusDeviceId"].as<uint32_t>());
}

void test_gpio_switch_api_adapter_parses_create_request() {
    StaticJsonDocument<512> doc;
    doc["typeName"] = "gpio_switch";
    JsonObject config = doc.createNestedObject("config");
    config["name"] = "relay";
    config["enabled"] = true;
    config["restorePreviousState"] = true;
    config["startupState"] = true;
    config["safeState"] = false;
    config["inverted"] = true;
    config["gpioPin"] = 21;

    assertMatchesJsonSchema("schemas/rest/v1/requests/devices-create-gpio_switch.request.schema.json", doc.as<JsonVariantConst>());

    DeviceCreateRequest request{};
    const char* error = nullptr;
    TEST_ASSERT_TRUE_MESSAGE(GpioSwitchDeviceApiAdapter::instance().parseCreateRequest(doc.as<JsonObjectConst>(), request, error), error);
    TEST_ASSERT_EQUAL_UINT32(GpioSwitchDevice::descriptor().typeId, request.typeId);
    TEST_ASSERT_EQUAL_STRING("relay", request.baseConfig.name);
    TEST_ASSERT_TRUE(request.isEnabled());

    GpioSwitchDeviceConfigV3 parsed{};
    TEST_ASSERT_TRUE(
        decodeGpioSwitchDeviceConfig(reinterpret_cast<const uint8_t*>(request.configBlob.data()), request.configBlob.size(), parsed));
    TEST_ASSERT_TRUE(parsed.enabled);
    TEST_ASSERT_EQUAL_STRING("relay", parsed.name);
    TEST_ASSERT_TRUE(parsed.restorePreviousState);
    TEST_ASSERT_TRUE(parsed.startupState == kSwitchOutputOn);
    TEST_ASSERT_TRUE(parsed.safeState == kSwitchOutputOff);
    TEST_ASSERT_TRUE(parsed.inverted);
    TEST_ASSERT_EQUAL_UINT8(21U, parsed.gpioPin);
}

void test_gpio_switch_api_adapter_partial_update_preserves_other_fields() {
    FakeGpioOutputDriver driver;
    auto runtime = makeGpioSwitchRuntime(driver);

    StaticJsonDocument<128> doc;
    JsonObject config = doc.createNestedObject("config");
    config["gpioPin"] = 19;

    assertMatchesJsonSchema("schemas/rest/v1/requests/devices-update-gpio_switch.request.schema.json", doc.as<JsonVariantConst>());

    DeviceConfigUpdateRequest request{};
    const char* error = nullptr;
    TEST_ASSERT_TRUE_MESSAGE(
        GpioSwitchDeviceApiAdapter::instance().parseUpdateConfigRequest(doc.as<JsonObjectConst>(), *runtime, request, error), error);
    TEST_ASSERT_FALSE(request.depsProvided);

    GpioSwitchDeviceConfigV3 parsed{};
    TEST_ASSERT_TRUE(
        decodeGpioSwitchDeviceConfig(reinterpret_cast<const uint8_t*>(request.configBlob.data()), request.configBlob.size(), parsed));
    TEST_ASSERT_EQUAL_UINT8(19U, parsed.gpioPin);
    TEST_ASSERT_TRUE(parsed.restorePreviousState);
    TEST_ASSERT_TRUE(parsed.inverted);
    TEST_ASSERT_TRUE(parsed.startupState == kSwitchOutputOn);
    TEST_ASSERT_TRUE(parsed.safeState == kSwitchOutputOff);
}

void test_gpio_switch_api_adapter_serializes_record() {
    FakeGpioOutputDriver driver;
    auto runtime = makeGpioSwitchRuntime(driver);

    StaticJsonDocument<1024> doc;
    JsonObject output = doc.to<JsonObject>();
    GpioSwitchDeviceApiAdapter::instance().writeDeviceJson(*runtime, runtime->status(), output);

    TEST_ASSERT_FALSE(doc.overflowed());
    assertMatchesJsonSchema("schemas/rest/v1/responses/devices-gpio_switch.response.schema.json", doc.as<JsonVariantConst>());
    TEST_ASSERT_EQUAL_UINT32(7U, output["record"]["id"].as<uint32_t>());
    TEST_ASSERT_EQUAL_STRING("gpio_switch", output["record"]["typeName"].as<const char*>());
    TEST_ASSERT_EQUAL_STRING("relay", output["config"]["name"].as<const char*>());
    TEST_ASSERT_TRUE(output["runtime"]["output"]["state"].as<bool>());
    TEST_ASSERT_FALSE(output["runtime"]["output"]["physicalLevel"].as<bool>());
}

void test_analog_port_input_api_adapter_schema_smoke() {
    StaticJsonDocument<512> createDoc;
    createDoc["typeName"] = "analog_port_input";
    JsonObject createConfig = createDoc.createNestedObject("config");
    makeAnalogPortInputConfig().writeJson(createConfig);
    assertMatchesJsonSchema("schemas/rest/v1/requests/devices-create-analog_port_input.request.schema.json",
                            createDoc.as<JsonVariantConst>());

    StaticJsonDocument<512> updateDoc;
    JsonObject updateConfig = updateDoc.createNestedObject("config");
    updateConfig["gpioPin"] = 35;
    updateConfig["pollMs"] = 250;
    assertMatchesJsonSchema("schemas/rest/v1/requests/devices-update-analog_port_input.request.schema.json",
                            updateDoc.as<JsonVariantConst>());
}

void test_analog_port_input_api_adapter_parses_create_request() {
    StaticJsonDocument<512> doc;
    doc["typeName"] = "analog_port_input";
    JsonObject config = doc.createNestedObject("config");
    makeAnalogPortInputConfig().writeJson(config);

    DeviceCreateRequest request{};
    const char* error = nullptr;
    TEST_ASSERT_TRUE_MESSAGE(AnalogPortInputDeviceApiAdapter::instance().parseCreateRequest(doc.as<JsonObjectConst>(), request, error),
                             error);
    TEST_ASSERT_EQUAL_UINT32(kAnalogPortInputTypeId, request.typeId);
    TEST_ASSERT_EQUAL_STRING("analog port", request.baseConfig.name);

    AnalogPortInputDeviceConfigV1 parsed{};
    TEST_ASSERT_TRUE(decodeValidatedFixedConfigBlob(AnalogPortInputDeviceConfigV1::kMagic,
                                                    reinterpret_cast<const uint8_t*>(request.configBlob.data()), request.configBlob.size(),
                                                    parsed));
    TEST_ASSERT_EQUAL_UINT8(34U, parsed.gpioPin);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(AdcAttenuation::Db11), parsed.attenuation);
    TEST_ASSERT_EQUAL_UINT8(4U, parsed.poll.adcSamples);
}

void test_analog_port_input_api_adapter_partial_update_preserves_other_fields() {
    FakeAdcInputDriver driver;
    AnalogPortInputDevice runtime(makeAnalogPortInputConfig(), driver);
    bindAnalogPortInputIdentity(runtime, 8020U);

    StaticJsonDocument<256> doc;
    JsonObject config = doc.createNestedObject("config");
    config["gpioPin"] = 35;
    config["pollMs"] = 250;

    assertMatchesJsonSchema("schemas/rest/v1/requests/devices-update-analog_port_input.request.schema.json", doc.as<JsonVariantConst>());

    DeviceConfigUpdateRequest request{};
    const char* error = nullptr;
    TEST_ASSERT_TRUE_MESSAGE(
        AnalogPortInputDeviceApiAdapter::instance().parseUpdateConfigRequest(doc.as<JsonObjectConst>(), runtime, request, error), error);
    TEST_ASSERT_FALSE(request.depsProvided);

    AnalogPortInputDeviceConfigV1 parsed{};
    TEST_ASSERT_TRUE(decodeValidatedFixedConfigBlob(AnalogPortInputDeviceConfigV1::kMagic,
                                                    reinterpret_cast<const uint8_t*>(request.configBlob.data()), request.configBlob.size(),
                                                    parsed));
    TEST_ASSERT_EQUAL_UINT8(35U, parsed.gpioPin);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(AdcAttenuation::Db11), parsed.attenuation);
    TEST_ASSERT_EQUAL_UINT8(4U, parsed.poll.adcSamples);
    TEST_ASSERT_EQUAL_UINT32(250U, parsed.poll.pollMs);
}

void test_analog_port_input_api_adapter_serializes_record() {
    FakeAdcInputDriver driver;
    driver.milliVolts = 1650U;
    AnalogPortInputDevice device(makeAnalogPortInputConfig(), driver);
    bindAnalogPortInputIdentity(device, 8021U);
    driveAnalogPortInputUntilReading(device);

    StaticJsonDocument<1024> doc;
    JsonObject output = doc.to<JsonObject>();
    AnalogPortInputDeviceApiAdapter::instance().writeDeviceJson(device, device.status(), output);

    TEST_ASSERT_FALSE(doc.overflowed());
    assertMatchesJsonSchema("schemas/rest/v1/responses/devices-analog_port_input.response.schema.json", doc.as<JsonVariantConst>());
    TEST_ASSERT_EQUAL_UINT32(8021U, output["record"]["id"].as<uint32_t>());
    TEST_ASSERT_EQUAL_STRING("analog_port_input", output["record"]["typeName"].as<const char*>());
    TEST_ASSERT_EQUAL_STRING("analog port", output["config"]["name"].as<const char*>());
    TEST_ASSERT_TRUE(output["runtime"]["output"]["analogInput"]["valid"].as<bool>());
    TEST_ASSERT_EQUAL_UINT32(1650U, output["runtime"]["output"]["analogInput"]["milliVolts"].as<uint32_t>());
}

void test_schedule_api_adapter_parses_create_request() {
    StaticJsonDocument<2048> doc;
    doc["typeName"] = "schedule";
    JsonObject config = doc.createNestedObject("config");
    config["name"] = "irrigation";
    config["enabled"] = true;
    JsonArray rules = config.createNestedArray("rules");
    JsonObject rule = rules.createNestedObject();
    rule["enabled"] = true;
    JsonArray weekDays = rule.createNestedArray("weekDays");
    for (int day = 1; day <= 5; ++day) { // Monday..Friday -> mask 0x3E
        weekDays.add(day);
    }
    rule["startMinuteOfDay"] = 8 * 60;
    rule["endMinuteOfDay"] = 20 * 60;
    rule["mode"] = "interval";
    rule["intervalsPerWindow"] = 3;
    rule["durationMinutes"] = 10;

    assertMatchesJsonSchema("schemas/rest/v1/requests/devices-create-schedule.request.schema.json", doc.as<JsonVariantConst>());

    DeviceCreateRequest request{};
    const char* error = nullptr;
    TEST_ASSERT_TRUE(ScheduleDeviceApiAdapter::instance().parseCreateRequest(doc.as<JsonObjectConst>(), request, error));
    TEST_ASSERT_EQUAL_UINT32(kScheduleDeviceTypeId, request.typeId);
    TEST_ASSERT_EQUAL_UINT32(kScheduleDeviceConfigVersion, request.configVersion);
    TEST_ASSERT_EQUAL_STRING("irrigation", request.baseConfig.name);
    TEST_ASSERT_TRUE(request.isEnabled());

    ScheduleDeviceConfigV1 parsed{};
    TEST_ASSERT_TRUE(decodeValidatedFixedConfigBlob(
        ScheduleDeviceConfigV1::kMagic, reinterpret_cast<const uint8_t*>(request.configBlob.data()), request.configBlob.size(), parsed));
    TEST_ASSERT_EQUAL_UINT8(1, parsed.ruleCount);
    TEST_ASSERT_EQUAL_UINT8(0x3E, parsed.rules[0].weekDays);
    TEST_ASSERT_EQUAL_UINT16(480, parsed.rules[0].startMinuteOfDay);
    TEST_ASSERT_EQUAL_UINT16(1200, parsed.rules[0].endMinuteOfDay);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(ScheduleRuleMode::Interval), parsed.rules[0].mode);
    TEST_ASSERT_EQUAL_UINT16(3, parsed.rules[0].intervalsPerWindow);
    TEST_ASSERT_EQUAL_UINT16(10, parsed.rules[0].durationMinutes);
}

void test_schedule_api_adapter_serializes_record() {
    const ScheduleDeviceConfigV1 config = makeScheduleConfig();
    const DeviceConfigBlob blob = encodeScheduleBlob(config);
    DeviceRegistryEntry record{};
    record.header.deviceId = 23U;
    record.header.typeId = kScheduleDeviceTypeId;
    record.header.configVersion = kScheduleDeviceConfigVersion;
    record.header.payloadLength = static_cast<uint32_t>(blob.size());

    ScheduleDevice device(config);
    device.bindDeviceIdentity(record, blob);
    device.begin(10U);
    device.tickFastLoop(11U);

    StaticJsonDocument<2048> doc;
    JsonObject output = doc.to<JsonObject>();
    ScheduleDeviceApiAdapter::instance().writeDeviceJson(device, device.status(), output);

    TEST_ASSERT_FALSE(doc.overflowed());
    assertMatchesJsonSchema("schemas/rest/v1/responses/devices-schedule.response.schema.json", doc.as<JsonVariantConst>());
    TEST_ASSERT_EQUAL_UINT32(23U, output["record"]["id"].as<uint32_t>());
    TEST_ASSERT_EQUAL_STRING("schedule", output["record"]["typeName"].as<const char*>());
    TEST_ASSERT_EQUAL_STRING("irrigation", output["config"]["name"].as<const char*>());
    TEST_ASSERT_FALSE(output["runtime"].containsKey("output"));
}

void test_schedule_api_adapter_rejects_missing_config() {
    StaticJsonDocument<128> doc;
    doc["typeName"] = "schedule";

    DeviceCreateRequest request{};
    const char* error = nullptr;
    TEST_ASSERT_FALSE(ScheduleDeviceApiAdapter::instance().parseCreateRequest(doc.as<JsonObjectConst>(), request, error));
    TEST_ASSERT_EQUAL_STRING("device config is required", error);
}

void test_schedule_api_adapter_rejects_non_array_rules() {
    StaticJsonDocument<256> doc;
    JsonObject config = doc.createNestedObject("config");
    config["name"] = "irrigation";
    config["rules"] = "bad";

    DeviceCreateRequest request{};
    const char* error = nullptr;
    TEST_ASSERT_FALSE(ScheduleDeviceApiAdapter::instance().parseCreateRequest(doc.as<JsonObjectConst>(), request, error));
    TEST_ASSERT_EQUAL_STRING("schedule rules must be an array", error);
}

void test_schedule_api_adapter_partial_update_preserves_rules() {
    ScheduleDeviceConfigV1 current{};
    current.enabled = 1U;
    std::snprintf(current.name, sizeof(current.name), "%s", "irrigation");
    current.ruleCount = 1;
    current.rules[0].weekDays = 0x41; // Saturday + Sunday
    current.rules[0].startMinuteOfDay = 60;
    current.rules[0].endMinuteOfDay = 120;
    ScheduleDevice runtime(current);

    StaticJsonDocument<256> doc;
    JsonObject config = doc.createNestedObject("config");
    config["name"] = "renamed";
    config["enabled"] = false;

    assertMatchesJsonSchema("schemas/rest/v1/requests/devices-update-schedule.request.schema.json", doc.as<JsonVariantConst>());

    DeviceConfigUpdateRequest request{};
    const char* error = nullptr;
    TEST_ASSERT_TRUE(ScheduleDeviceApiAdapter::instance().parseUpdateConfigRequest(doc.as<JsonObjectConst>(), runtime, request, error));
    TEST_ASSERT_EQUAL_UINT32(kScheduleDeviceConfigVersion, request.configVersion);
    TEST_ASSERT_EQUAL_STRING("renamed", request.baseConfig.name);
    TEST_ASSERT_FALSE(request.isEnabled());

    ScheduleDeviceConfigV1 parsed{};
    TEST_ASSERT_TRUE(decodeValidatedFixedConfigBlob(
        ScheduleDeviceConfigV1::kMagic, reinterpret_cast<const uint8_t*>(request.configBlob.data()), request.configBlob.size(), parsed));
    TEST_ASSERT_EQUAL_STRING("renamed", parsed.name);
    TEST_ASSERT_EQUAL_UINT8(1, parsed.ruleCount);
    TEST_ASSERT_EQUAL_UINT8(0x41, parsed.rules[0].weekDays);
    TEST_ASSERT_EQUAL_UINT16(60, parsed.rules[0].startMinuteOfDay);
    TEST_ASSERT_EQUAL_UINT16(120, parsed.rules[0].endMinuteOfDay);
}

void test_schedule_api_adapter_rejects_missing_update_config() {
    ScheduleDeviceConfigV1 current{};
    ScheduleDevice runtime(current);

    StaticJsonDocument<64> doc;
    doc["command"] = "updateConfig";

    DeviceConfigUpdateRequest request{};
    const char* error = nullptr;
    TEST_ASSERT_FALSE(ScheduleDeviceApiAdapter::instance().parseUpdateConfigRequest(doc.as<JsonObjectConst>(), runtime, request, error));
    TEST_ASSERT_EQUAL_STRING("device config is required", error);
}

void test_ads1115_hub_api_adapter_parses_create_request() {
    StaticJsonDocument<512> doc;
    doc["typeName"] = "ads1115_hub";
    JsonObject config = doc.createNestedObject("config");
    config["name"] = "ADS1115";
    config["enabled"] = true;
    config["i2cAddress"] = 72;
    config["gain"] = "fsr4096";
    config["dataRateSps"] = "250";
    JsonArray deps = config.createNestedArray("deps");
    JsonObject busDep = deps.createNestedObject();
    busDep["role"] = "i2c_bus";
    busDep["deviceId"] = 17;

    assertMatchesJsonSchema("schemas/rest/v1/requests/devices-create-ads1115_hub.request.schema.json", doc.as<JsonVariantConst>());

    DeviceCreateRequest request{};
    const char* error = nullptr;
    TEST_ASSERT_TRUE(Ads1115HubDeviceApiAdapter::instance().parseCreateRequest(doc.as<JsonObjectConst>(), request, error));
    TEST_ASSERT_EQUAL_UINT32(kAds1115HubTypeId, request.typeId);
    TEST_ASSERT_EQUAL_UINT32(kAds1115HubConfigVersion, request.configVersion);
    TEST_ASSERT_EQUAL_STRING("ADS1115", request.baseConfig.name);
    TEST_ASSERT_TRUE(request.isEnabled());
    TEST_ASSERT_EQUAL_UINT8(1U, request.dependencyCount());
    TEST_ASSERT_EQUAL_UINT32(17U, request.dependencyLinks()[0].deviceId);

    Ads1115HubDeviceConfigV1 parsed{};
    TEST_ASSERT_TRUE(decodeValidatedFixedConfigBlob(
        Ads1115HubDeviceConfigV1::kMagic, reinterpret_cast<const uint8_t*>(request.configBlob.data()), request.configBlob.size(), parsed));
    TEST_ASSERT_EQUAL_UINT8(72U, parsed.i2cAddress);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Ads1115Gain::Fsr4096), parsed.gain);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Ads1115DataRate::Sps250), parsed.dataRateSps);
}

void test_ads1115_hub_api_adapter_partial_update_preserves_bus_and_gain() {
    Ads1115HubDeviceConfigV1 current = makeAds1115HubConfig();
    Ads1115HubDevice runtime(current);

    StaticJsonDocument<256> doc;
    JsonObject config = doc.createNestedObject("config");
    config["gain"] = "fsr0256";

    assertMatchesJsonSchema("schemas/rest/v1/requests/devices-update-ads1115_hub.request.schema.json", doc.as<JsonVariantConst>());

    DeviceConfigUpdateRequest request{};
    const char* error = nullptr;
    TEST_ASSERT_TRUE(Ads1115HubDeviceApiAdapter::instance().parseUpdateConfigRequest(doc.as<JsonObjectConst>(), runtime, request, error));
    TEST_ASSERT_EQUAL_UINT32(kAds1115HubConfigVersion, request.configVersion);
    TEST_ASSERT_FALSE(request.depsProvided);

    Ads1115HubDeviceConfigV1 parsed{};
    TEST_ASSERT_TRUE(decodeValidatedFixedConfigBlob(
        Ads1115HubDeviceConfigV1::kMagic, reinterpret_cast<const uint8_t*>(request.configBlob.data()), request.configBlob.size(), parsed));
    TEST_ASSERT_EQUAL_UINT8(current.i2cAddress, parsed.i2cAddress);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(Ads1115Gain::Fsr0256), parsed.gain);
    TEST_ASSERT_EQUAL_UINT8(current.dataRateSps, parsed.dataRateSps);
}

void test_ads1115_hub_api_adapter_serializes_record() {
    const Ads1115HubDeviceConfigV1 config = makeAds1115HubConfig();
    const DeviceConfigBlob blob = encodeAds1115HubBlob(config);
    DeviceRegistryEntry record{};
    record.header.deviceId = 55U;
    record.header.typeId = kAds1115HubTypeId;
    record.header.configVersion = kAds1115HubConfigVersion;
    record.header.payloadLength = static_cast<uint32_t>(blob.size());

    Ads1115HubDevice device(config);
    device.bindDeviceIdentity(record, blob);

    StaticJsonDocument<1024> doc;
    JsonObject output = doc.to<JsonObject>();
    Ads1115HubDeviceApiAdapter::instance().writeDeviceJson(device, device.status(), output);

    TEST_ASSERT_FALSE(doc.overflowed());
    assertMatchesJsonSchema("schemas/rest/v1/responses/devices-ads1115_hub.response.schema.json", doc.as<JsonVariantConst>());
    TEST_ASSERT_EQUAL_UINT32(55U, output["record"]["id"].as<uint32_t>());
    TEST_ASSERT_EQUAL_STRING("ads1115_hub", output["record"]["typeName"].as<const char*>());
    TEST_ASSERT_EQUAL_STRING("ADS1115", output["config"]["name"].as<const char*>());
    TEST_ASSERT_FALSE(output["runtime"].containsKey("output"));
}

void test_cd74hc4067_hub_api_adapter_parses_create_request() {
    StaticJsonDocument<512> doc;
    doc["typeName"] = "cd74hc4067_hub";
    JsonObject config = doc.createNestedObject("config");
    config["name"] = "MUX";
    config["enabled"] = true;
    JsonArray selectPins = config.createNestedArray("selectPins");
    selectPins.add(16);
    selectPins.add(17);
    selectPins.add(18);
    selectPins.add(19);
    config["enablePin"] = 255;
    config["sigPin"] = 34;
    config["sigAttenuation"] = "11db";

    assertMatchesJsonSchema("schemas/rest/v1/requests/devices-create-cd74hc4067_hub.request.schema.json", doc.as<JsonVariantConst>());

    DeviceCreateRequest request{};
    const char* error = nullptr;
    TEST_ASSERT_TRUE(Cd74hc4067HubDeviceApiAdapter::instance().parseCreateRequest(doc.as<JsonObjectConst>(), request, error));
    TEST_ASSERT_EQUAL_UINT32(kCd74hc4067HubTypeId, request.typeId);
    TEST_ASSERT_EQUAL_UINT32(kCd74hc4067HubConfigVersion, request.configVersion);
    TEST_ASSERT_EQUAL_STRING("MUX", request.baseConfig.name);
    TEST_ASSERT_TRUE(request.isEnabled());

    Cd74hc4067HubDeviceConfigV1 parsed{};
    TEST_ASSERT_TRUE(decodeValidatedFixedConfigBlob(Cd74hc4067HubDeviceConfigV1::kMagic,
                                                    reinterpret_cast<const uint8_t*>(request.configBlob.data()), request.configBlob.size(),
                                                    parsed));
    TEST_ASSERT_EQUAL_UINT8(16U, parsed.selectPins[0]);
    TEST_ASSERT_EQUAL_UINT8(17U, parsed.selectPins[1]);
    TEST_ASSERT_EQUAL_UINT8(18U, parsed.selectPins[2]);
    TEST_ASSERT_EQUAL_UINT8(19U, parsed.selectPins[3]);
    TEST_ASSERT_EQUAL_UINT8(kGpioPinUnset, parsed.enablePin);
    TEST_ASSERT_EQUAL_UINT8(34U, parsed.sigPin);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(AdcAttenuation::Db11), parsed.sigAttenuation);
}

void test_cd74hc4067_hub_api_adapter_partial_update_preserves_select_pins() {
    Cd74hc4067HubDeviceConfigV1 current = makeCd74hc4067HubConfig();
    Cd74hc4067HubDevice runtime(current);

    StaticJsonDocument<256> doc;
    JsonObject config = doc.createNestedObject("config");
    config["sigAttenuation"] = "6db";

    assertMatchesJsonSchema("schemas/rest/v1/requests/devices-update-cd74hc4067_hub.request.schema.json", doc.as<JsonVariantConst>());

    DeviceConfigUpdateRequest request{};
    const char* error = nullptr;
    TEST_ASSERT_TRUE(
        Cd74hc4067HubDeviceApiAdapter::instance().parseUpdateConfigRequest(doc.as<JsonObjectConst>(), runtime, request, error));
    TEST_ASSERT_EQUAL_UINT32(kCd74hc4067HubConfigVersion, request.configVersion);

    Cd74hc4067HubDeviceConfigV1 parsed{};
    TEST_ASSERT_TRUE(decodeValidatedFixedConfigBlob(Cd74hc4067HubDeviceConfigV1::kMagic,
                                                    reinterpret_cast<const uint8_t*>(request.configBlob.data()), request.configBlob.size(),
                                                    parsed));
    TEST_ASSERT_EQUAL_UINT8(current.selectPins[0], parsed.selectPins[0]);
    TEST_ASSERT_EQUAL_UINT8(current.selectPins[1], parsed.selectPins[1]);
    TEST_ASSERT_EQUAL_UINT8(current.selectPins[2], parsed.selectPins[2]);
    TEST_ASSERT_EQUAL_UINT8(current.selectPins[3], parsed.selectPins[3]);
    TEST_ASSERT_EQUAL_UINT8(current.enablePin, parsed.enablePin);
    TEST_ASSERT_EQUAL_UINT8(current.sigPin, parsed.sigPin);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(AdcAttenuation::Db6), parsed.sigAttenuation);
}

void test_cd74hc4067_hub_api_adapter_serializes_record() {
    const Cd74hc4067HubDeviceConfigV1 config = makeCd74hc4067HubConfig();
    const DeviceConfigBlob blob = encodeCd74hc4067HubBlob(config);
    DeviceRegistryEntry record{};
    record.header.deviceId = 57U;
    record.header.typeId = kCd74hc4067HubTypeId;
    record.header.configVersion = kCd74hc4067HubConfigVersion;
    record.header.payloadLength = static_cast<uint32_t>(blob.size());

    Cd74hc4067HubDevice device(config);
    device.bindDeviceIdentity(record, blob);

    StaticJsonDocument<1024> doc;
    JsonObject output = doc.to<JsonObject>();
    Cd74hc4067HubDeviceApiAdapter::instance().writeDeviceJson(device, device.status(), output);

    TEST_ASSERT_FALSE(doc.overflowed());
    assertMatchesJsonSchema("schemas/rest/v1/responses/devices-cd74hc4067_hub.response.schema.json", doc.as<JsonVariantConst>());
    TEST_ASSERT_EQUAL_UINT32(57U, output["record"]["id"].as<uint32_t>());
    TEST_ASSERT_EQUAL_STRING("cd74hc4067_hub", output["record"]["typeName"].as<const char*>());
    TEST_ASSERT_EQUAL_STRING("MUX", output["config"]["name"].as<const char*>());
    TEST_ASSERT_FALSE(output["runtime"].containsKey("output"));
}

void test_pcf8574_expander_api_adapter_parses_create_request() {
    StaticJsonDocument<512> doc;
    doc["typeName"] = "pcf8574_expander";
    JsonObject config = doc.createNestedObject("config");
    config["name"] = "PCF8574";
    config["enabled"] = true;
    config["i2cAddress"] = 32;
    config["inverted"] = true;
    JsonArray deps = config.createNestedArray("deps");
    JsonObject busDep = deps.createNestedObject();
    busDep["role"] = "i2c_bus";
    busDep["deviceId"] = 17;

    assertMatchesJsonSchema("schemas/rest/v1/requests/devices-create-pcf8574_expander.request.schema.json", doc.as<JsonVariantConst>());

    DeviceCreateRequest request{};
    const char* error = nullptr;
    TEST_ASSERT_TRUE(Pcf8574ExpanderDeviceApiAdapter::instance().parseCreateRequest(doc.as<JsonObjectConst>(), request, error));
    TEST_ASSERT_EQUAL_UINT32(12U, request.typeId);
    TEST_ASSERT_EQUAL_UINT32(kPcf857xExpanderConfigVersion, request.configVersion);
    TEST_ASSERT_EQUAL_STRING("PCF8574", request.baseConfig.name);
    TEST_ASSERT_TRUE(request.isEnabled());
    TEST_ASSERT_EQUAL_UINT8(1U, request.dependencyCount());
    TEST_ASSERT_EQUAL_UINT32(17U, request.dependencyLinks()[0].deviceId);

    Pcf857xExpanderConfigV2 parsed{};
    TEST_ASSERT_TRUE(
        decodePcf857xExpanderConfig(reinterpret_cast<const uint8_t*>(request.configBlob.data()), request.configBlob.size(), parsed));
    TEST_ASSERT_EQUAL_UINT8(32U, parsed.i2cAddress);
    TEST_ASSERT_EQUAL_UINT8(1U, parsed.inverted);
}

void test_pcf8574_expander_api_adapter_partial_update_preserves_address() {
    Pcf857xExpanderConfigV2 current = makePcf8574ExpanderConfig();
    Pcf8574ExpanderDevice runtime(current);

    StaticJsonDocument<256> doc;
    JsonObject config = doc.createNestedObject("config");
    config["inverted"] = false;
    JsonArray deps = doc.createNestedArray("deps");
    JsonObject busDep = deps.createNestedObject();
    busDep["role"] = "i2c_bus";
    busDep["deviceId"] = 17;

    assertMatchesJsonSchema("schemas/rest/v1/requests/devices-update-pcf8574_expander.request.schema.json", doc.as<JsonVariantConst>());

    DeviceConfigUpdateRequest request{};
    const char* error = nullptr;
    TEST_ASSERT_TRUE(
        Pcf8574ExpanderDeviceApiAdapter::instance().parseUpdateConfigRequest(doc.as<JsonObjectConst>(), runtime, request, error));
    TEST_ASSERT_EQUAL_UINT32(kPcf857xExpanderConfigVersion, request.configVersion);
    TEST_ASSERT_TRUE(request.depsProvided);

    Pcf857xExpanderConfigV2 parsed{};
    TEST_ASSERT_TRUE(
        decodePcf857xExpanderConfig(reinterpret_cast<const uint8_t*>(request.configBlob.data()), request.configBlob.size(), parsed));
    TEST_ASSERT_EQUAL_UINT8(current.i2cAddress, parsed.i2cAddress);
    TEST_ASSERT_EQUAL_UINT8(0U, parsed.inverted);
}

void test_pcf8574_expander_api_adapter_serializes_record() {
    const Pcf857xExpanderConfigV2 config = makePcf8574ExpanderConfig();
    const DeviceConfigBlob blob = encodePcf8574ExpanderBlob(config);
    DeviceRegistryEntry record{};
    record.header.deviceId = 61U;
    record.header.typeId = 12U;
    record.header.configVersion = kPcf857xExpanderConfigVersion;
    record.header.payloadLength = static_cast<uint32_t>(blob.size());

    Pcf8574ExpanderDevice device(config);
    device.bindDeviceIdentity(record, blob);

    StaticJsonDocument<1024> doc;
    JsonObject output = doc.to<JsonObject>();
    Pcf8574ExpanderDeviceApiAdapter::instance().writeDeviceJson(device, device.status(), output);

    TEST_ASSERT_FALSE(doc.overflowed());
    assertMatchesJsonSchema("schemas/rest/v1/responses/devices-pcf8574_expander.response.schema.json", doc.as<JsonVariantConst>());
    TEST_ASSERT_EQUAL_UINT32(61U, output["record"]["id"].as<uint32_t>());
    TEST_ASSERT_EQUAL_STRING("pcf8574_expander", output["record"]["typeName"].as<const char*>());
    TEST_ASSERT_EQUAL_STRING("PCF8574", output["config"]["name"].as<const char*>());
    TEST_ASSERT_EQUAL_UINT32(8U, output["runtime"]["channelCount"].as<uint32_t>());
    TEST_ASSERT_EQUAL_UINT32(0U, output["runtime"]["channelStates"].as<uint32_t>());
    TEST_ASSERT_FALSE(output["runtime"].containsKey("output"));
}

void test_pcf8575_expander_api_adapter_parses_create_request() {
    StaticJsonDocument<512> doc;
    doc["typeName"] = "pcf8575_expander";
    JsonObject config = doc.createNestedObject("config");
    config["name"] = "PCF8575";
    config["enabled"] = true;
    config["i2cAddress"] = 32;
    config["inverted"] = false;
    JsonArray deps = config.createNestedArray("deps");
    JsonObject busDep = deps.createNestedObject();
    busDep["role"] = "i2c_bus";
    busDep["deviceId"] = 17;

    assertMatchesJsonSchema("schemas/rest/v1/requests/devices-create-pcf8575_expander.request.schema.json", doc.as<JsonVariantConst>());

    DeviceCreateRequest request{};
    const char* error = nullptr;
    TEST_ASSERT_TRUE(Pcf8575ExpanderDeviceApiAdapter::instance().parseCreateRequest(doc.as<JsonObjectConst>(), request, error));
    TEST_ASSERT_EQUAL_UINT32(13U, request.typeId);
    TEST_ASSERT_EQUAL_UINT32(kPcf857xExpanderConfigVersion, request.configVersion);
    TEST_ASSERT_EQUAL_STRING("PCF8575", request.baseConfig.name);
    TEST_ASSERT_TRUE(request.isEnabled());
    TEST_ASSERT_EQUAL_UINT8(1U, request.dependencyCount());
    TEST_ASSERT_EQUAL_UINT32(17U, request.dependencyLinks()[0].deviceId);

    Pcf857xExpanderConfigV2 parsed{};
    TEST_ASSERT_TRUE(
        decodePcf857xExpanderConfig(reinterpret_cast<const uint8_t*>(request.configBlob.data()), request.configBlob.size(), parsed));
    TEST_ASSERT_EQUAL_UINT8(32U, parsed.i2cAddress);
    TEST_ASSERT_EQUAL_UINT8(0U, parsed.inverted);
}

void test_pcf8575_expander_api_adapter_partial_update_preserves_address() {
    Pcf857xExpanderConfigV2 current = makePcf8575ExpanderConfig();
    Pcf8575ExpanderDevice runtime(current);

    StaticJsonDocument<256> doc;
    JsonObject config = doc.createNestedObject("config");
    config["inverted"] = true;
    JsonArray deps = doc.createNestedArray("deps");
    JsonObject busDep = deps.createNestedObject();
    busDep["role"] = "i2c_bus";
    busDep["deviceId"] = 17;

    assertMatchesJsonSchema("schemas/rest/v1/requests/devices-update-pcf8575_expander.request.schema.json", doc.as<JsonVariantConst>());

    DeviceConfigUpdateRequest request{};
    const char* error = nullptr;
    TEST_ASSERT_TRUE(
        Pcf8575ExpanderDeviceApiAdapter::instance().parseUpdateConfigRequest(doc.as<JsonObjectConst>(), runtime, request, error));
    TEST_ASSERT_EQUAL_UINT32(kPcf857xExpanderConfigVersion, request.configVersion);
    TEST_ASSERT_TRUE(request.depsProvided);

    Pcf857xExpanderConfigV2 parsed{};
    TEST_ASSERT_TRUE(
        decodePcf857xExpanderConfig(reinterpret_cast<const uint8_t*>(request.configBlob.data()), request.configBlob.size(), parsed));
    TEST_ASSERT_EQUAL_UINT8(current.i2cAddress, parsed.i2cAddress);
    TEST_ASSERT_EQUAL_UINT8(1U, parsed.inverted);
}

void test_pcf8575_expander_api_adapter_serializes_record() {
    const Pcf857xExpanderConfigV2 config = makePcf8575ExpanderConfig();
    const DeviceConfigBlob blob = encodePcf8575ExpanderBlob(config);
    DeviceRegistryEntry record{};
    record.header.deviceId = 62U;
    record.header.typeId = 13U;
    record.header.configVersion = kPcf857xExpanderConfigVersion;
    record.header.payloadLength = static_cast<uint32_t>(blob.size());

    Pcf8575ExpanderDevice device(config);
    device.bindDeviceIdentity(record, blob);

    StaticJsonDocument<1024> doc;
    JsonObject output = doc.to<JsonObject>();
    Pcf8575ExpanderDeviceApiAdapter::instance().writeDeviceJson(device, device.status(), output);

    TEST_ASSERT_FALSE(doc.overflowed());
    assertMatchesJsonSchema("schemas/rest/v1/responses/devices-pcf8575_expander.response.schema.json", doc.as<JsonVariantConst>());
    TEST_ASSERT_EQUAL_UINT32(62U, output["record"]["id"].as<uint32_t>());
    TEST_ASSERT_EQUAL_STRING("pcf8575_expander", output["record"]["typeName"].as<const char*>());
    TEST_ASSERT_EQUAL_STRING("PCF8575", output["config"]["name"].as<const char*>());
    TEST_ASSERT_EQUAL_UINT32(16U, output["runtime"]["channelCount"].as<uint32_t>());
    TEST_ASSERT_EQUAL_UINT32(0U, output["runtime"]["channelStates"].as<uint32_t>());
    TEST_ASSERT_FALSE(output["runtime"].containsKey("output"));
}

void test_port_expander_switch_api_adapter_parses_create_request() {
    StaticJsonDocument<512> doc;
    doc["typeName"] = "port_expander_switch";
    JsonObject config = doc.createNestedObject("config");
    config["name"] = "ch5";
    config["enabled"] = true;
    config["restorePreviousState"] = true;
    config["startupState"] = true;
    config["safeState"] = false;
    config["inverted"] = false;
    config["channel"] = 5;
    JsonArray deps = config.createNestedArray("deps");
    JsonObject expanderDep = deps.createNestedObject();
    expanderDep["role"] = "port_expander";
    expanderDep["deviceId"] = 17;

    assertMatchesJsonSchema("schemas/rest/v1/requests/devices-create-port_expander_switch.request.schema.json", doc.as<JsonVariantConst>());

    DeviceCreateRequest request{};
    const char* error = nullptr;
    TEST_ASSERT_TRUE(PortExpanderSwitchDeviceApiAdapter::instance().parseCreateRequest(doc.as<JsonObjectConst>(), request, error));
    TEST_ASSERT_EQUAL_UINT32(14U, request.typeId);
    TEST_ASSERT_EQUAL_UINT32(3U, request.configVersion);
    TEST_ASSERT_EQUAL_STRING("ch5", request.baseConfig.name);
    TEST_ASSERT_TRUE(request.isEnabled());
    TEST_ASSERT_EQUAL_UINT8(1U, request.dependencyCount());
    TEST_ASSERT_EQUAL_UINT32(17U, request.dependencyLinks()[0].deviceId);

    PortExpanderSwitchDeviceConfigV3 parsed{};
    TEST_ASSERT_TRUE(decodePortExpanderSwitchDeviceConfig(reinterpret_cast<const uint8_t*>(request.configBlob.data()),
                                                          request.configBlob.size(), parsed));
    TEST_ASSERT_EQUAL_UINT8(5U, parsed.channel);
    TEST_ASSERT_TRUE(parsed.restorePreviousState);
    TEST_ASSERT_TRUE(parsed.startupState);
    TEST_ASSERT_FALSE(parsed.safeState);
}

void test_port_expander_switch_api_adapter_partial_update_preserves_channel() {
    PortExpanderSwitchDeviceConfigV3 current = makePortExpanderSwitchConfig();
    PortExpanderSwitchDevice runtime(current);

    StaticJsonDocument<256> doc;
    JsonObject config = doc.createNestedObject("config");
    config["inverted"] = true;
    JsonArray deps = doc.createNestedArray("deps");
    JsonObject expanderDep = deps.createNestedObject();
    expanderDep["role"] = "port_expander";
    expanderDep["deviceId"] = 17;

    assertMatchesJsonSchema("schemas/rest/v1/requests/devices-update-port_expander_switch.request.schema.json", doc.as<JsonVariantConst>());

    DeviceConfigUpdateRequest request{};
    const char* error = nullptr;
    TEST_ASSERT_TRUE(
        PortExpanderSwitchDeviceApiAdapter::instance().parseUpdateConfigRequest(doc.as<JsonObjectConst>(), runtime, request, error));
    TEST_ASSERT_EQUAL_UINT32(3U, request.configVersion);
    TEST_ASSERT_TRUE(request.depsProvided);

    PortExpanderSwitchDeviceConfigV3 parsed{};
    TEST_ASSERT_TRUE(decodePortExpanderSwitchDeviceConfig(reinterpret_cast<const uint8_t*>(request.configBlob.data()),
                                                          request.configBlob.size(), parsed));
    TEST_ASSERT_EQUAL_UINT8(current.channel, parsed.channel);
    TEST_ASSERT_TRUE(parsed.inverted);
}

void test_port_expander_switch_api_adapter_serializes_record() {
    const PortExpanderSwitchDeviceConfigV3 config = makePortExpanderSwitchConfig();
    const DeviceConfigBlob blob = encodePortExpanderSwitchBlob(config);
    DeviceRegistryEntry record{};
    record.header.deviceId = 63U;
    record.header.typeId = 14U;
    record.header.configVersion = 3U;
    record.header.payloadLength = static_cast<uint32_t>(blob.size());

    PortExpanderSwitchDevice device(config);
    device.bindDeviceIdentity(record, blob);

    StaticJsonDocument<1024> doc;
    JsonObject output = doc.to<JsonObject>();
    PortExpanderSwitchDeviceApiAdapter::instance().writeDeviceJson(device, device.status(), output);

    TEST_ASSERT_FALSE(doc.overflowed());
    assertMatchesJsonSchema("schemas/rest/v1/responses/devices-port_expander_switch.response.schema.json", doc.as<JsonVariantConst>());
    TEST_ASSERT_EQUAL_UINT32(63U, output["record"]["id"].as<uint32_t>());
    TEST_ASSERT_EQUAL_STRING("port_expander_switch", output["record"]["typeName"].as<const char*>());
    TEST_ASSERT_EQUAL_STRING("ch5", output["config"]["name"].as<const char*>());
    TEST_ASSERT_EQUAL_UINT32(5U, output["config"]["channel"].as<uint32_t>());
    TEST_ASSERT_TRUE(output["runtime"]["output"]["state"].is<bool>());
    TEST_ASSERT_TRUE(output["runtime"]["output"]["physicalLevel"].is<bool>());
}

void test_thermostat_api_adapter_parses_create_request() {
    StaticJsonDocument<2048> doc;
    doc["typeName"] = "thermostat";
    JsonObject config = doc.createNestedObject("config");
    config["name"] = "Tank";
    config["enabled"] = true;
    config["mode"] = "heat";
    config["algorithm"] = "hysteresis";
    config["targetCelsius"] = 25.0;
    config["minSafeCelsius"] = 0.0;
    config["maxSafeCelsius"] = 50.0;
    config["hysteresisCelsius"] = 0.5;
    config["checkIntervalMs"] = 1000;
    config["sensorTimeoutMs"] = 3000;
    config["retryAfterErrorMs"] = 30000;
    config["minSwitchIntervalMs"] = 5000;
    JsonArray deps = config.createNestedArray("deps");
    JsonObject temperatureDep = deps.createNestedObject();
    temperatureDep["role"] = "temperature_sensor";
    temperatureDep["deviceId"] = 40;
    JsonObject switchDep = deps.createNestedObject();
    switchDep["role"] = "switch";
    switchDep["deviceId"] = 41;

    assertMatchesJsonSchema("schemas/rest/v1/requests/devices-create-thermostat.request.schema.json", doc.as<JsonVariantConst>());

    DeviceCreateRequest request{};
    const char* error = nullptr;
    TEST_ASSERT_TRUE(ThermostatDeviceApiAdapter::instance().parseCreateRequest(doc.as<JsonObjectConst>(), request, error));
    TEST_ASSERT_EQUAL_UINT32(kThermostatDeviceTypeId, request.typeId);
    TEST_ASSERT_EQUAL_UINT32(kThermostatDeviceConfigVersion, request.configVersion);
    TEST_ASSERT_EQUAL_STRING("Tank", request.baseConfig.name);
    TEST_ASSERT_TRUE(request.isEnabled());
    TEST_ASSERT_EQUAL_UINT8(2U, request.dependencyCount());
    TEST_ASSERT_EQUAL_UINT32(40U, request.dependencyLinks()[0].deviceId);
    TEST_ASSERT_EQUAL_UINT32(41U, request.dependencyLinks()[1].deviceId);

    ThermostatDeviceConfigV1 parsed{};
    TEST_ASSERT_TRUE(decodeValidatedFixedConfigBlob(
        ThermostatDeviceConfigV1::kMagic, reinterpret_cast<const uint8_t*>(request.configBlob.data()), request.configBlob.size(), parsed));
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(ThermostatMode::Heat), parsed.mode);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(ThermostatAlgorithm::Hysteresis), parsed.algorithm);
    TEST_ASSERT_EQUAL_INT32(25000, parsed.targetMilliCelsius);
    TEST_ASSERT_EQUAL_INT32(0, parsed.minSafeMilliCelsius);
    TEST_ASSERT_EQUAL_INT32(50000, parsed.maxSafeMilliCelsius);
    TEST_ASSERT_EQUAL_UINT16(50U, parsed.hysteresisCentiCelsius);
    TEST_ASSERT_EQUAL_UINT32(1000U, parsed.checkIntervalMs);
    TEST_ASSERT_EQUAL_UINT32(3000U, parsed.sensorTimeoutMs);
    TEST_ASSERT_EQUAL_UINT32(30000U, parsed.retryAfterErrorMs);
    TEST_ASSERT_EQUAL_UINT32(5000U, parsed.minSwitchIntervalMs);
}

void test_thermostat_api_adapter_validates_update_request_schema() {
    StaticJsonDocument<512> doc;
    JsonObject config = doc.createNestedObject("config");
    config["targetCelsius"] = 24.5;
    config["hysteresisCelsius"] = 0.75;
    JsonArray deps = doc.createNestedArray("deps");
    JsonObject temperatureDep = deps.createNestedObject();
    temperatureDep["role"] = "temperature_sensor";
    temperatureDep["deviceId"] = 40;
    JsonObject switchDep = deps.createNestedObject();
    switchDep["role"] = "switch";
    switchDep["deviceId"] = 41;

    assertMatchesJsonSchema("schemas/rest/v1/requests/devices-update-thermostat.request.schema.json", doc.as<JsonVariantConst>());
}

void test_thermostat_api_adapter_serializes_record() {
    const ThermostatDeviceConfigV1 config = makeThermostatConfig();
    const DeviceConfigBlob blob = encodeThermostatBlob(config);
    DeviceRegistryEntry record{};
    record.header.deviceId = 44U;
    record.header.typeId = kThermostatDeviceTypeId;
    record.header.configVersion = kThermostatDeviceConfigVersion;
    record.header.payloadLength = static_cast<uint32_t>(blob.size());

    ThermostatDevice device(config);
    device.bindDeviceIdentity(record, blob);
    device.begin(10U);
    device.tickFastLoop(11U);

    StaticJsonDocument<2048> doc;
    JsonObject output = doc.to<JsonObject>();
    ThermostatDeviceApiAdapter::instance().writeDeviceJson(device, device.status(), output);

    TEST_ASSERT_FALSE(doc.overflowed());
    assertMatchesJsonSchema("schemas/rest/v1/responses/devices-thermostat.response.schema.json", doc.as<JsonVariantConst>());
    TEST_ASSERT_EQUAL_UINT32(44U, output["record"]["id"].as<uint32_t>());
    TEST_ASSERT_EQUAL_STRING("thermostat", output["record"]["typeName"].as<const char*>());
    TEST_ASSERT_EQUAL_STRING("Tank", output["config"]["name"].as<const char*>());
    TEST_ASSERT_TRUE(output["runtime"]["output"]["temperature"].is<JsonObjectConst>());
    TEST_ASSERT_TRUE(output["runtime"]["output"]["controlStatus"].is<const char*>());
}

void test_ntc_thermistor_api_adapter_parses_create_request() {
    StaticJsonDocument<512> doc;
    doc["typeName"] = "ntc_thermistor_temperature_sensor";
    JsonObject config = doc.createNestedObject("config");
    config["name"] = "boiler probe";
    config["enabled"] = true;
    config["formulaMode"] = "beta";
    config["seriesResistorOhms"] = 9800;
    config["supplyMilliVolts"] = 3300;
    config["nominalResistanceOhms"] = 100000;
    config["betaCoefficient"] = 3950;
    JsonArray deps = config.createNestedArray("deps");
    JsonObject analogInputDep = deps.createNestedObject();
    analogInputDep["role"] = "analog_input";
    analogInputDep["deviceId"] = 42;

    assertMatchesJsonSchema("schemas/rest/v1/requests/devices-create-ntc_thermistor_temperature_sensor.request.schema.json",
                            doc.as<JsonVariantConst>());

    DeviceCreateRequest request{};
    const char* error = nullptr;
    TEST_ASSERT_TRUE_MESSAGE(
        NtcThermistorTemperatureSensorDeviceApiAdapter::instance().parseCreateRequest(doc.as<JsonObjectConst>(), request, error), error);
    TEST_ASSERT_EQUAL_UINT32(kNtcThermistorTemperatureSensorTypeId, request.typeId);
    TEST_ASSERT_EQUAL_UINT32(kNtcThermistorTemperatureSensorConfigVersion, request.configVersion);
    TEST_ASSERT_EQUAL_STRING("boiler probe", request.baseConfig.name);
    TEST_ASSERT_TRUE(request.isEnabled());

    NtcThermistorTemperatureSensorConfigV1 parsed{};
    TEST_ASSERT_TRUE(decodeValidatedFixedConfigBlob(NtcThermistorTemperatureSensorConfigV1::kMagic,
                                                    reinterpret_cast<const uint8_t*>(request.configBlob.data()), request.configBlob.size(),
                                                    parsed));
    TEST_ASSERT_EQUAL_UINT16(9800, parsed.seriesResistorOhms);
    TEST_ASSERT_EQUAL_UINT16(3300, parsed.supplyMilliVolts);
    TEST_ASSERT_EQUAL_UINT32(100000, parsed.nominalResistanceOhms);
    TEST_ASSERT_EQUAL_UINT16(3950, parsed.betaCoefficient);
}

void test_ntc_thermistor_api_adapter_rejects_missing_config() {
    StaticJsonDocument<128> doc;
    doc["typeName"] = "ntc_thermistor_temperature_sensor";

    DeviceCreateRequest request{};
    const char* error = nullptr;
    TEST_ASSERT_FALSE(
        NtcThermistorTemperatureSensorDeviceApiAdapter::instance().parseCreateRequest(doc.as<JsonObjectConst>(), request, error));
    TEST_ASSERT_EQUAL_STRING("device config is required", error);
}

void test_ntc_thermistor_api_adapter_rejects_invalid_config() {
    StaticJsonDocument<256> doc;
    JsonObject config = doc.createNestedObject("config");
    config["name"] = "boiler probe";
    config["seriesResistorOhms"] = 0; // parseJson runs validate() itself and surfaces its message

    DeviceCreateRequest request{};
    const char* error = nullptr;
    TEST_ASSERT_FALSE(
        NtcThermistorTemperatureSensorDeviceApiAdapter::instance().parseCreateRequest(doc.as<JsonObjectConst>(), request, error));
    TEST_ASSERT_EQUAL_STRING("ntc thermistor series resistor is invalid", error);
}

void test_binary_sensor_api_adapter_parses_create_request() {
    StaticJsonDocument<512> doc;
    doc["typeName"] = "binary_sensor";
    JsonObject config = doc.createNestedObject("config");
    config["name"] = "reagent low level";
    config["enabled"] = true;
    config["gpioPin"] = 27;
    config["pullMode"] = "pulldown";
    config["inverted"] = true;
    config["debounceMs"] = 120;

    assertMatchesJsonSchema("schemas/rest/v1/requests/devices-create-binary_sensor.request.schema.json", doc.as<JsonVariantConst>());

    DeviceCreateRequest request{};
    const char* error = nullptr;
    TEST_ASSERT_TRUE(BinarySensorDeviceApiAdapter::instance().parseCreateRequest(doc.as<JsonObjectConst>(), request, error));
    TEST_ASSERT_EQUAL_UINT32(kBinarySensorDeviceTypeId, request.typeId);
    TEST_ASSERT_EQUAL_UINT32(kBinarySensorDeviceConfigVersion, request.configVersion);
    TEST_ASSERT_EQUAL_STRING("reagent low level", request.baseConfig.name);
    TEST_ASSERT_TRUE(request.isEnabled());

    BinarySensorDeviceConfigV1 parsed{};
    TEST_ASSERT_TRUE(decodeValidatedFixedConfigBlob(BinarySensorDeviceConfigV1::kMagic,
                                                    reinterpret_cast<const uint8_t*>(request.configBlob.data()), request.configBlob.size(),
                                                    parsed));
    TEST_ASSERT_EQUAL_UINT8(27, parsed.gpioPin);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(GpioInputPullMode::PullDown), parsed.pullMode);
    TEST_ASSERT_EQUAL_UINT8(1U, parsed.inverted);
    TEST_ASSERT_EQUAL_UINT16(120, parsed.debounceMs);
}

void test_binary_sensor_api_adapter_serializes_record() {
    FakeBinarySensorDriver driver;
    driver.level = true;
    const BinarySensorDeviceConfigV1 config = makeBinarySensorConfig();
    BinarySensorDevice device(config, driver);
    DeviceRegistryEntry record{};
    record.header.deviceId = 27U;
    record.header.typeId = kBinarySensorDeviceTypeId;
    record.header.configVersion = kBinarySensorDeviceConfigVersion;
    const DeviceConfigBlob blob = [&]() {
        uint8_t buffer[kMaxDeviceConfigBytes]{};
        DeviceConfigBlob out{};
        TEST_ASSERT_TRUE(encodeFixedConfigBlob(BinarySensorDeviceConfigV1::kMagic, config, buffer, binarySensorDeviceConfigSize(config)));
        TEST_ASSERT_TRUE(out.assign(buffer, binarySensorDeviceConfigSize(config)));
        return out;
    }();
    record.header.payloadLength = static_cast<uint32_t>(blob.size());

    device.bindDeviceIdentity(record, blob);
    device.begin(10U);
    for (int i = 0; i < 4 && device.status() != DeviceStatus::Ready; ++i) {
        device.tickFastLoop(10U);
    }
    device.tickFastLoop(11U);

    StaticJsonDocument<1024> doc;
    JsonObject output = doc.to<JsonObject>();
    BinarySensorDeviceApiAdapter::instance().writeDeviceJson(device, device.status(), output);

    TEST_ASSERT_FALSE(doc.overflowed());
    assertMatchesJsonSchema("schemas/rest/v1/responses/devices-binary_sensor.response.schema.json", doc.as<JsonVariantConst>());
    TEST_ASSERT_EQUAL_UINT32(27U, output["record"]["id"].as<uint32_t>());
    TEST_ASSERT_EQUAL_STRING("binary_sensor", output["record"]["typeName"].as<const char*>());
    TEST_ASSERT_EQUAL_STRING("float switch", output["config"]["name"].as<const char*>());
    TEST_ASSERT_TRUE(output["runtime"]["output"]["active"].as<bool>());
    TEST_ASSERT_TRUE(output["runtime"]["output"]["rawLevel"].as<bool>());
    TEST_ASSERT_TRUE(output["runtime"]["output"]["hasReading"].as<bool>());
}

void test_binary_sensor_api_adapter_rejects_pull_on_input_only_pin() {
    StaticJsonDocument<256> doc;
    doc["typeName"] = "binary_sensor";
    JsonObject config = doc.createNestedObject("config");
    config["name"] = "reagent low level";
    config["gpioPin"] = 36;
    config["pullMode"] = "pullup";

    DeviceCreateRequest request{};
    const char* error = nullptr;
    TEST_ASSERT_FALSE(BinarySensorDeviceApiAdapter::instance().parseCreateRequest(doc.as<JsonObjectConst>(), request, error));
    TEST_ASSERT_EQUAL_STRING("device config is invalid", error);
}

void test_binary_sensor_api_adapter_partial_update_preserves_pin_and_debounce() {
    BinarySensorDeviceConfigV1 current{};
    current.enabled = 1U;
    std::snprintf(current.name, sizeof(current.name), "%s", "reagent low level");
    current.gpioPin = 27U;
    current.pullMode = static_cast<uint8_t>(GpioInputPullMode::PullDown);
    current.debounceMs = 120U;
    class FakeDriver final : public IGpioInputDriver {
    public:
        bool configureInput(uint8_t, GpioInputPullMode) override {
            return true;
        }
        bool read(uint8_t, bool& level) override {
            level = false;
            return true;
        }
        void release(uint8_t) override {}
    } driver;
    BinarySensorDevice runtime(current, driver);

    StaticJsonDocument<256> doc;
    JsonObject config = doc.createNestedObject("config");
    config["inverted"] = true;

    assertMatchesJsonSchema("schemas/rest/v1/requests/devices-update-binary_sensor.request.schema.json", doc.as<JsonVariantConst>());

    DeviceConfigUpdateRequest request{};
    const char* error = nullptr;
    TEST_ASSERT_TRUE(BinarySensorDeviceApiAdapter::instance().parseUpdateConfigRequest(doc.as<JsonObjectConst>(), runtime, request, error));
    TEST_ASSERT_EQUAL_STRING("reagent low level", request.baseConfig.name);
    TEST_ASSERT_TRUE(request.isEnabled());

    BinarySensorDeviceConfigV1 parsed{};
    TEST_ASSERT_TRUE(decodeValidatedFixedConfigBlob(BinarySensorDeviceConfigV1::kMagic,
                                                    reinterpret_cast<const uint8_t*>(request.configBlob.data()), request.configBlob.size(),
                                                    parsed));
    TEST_ASSERT_EQUAL_UINT8(1U, parsed.inverted);
    TEST_ASSERT_EQUAL_UINT8(27U, parsed.gpioPin);
    TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>(GpioInputPullMode::PullDown), parsed.pullMode);
    TEST_ASSERT_EQUAL_UINT16(120U, parsed.debounceMs);
}

void test_dosing_pump_api_adapter_parses_create_request_with_deps() {
    StaticJsonDocument<2048> doc;
    doc["typeName"] = "dosing_pump";
    JsonObject config = doc.createNestedObject("config");
    config["name"] = "Calcium";
    config["enabled"] = true;
    config["dosingSpeedMlPerSec"] = 1.25;
    JsonObject container = config.createNestedObject("container");
    container["capacityMl"] = 500;
    container["thresholdPercent"] = 15;
    container["blockAutoWhenEmpty"] = true;
    JsonObject schedule = config.createNestedObject("schedule");
    schedule["mode"] = "daily";
    schedule["everyDays"] = 2;
    schedule["anchorDay"] = 20647;
    JsonArray doses = schedule.createNestedArray("doses");
    JsonObject dose = doses.createNestedObject();
    dose["time"] = "08:30";
    dose["amountMl"] = 12.3;
    JsonArray deps = config.createNestedArray("deps");
    JsonObject switchDep = deps.createNestedObject();
    switchDep["role"] = "switch";
    switchDep["deviceId"] = 7;
    JsonObject sensorDep = deps.createNestedObject();
    sensorDep["role"] = "condition";
    sensorDep["deviceId"] = 8;
    sensorDep["invert"] = true;

    assertMatchesJsonSchema("schemas/rest/v1/requests/devices-create-dosing_pump.request.schema.json", doc.as<JsonVariantConst>());

    DeviceCreateRequest request{};
    const char* error = nullptr;
    TEST_ASSERT_TRUE(DosingPumpDeviceApiAdapter::instance().parseCreateRequest(doc.as<JsonObjectConst>(), request, error));
    TEST_ASSERT_EQUAL_UINT32(kDosingPumpDeviceTypeId, request.typeId);
    TEST_ASSERT_EQUAL_STRING("Calcium", request.baseConfig.name);
    TEST_ASSERT_EQUAL_UINT8(2U, request.dependencyCount());
    TEST_ASSERT_TRUE(request.dependencyLinks()[0].role == DeviceRole::Switch);
    TEST_ASSERT_EQUAL_UINT32(7U, request.dependencyLinks()[0].deviceId);
    TEST_ASSERT_TRUE(request.dependencyLinks()[1].role == DeviceRole::Condition);
    TEST_ASSERT_TRUE(request.dependencyLinks()[1].invert);

    DosingPumpDeviceConfigV1 parsed{};
    TEST_ASSERT_TRUE(decodeValidatedFixedConfigBlob(
        DosingPumpDeviceConfigV1::kMagic, reinterpret_cast<const uint8_t*>(request.configBlob.data()), request.configBlob.size(), parsed));
    TEST_ASSERT_EQUAL_UINT16(1250U, parsed.speedMilliMlPerSec);
    TEST_ASSERT_EQUAL_UINT16(500U, parsed.containerCapacityMl);
    TEST_ASSERT_EQUAL_UINT8(15U, parsed.thresholdPercent);
    TEST_ASSERT_EQUAL_UINT8(2U, parsed.everyDays);
    TEST_ASSERT_EQUAL_UINT16(20647U, parsed.anchorDay);
    TEST_ASSERT_EQUAL_UINT8(1U, parsed.doseCount);
    TEST_ASSERT_EQUAL_UINT16(8U * 60U + 30U, parsed.doses[0].minuteOfDay);
    TEST_ASSERT_EQUAL_UINT16(1230U, parsed.doses[0].amountCentiMl);
}

void test_dosing_pump_api_adapter_serializes_record() {
    const DosingPumpDeviceConfigV1 config = makeDosingPumpConfig();
    const DeviceConfigBlob blob = encodeDosingPumpBlob(config);
    DeviceRegistryEntry record{};
    record.header.deviceId = 31U;
    record.header.typeId = kDosingPumpDeviceTypeId;
    record.header.configVersion = kDosingPumpDeviceConfigVersion;
    record.header.payloadLength = static_cast<uint32_t>(blob.size());

    DosingPumpDevice device(config);
    device.bindDeviceIdentity(record, blob);
    device.begin(10U);
    device.tickFastLoop(11U);

    StaticJsonDocument<2048> doc;
    JsonObject output = doc.to<JsonObject>();
    DosingPumpDeviceApiAdapter::instance().writeDeviceJson(device, device.status(), output);

    TEST_ASSERT_FALSE(doc.overflowed());
    assertMatchesJsonSchema("schemas/rest/v1/responses/devices-dosing_pump.response.schema.json", doc.as<JsonVariantConst>());
    TEST_ASSERT_EQUAL_UINT32(31U, output["record"]["id"].as<uint32_t>());
    TEST_ASSERT_EQUAL_STRING("dosing_pump", output["record"]["typeName"].as<const char*>());
    TEST_ASSERT_EQUAL_STRING("Calcium", output["config"]["name"].as<const char*>());
    TEST_ASSERT_TRUE(output["runtime"]["output"]["container"].is<JsonObjectConst>());
    TEST_ASSERT_TRUE(output["runtime"]["output"]["skipNext"].is<JsonArrayConst>());
}

void test_dosing_pump_api_adapter_rejects_missing_switch_dep() {
    StaticJsonDocument<1024> doc;
    doc["typeName"] = "dosing_pump";
    JsonObject config = doc.createNestedObject("config");
    config["name"] = "Calcium";
    JsonArray deps = config.createNestedArray("deps");
    JsonObject sensorDep = deps.createNestedObject();
    sensorDep["role"] = "condition";
    sensorDep["deviceId"] = 8;

    assertMatchesJsonSchema("schemas/rest/v1/requests/devices-update-dosing_pump.request.schema.json", doc.as<JsonVariantConst>());

    DeviceCreateRequest request{};
    const char* error = nullptr;
    TEST_ASSERT_FALSE(DosingPumpDeviceApiAdapter::instance().parseCreateRequest(doc.as<JsonObjectConst>(), request, error));
    TEST_ASSERT_EQUAL_STRING("dosing pump requires a switch dependency", error);
}

void test_dosing_pump_api_adapter_partial_update_preserves_schedule() {
    DosingPumpDeviceConfigV1 current{};
    current.enabled = 1U;
    std::snprintf(current.name, sizeof(current.name), "%s", "Calcium");
    current.speedMilliMlPerSec = 1250U;
    current.containerCapacityMl = 500U;
    current.doses[0] = DosingPumpDoseV1{510U, 1230U};
    current.doseCount = 1U;
    DosingPumpDevice runtime(current);

    StaticJsonDocument<256> doc;
    JsonObject config = doc.createNestedObject("config");
    config["dosingSpeedMlPerSec"] = 1.3;

    assertMatchesJsonSchema("schemas/rest/v1/requests/devices-update-dosing_pump.request.schema.json", doc.as<JsonVariantConst>());

    DeviceConfigUpdateRequest request{};
    const char* error = nullptr;
    TEST_ASSERT_TRUE(DosingPumpDeviceApiAdapter::instance().parseUpdateConfigRequest(doc.as<JsonObjectConst>(), runtime, request, error));
    TEST_ASSERT_EQUAL_STRING("Calcium", request.baseConfig.name);
    TEST_ASSERT_FALSE(request.depsProvided);

    DosingPumpDeviceConfigV1 parsed{};
    TEST_ASSERT_TRUE(decodeValidatedFixedConfigBlob(
        DosingPumpDeviceConfigV1::kMagic, reinterpret_cast<const uint8_t*>(request.configBlob.data()), request.configBlob.size(), parsed));
    TEST_ASSERT_EQUAL_UINT16(1300U, parsed.speedMilliMlPerSec);
    TEST_ASSERT_EQUAL_UINT16(500U, parsed.containerCapacityMl);
    TEST_ASSERT_EQUAL_UINT8(1U, parsed.doseCount);
    TEST_ASSERT_EQUAL_UINT16(510U, parsed.doses[0].minuteOfDay);
    TEST_ASSERT_EQUAL_UINT16(1230U, parsed.doses[0].amountCentiMl);
}

void test_auto_switch_api_adapter_parses_create_request() {
    StaticJsonDocument<512> doc;
    doc["typeName"] = "auto_switch";
    JsonObject config = doc.createNestedObject("config");
    config["name"] = "auto switch";
    config["enabled"] = true;
    config["pauseDurationSeconds"] = 900;
    JsonArray deps = config.createNestedArray("deps");
    JsonObject switchDep = deps.createNestedObject();
    switchDep["role"] = "switch";
    switchDep["deviceId"] = 10;
    JsonObject conditionDep = deps.createNestedObject();
    conditionDep["role"] = "condition";
    conditionDep["deviceId"] = 11;
    conditionDep["invert"] = true;

    assertMatchesJsonSchema("schemas/rest/v1/requests/devices-create-auto_switch.request.schema.json", doc.as<JsonVariantConst>());

    DeviceCreateRequest request{};
    const char* error = nullptr;
    TEST_ASSERT_TRUE(AutoSwitchDeviceApiAdapter::instance().parseCreateRequest(doc.as<JsonObjectConst>(), request, error));
    TEST_ASSERT_EQUAL_UINT32(kAutoSwitchDeviceTypeId, request.typeId);
    TEST_ASSERT_EQUAL_UINT32(kAutoSwitchDeviceConfigVersion, request.configVersion);
    TEST_ASSERT_EQUAL_STRING("auto switch", request.baseConfig.name);
    TEST_ASSERT_TRUE(request.isEnabled());
    TEST_ASSERT_EQUAL_UINT8(2U, request.dependencyCount());
    TEST_ASSERT_EQUAL_UINT32(10U, request.dependencyLinks()[0].deviceId);
    TEST_ASSERT_EQUAL_UINT32(11U, request.dependencyLinks()[1].deviceId);

    AutoSwitchDeviceConfigV1 parsed{};
    TEST_ASSERT_TRUE(decodeValidatedFixedConfigBlob(
        AutoSwitchDeviceConfigV1::kMagic, reinterpret_cast<const uint8_t*>(request.configBlob.data()), request.configBlob.size(), parsed));
    TEST_ASSERT_EQUAL_UINT32(900U, parsed.pauseDurationSeconds);
}

void test_auto_switch_api_adapter_parses_update_request() {
    AutoSwitchDeviceConfigV1 currentConfig = makeAutoSwitchConfig();
    AutoSwitchDevice runtime(currentConfig);

    StaticJsonDocument<512> doc;
    JsonObject config = doc.createNestedObject("config");
    config["pauseDurationSeconds"] = 1200;
    JsonArray deps = doc.createNestedArray("deps");
    JsonObject switchDep = deps.createNestedObject();
    switchDep["role"] = "switch";
    switchDep["deviceId"] = 10;
    JsonObject conditionDep = deps.createNestedObject();
    conditionDep["role"] = "condition";
    conditionDep["deviceId"] = 11;

    assertMatchesJsonSchema("schemas/rest/v1/requests/devices-update-auto_switch.request.schema.json", doc.as<JsonVariantConst>());

    DeviceConfigUpdateRequest request{};
    const char* error = nullptr;
    TEST_ASSERT_TRUE(AutoSwitchDeviceApiAdapter::instance().parseUpdateConfigRequest(doc.as<JsonObjectConst>(), runtime, request, error));
    TEST_ASSERT_EQUAL_UINT32(kAutoSwitchDeviceConfigVersion, request.configVersion);
    TEST_ASSERT_TRUE(request.depsProvided);
    TEST_ASSERT_EQUAL_UINT8(2U, request.depCount);

    AutoSwitchDeviceConfigV1 parsed{};
    TEST_ASSERT_TRUE(decodeValidatedFixedConfigBlob(
        AutoSwitchDeviceConfigV1::kMagic, reinterpret_cast<const uint8_t*>(request.configBlob.data()), request.configBlob.size(), parsed));
    TEST_ASSERT_EQUAL_UINT32(1200U, parsed.pauseDurationSeconds);
    TEST_ASSERT_EQUAL_STRING("auto switch", request.baseConfig.name);
}

void test_auto_switch_api_adapter_serializes_record() {
    const AutoSwitchDeviceConfigV1 config = makeAutoSwitchConfig();
    const DeviceConfigBlob blob = encodeAutoSwitchBlob(config);
    DeviceRegistryEntry record{};
    record.header.deviceId = 19U;
    record.header.typeId = kAutoSwitchDeviceTypeId;
    record.header.configVersion = kAutoSwitchDeviceConfigVersion;
    record.header.payloadLength = static_cast<uint32_t>(blob.size());

    AutoSwitchDevice device(record, blob);
    device.begin(10U);
    device.tickFastLoop(11U);

    StaticJsonDocument<1024> doc;
    JsonObject output = doc.to<JsonObject>();
    AutoSwitchDeviceApiAdapter::instance().writeDeviceJson(device, device.status(), output);

    TEST_ASSERT_FALSE(doc.overflowed());
    assertMatchesJsonSchema("schemas/rest/v1/responses/devices-auto_switch.response.schema.json", doc.as<JsonVariantConst>());
    TEST_ASSERT_EQUAL_UINT32(19U, output["record"]["id"].as<uint32_t>());
    TEST_ASSERT_EQUAL_STRING("auto_switch", output["record"]["typeName"].as<const char*>());
    TEST_ASSERT_EQUAL_STRING("auto switch", output["config"]["name"].as<const char*>());
    TEST_ASSERT_EQUAL_UINT32(900U, output["config"]["pauseDurationSeconds"].as<uint32_t>());
    TEST_ASSERT_EQUAL_STRING("auto", output["runtime"]["output"]["mode"].as<const char*>());
}

void test_ntc_thermistor_api_adapter_partial_update_preserves_calibration() {
    NtcThermistorTemperatureSensorConfigV1 current{};
    current.enabled = 1U;
    std::snprintf(current.name, sizeof(current.name), "%s", "boiler probe");
    current.supplyMilliVolts = 3250;
    current.betaCoefficient = 4100;
    current.seriesResistorOhms = 9800;
    NtcThermistorTemperatureSensorDevice runtime(current);

    StaticJsonDocument<256> doc;
    JsonObject config = doc.createNestedObject("config");
    config["pollMs"] = 2000;

    assertMatchesJsonSchema("schemas/rest/v1/requests/devices-update-ntc_thermistor_temperature_sensor.request.schema.json",
                            doc.as<JsonVariantConst>());

    DeviceConfigUpdateRequest request{};
    const char* error = nullptr;
    TEST_ASSERT_TRUE(NtcThermistorTemperatureSensorDeviceApiAdapter::instance().parseUpdateConfigRequest(doc.as<JsonObjectConst>(), runtime,
                                                                                                         request, error));
    TEST_ASSERT_EQUAL_STRING("boiler probe", request.baseConfig.name);
    TEST_ASSERT_TRUE(request.isEnabled());

    NtcThermistorTemperatureSensorConfigV1 parsed{};
    TEST_ASSERT_TRUE(decodeValidatedFixedConfigBlob(NtcThermistorTemperatureSensorConfigV1::kMagic,
                                                    reinterpret_cast<const uint8_t*>(request.configBlob.data()), request.configBlob.size(),
                                                    parsed));
    TEST_ASSERT_EQUAL_UINT32(2000, parsed.pollMs);
    TEST_ASSERT_EQUAL_UINT16(3250, parsed.supplyMilliVolts);
    TEST_ASSERT_EQUAL_UINT16(4100, parsed.betaCoefficient);
    TEST_ASSERT_EQUAL_UINT16(9800, parsed.seriesResistorOhms);
}
