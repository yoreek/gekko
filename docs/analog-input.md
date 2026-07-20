# Analog Input

Four device types model where an analog voltage reading comes from and how a sensor that needs
one gets it, without the sensor ever touching ADC hardware directly: `analog_port_input` (the
ESP32's own on-chip ADC, standalone), `ads1115_hub` (an ADS1115 16-bit I2C ADC, 4 channels),
`cd74hc4067_hub` (a CD74HC4067 16-channel analog multiplexer feeding one ADC pin), and
`analog_input_channel` — the single channel-leaf type that depends on *either* hub generically via
the `analog_input_hub` role (see "One leaf type, not one per hub chip" below).
`ntc_thermistor_temperature_sensor` is the first consumer built on top of this: it owns only the
divider geometry and the Beta/Steinhart-Hart curve, not any ADC pin. See
[REST API Contract](rest-api-contract.md) for the public config/runtime JSON shape of each type.

## Shared input runtime (`src/devices/core/DeviceTypes.h`)

```cpp
struct AnalogInputReading {
    uint16_t rawCode{0};      // normalized 0..kAnalogInputResolutionMax (4095), UI/diagnostics only
    int32_t milliVolts{0};    // authoritative physical value
    uint32_t measuredAtMs{0};
    bool valid{false};
};

class IAnalogInputRuntime {
public:
    static constexpr DeviceRole kProvidedRole = DeviceRole::AnalogInput;
    virtual bool latestAnalogInputReading(AnalogInputReading& reading) const = 0;
    virtual const char* latestAnalogInputStatus() const = 0;
};

enum class AnalogInputHubPollResult : uint8_t { Busy = 0, Pending = 1, Ready = 2, Fault = 3 };

class IAnalogInputHubRuntime {
public:
    static constexpr DeviceRole kProvidedRole = DeviceRole::AnalogInputHub;
    virtual uint8_t channelCount() const = 0;
    virtual uint32_t generation() const = 0;
    virtual AnalogInputHubPollResult pollChannelReading(uint8_t channel, DeviceId requester, uint32_t now,
                                                        AnalogInputReading& outReading, const char*& outStatus) = 0;
    virtual void releaseChannelRequest(uint8_t channel, DeviceId requester) = 0;
};
```

`milliVolts` is the value every consumer should compute from; `rawCode` is a best-effort
normalization of whatever native resolution the backend ADC actually has (12-bit for the on-chip
port and the CD74HC4067's shared SIG pin, 16-bit for the ADS1115) and exists only for UI/telemetry.

Two roles, not one, because a multi-channel ADC and a single reading are different things a
consumer might depend on:

- **`AnalogInput`** - one reading. Provided by `AnalogPortInputDevice` and
  `AnalogInputChannelDevice`. This is the role `NtcThermistorTemperatureSensorDevice` depends on.
- **`AnalogInputHub`** - a shared multi-channel source that arbitrates access to its channels.
  Provided by `Ads1115HubDevice` and `Cd74hc4067HubDevice`. This is the role
  `AnalogInputChannelDevice` depends on to reach its channel.

## Hub-and-channel, not one device per physical chip

`Ads1115HubDevice`/`Cd74hc4067HubDevice` and the channel leaf mirror the PCF8574/PCF8575 port
expander pattern (see [Device Model Structures](device-model-structures.md)): the hub owns the
chip/pins and arbitrates access; each *used* channel is a separate, independently
enable/disable/poll-configurable registry device depending on the hub by role, not by concrete
type. `AnalogInputHubChannelDeviceBase` (`src/devices/analog/input/AnalogInputHubChannelDeviceBase.h`)
implements the entire poll/arbitration state machine; `AnalogInputChannelDevice`
(`src/devices/analog/input/channel/AnalogInputChannelDevice.h`) is the one concrete leaf built on
top of it, reading `channel()`/`adcSampleCount()`/`reportAlways()`/`reportDeltaMilliVolts()`/
`pollIntervalMs()`/`channelEnabled()` from its own config. `AnalogPortInputDevice` has no hub - it
owns its GPIO pin directly and reads it synchronously, the same shape as the pre-refactor NTC
sensor used to.

Channel-occupancy collision is the same generic mechanism port expanders already use:
`IDeviceRuntime::expanderChannel(uint8_t&)` (leaf reports its configured channel) and
`hasDuplicateDependentChannel(channel, ignoreDependent)` (hub walks its `dependentRuntimes()` and
rejects a second leaf on the same channel) - no AnalogInput-specific duplicate-channel machinery
was added; the field names are historically expander-flavored but were already role-generic.

### One leaf type, not one per hub chip

The first pass at this had `Ads1115InputDevice`/`Cd74hc4067InputDevice` as two separate typeIds
with two separate config structs, each carrying its own static `channel` upper bound
(`kAds1115InputMaxChannel = 3`, `kCd74hc4067InputMaxChannel = 15`). That was wrong: because a
channel leaf only ever talks to its hub through `IAnalogInputHubRuntime` (never naming the
concrete hub class), the REST layer's own dependency check
(`validateAnalogInputHubDependency` in `src/integrations/rest/common/AnalogInputDeviceApiSupport.cpp`)
already bounds `channel` dynamically against whichever hub is actually attached
(`hub->channelCount()`) - the UI's hub picker has no way to (and no reason to) filter by leaf
typeId, since the role contract makes every `AnalogInputHub` interchangeable from a channel leaf's
point of view. A static per-typeId bound on top of that dynamic check was therefore never the real
constraint; at best redundant, at worst wrong (an `ads1115_input` leaf could never reach channel 10
even when pointed at a 16-channel CD74HC4067 hub).

`AnalogInputChannelDevice` collapses this into a single typeId with a generous, hub-agnostic sanity
bound (`kAnalogInputChannelMaxChannel = 15`, the largest channel count any current hub backend
supports; `kAnalogInputChannelMaxAdcSamples = 32` similarly) - the real bound is, and always was,
`hub->channelCount()`. This mirrors `PortExpanderSwitchDeviceConfig`'s own
`kMaxPortExpanderChannel` (a "cheap, dependency-independent sanity bound" per its own comment) and
`Pcf857xExpanderConfigV2` (one shared config struct for PCF8574/PCF8575, differentiated only by
`channelCountImpl()`, never by two separate config types) - both established precedents for
exactly this situation before this device family existed.

## Non-blocking hub arbitration

Both hubs implement `pollChannelReading` without ever calling `delay()`: only one
`(channel, requester)` pair can be "owned" at a time, and a channel leaf simply polls again next
tick until it sees `Ready` or `Fault` instead of the hub blocking.

- **ADS1115** (`Ads1115HubDevice`): single-shot conversion over I2C. The first poll for a channel
  writes the config register (mux/gain/data-rate/`MODE=1`) through the existing
  `I2cBusDevice::DependencyTransaction` lease and returns `Pending` with a conversion deadline
  (`Ads1115Protocol::ads1115ConversionTimeMs`, a ceiling on `1000/dataRateSps` plus margin, the
  same shape as `ds18b20ConversionTimeMs()`). Once the deadline has passed, the next poll reads
  the conversion register and returns `Ready`; a bus contention (`DependencyAccessResult::Busy`)
  at either step just returns `Pending` again without disturbing hub state.
- **CD74HC4067** (`Cd74hc4067HubDevice`): no bus at all, just `IGpioOutputDriver` for the S0-S3
  address lines and the existing `IAdcInputDriver` (moved to `src/devices/analog/adc/`, shared with
  `AnalogPortInputDevice`) for the shared SIG pin. If the requested channel differs from the mux's
  last-selected channel, the hub flips the address lines and returns `Pending` - the settle time
  is paid for free by waiting until the *next* tick instead of a busy-wait. Once the mux is already
  on the requested channel, the hub reads the SIG pin synchronously and returns `Ready`.

A channel leaf's own poll loop (`AnalogInputHubChannelDeviceBase::Sampling`) repeats this
request/response cycle once per configured `adcSamples`, averaging into an `AdcSampleAccumulator`
- each sample after the first typically resolves in a single tick once the hub is already
converting/settled on that channel.

## Presets are a frontend-only convenience, never persisted

NTC thermistor sensors support both a **Beta equation** (`1/T = 1/T0 + (1/Beta)*ln(R/R0)`, the
two-point form every datasheet publishes, accurate to roughly +-0.5-1 degC over an aquarium
temperature range) and a **Steinhart-Hart equation** (`1/T = A + B*ln(R) + C*ln(R)^3`, more
accurate across a wider range when the coefficients are known or fitted from a 3-point R(T)
table). `NtcCurve.h` (`src/devices/sensors/ntc_thermistor/`) keeps both as pure, independently
unit-testable functions plus the shared divider math
(`ntcDividerResistanceOhms`) - the device itself only picks which curve function to call based on
`config.formulaMode`.

`portal-spa/src/models/devices/ntc-presets.ts` lists a handful of common thermistor models (10k
B3950, EPCOS/TDK 10k B3435, Vishay NTCLE100E3 10k B3977, Semitec 104GT-2 100k B4267) as
`{ seriesResistorOhms, nominalResistanceOhms, betaCoefficient }` triples. Selecting one from the
dropdown just pre-fills those numeric fields in the create/edit form (`NtcThermistorFields.vue`);
nothing about the preset choice itself is sent to the firmware or persisted - editing a field
afterward is always safe and never "fights" a stale preset selection. There is one type, not one
type per model.

## Home Assistant integration

One universal `AnalogInputHaEntityAdapter` (`src/integrations/mqtt/analog_input/`), the same
pattern as `AnalogOutputHaEntityAdapter`/`TemperatureSensorHaEntityAdapter`, is registered once per
**leaf** typeId (`analog_port_input`, `analog_input_channel`) with only `{typeId, typeName, icon}`
differing between registrations. It publishes an HA `sensor` entity in volts
(`device_class: voltage`). Hubs are not registered - they provide channels, not a reading of their
own.

## SPA model layer

- `analog-port-input.ts` - standalone leaf, no dependency.
- `ads1115-hub.ts` - depends on `i2c_bus`; `cd74hc4067-hub.ts` - no dependency (owns its GPIO pins
  directly). Both export a static `CHANNEL_COUNT`.
- `analog-input-channel.ts` - the single concrete `AnalogInputChannelDevice` model (mirrors the
  firmware's single typeId - see "One leaf type, not one per hub chip" above); also exports
  `analogInputHubChannelCount(hubTypeName)`, the SPA-side counterpart of the firmware's
  `hub->channelCount()` dynamic bound, used both by `AnalogInputChannelFields.vue` (to bound the
  channel number input to whichever hub is actually selected in the create/edit form) and by
  `AnalogInputHubWidget.vue` (`components/devices/analog-input/`, shared by both hub typeIds the
  same way `Pcf857xExpanderWidget.vue` is shared by PCF8574/PCF8575) to show "N channels" for
  whichever hub it's rendering.
- `ntc-thermistor.ts` - `dependencyDeviceId` now targets the `analog_input` role instead of owning
  `gpioPin`/`attenuation`/`adcSamples`; `ntc-presets.ts` holds the preset table described above.
