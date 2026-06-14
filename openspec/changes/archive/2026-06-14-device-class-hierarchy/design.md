## Context

`DummyDevice` is the first dynamic runtime device and currently combines several layers:

- generic `IDeviceRuntime` lifecycle methods and status tracking
- parent/child runtime wiring
- `StateMachine` lifecycle states
- retained on/off output state
- Dummy-specific config codec, descriptor, and simulated commands

That was acceptable for the first device type, but it will not scale to hardware devices. A GPIO switch, an I2C port-expander switch, and future switch variants need the same on/off semantics while differing only in how the physical output is driven.

The existing registry already provides the important boundaries:

- runtime instances are created through `DeviceTypeDescriptor::createRuntime`
- type configs are binary, versioned payloads
- retained runtime state is separate from config payloads
- runtime work is cooperative through `tickFastLoop`, `tick100ms`, and `tick1s`

## Goals / Non-Goals

**Goals:**

- Extract common runtime behavior from `DummyDevice` into a reusable base class.
- Keep `DummyDevice` behavior and compatibility intact while making it inherit from the new base.
- Add a reusable switch-device base class for output devices with explicit `OutputState` capabilities.
- Add the first concrete hardware switch implementation for GPIO output.
- Keep output state as runtime/retained state instead of rewriting config for every output-state command.
- Keep hardware-specific code isolated from generic switch behavior.

**Non-Goals:**

- Do not add I2C expander support in this change.
- Do not redesign `DeviceRegistry` persistence or public device API shapes beyond the new type registration.
- Do not add blocking GPIO waits or hardware polling loops.
- Do not change dashboard widget rendering in this change.

## Decisions

### Base runtime class

Add `DeviceRuntimeBase` under `src/devices/core/`.

Suggested responsibility:

- implement `IDeviceRuntime`
- own common status flags: `startRequested_`, `reconfigureRequested_`, `disableRequested_`, `deleteRequested_`, `faultRequested_`, `deleted_`
- own parent/child runtime pointers
- provide default no-op cadence handlers
- expose protected lifecycle helpers for derived classes
- provide common `parentReady()`, child attach/detach, and delete/disable/reconfigure request behavior

Suggested shape:

```cpp
class DeviceRuntimeBase : public StateMachine, public IDeviceRuntime {
public:
    explicit DeviceRuntimeBase(PState initialState);

    void begin(uint32_t now) override;
    void tickFastLoop(uint32_t now) override;
    void tick100ms(uint32_t now) override;
    void tick1s(uint32_t now) override;
    void setParentRuntime(IDeviceRuntime* parentRuntime) override;
    IDeviceRuntime* parentRuntime() const override;
    void attachChildRuntime(IDeviceRuntime* childRuntime) override;
    void detachChildRuntime(IDeviceRuntime* childRuntime) override;
    const std::vector<IDeviceRuntime*>& childRuntimes() const override;
    void requestReconfigure() override;
    void requestDisable() override;
    void requestDelete() override;
    DeviceStatus status() const override;

protected:
    void setStatus(DeviceStatus status);
    bool parentReady() const;
    bool startRequested() const;
    bool reconfigureRequested() const;
    bool disableRequested() const;
    bool deleteRequested() const;
    bool faultRequested() const;
    void clearStartRequested();
    void clearReconfigureRequested();
    void setDeleted();
};
```

`DummyDevice` should initially keep its own state-machine states, but reuse the base storage and parent/child logic. This avoids a large behavior rewrite and keeps the refactor testable.

### Switch output model

Switch-like devices share a small explicit output-state model:

```cpp
enum class OutputState : uint8_t {
    Off = 0,
    On = 1,
    Disabled = 2,
};
```

`Disabled` means the output is intentionally disabled/high-impedance when the concrete hardware supports that mode. It is a normal Ready-capable output state for device classes that advertise support for it.

Shared switch config fields:

- `enabled`: whether runtime starts enabled
- `restorePreviousState`: whether retained output state is saved and restored
- `startupState`: fallback `OutputState` after boot when retained state is not used
- `safeState`: `OutputState` applied for disable/delete/failure safe handling when supported
- `inverted`: maps `On`/`Off` to physical output levels; it does not apply to `Disabled`

Retained state stores `OutputState`, not a boolean. Retained state is saved only when `restorePreviousState` is enabled, only after the requested state is successfully applied, and through the existing debounced/coalesced retained-state persistence path. When `restorePreviousState` is disabled, no retained switch state is saved.

### Switch base class

Add `SwitchDeviceBase` under `src/devices/switch/`.

Suggested responsibility:

- inherit from `DeviceRuntimeBase`
- own shared switch config and current `OutputState`
- parse retained state and apply startup policy
- handle common explicit set-state commands
- call hardware-specific hooks when the desired output changes

Suggested runtime state:

- `outputState_`: requested user-visible `OutputState`
- `physicalState_`: computed physical level for `On`/`Off` after inversion
- `retainedStateAvailable_`
- `retainedOutputState_`
- `outputDirty_`: indicates retained state should be persisted by registry/event flow

Suggested hooks:

```cpp
class SwitchDeviceBase : public DeviceRuntimeBase {
public:
    OutputState outputState() const;
    bool physicalOutputState() const;
    void applyRetainedState(OutputState state);
    bool handleCommand(const DeviceCommand& command) override;

protected:
    explicit SwitchDeviceBase(PState initialState, const SwitchDeviceConfig& config);

    bool setOutputState(OutputState state, uint32_t now);
    bool supportsOutputState(OutputState state) const;
    bool restorePreviousState() const;
    OutputState startupState() const;
    OutputState safeState() const;
    bool inverted() const;

    virtual DeviceValidationResult configureHardware(uint32_t now) = 0;
    virtual DeviceValidationResult applyHardwareOutput(OutputState state, uint32_t now) = 0;
    virtual uint8_t supportedOutputStateMask() const = 0;
    virtual void releaseHardware(uint32_t now) = 0;
};
```

Common commands explicitly set the target state. There is no implicit "toggle to opposite" command because `Disabled` makes opposite-state semantics ambiguous.

- set `OutputState::On`
- set `OutputState::Off`
- set `OutputState::Disabled` when the concrete device supports it
- `SetStatus` payload `fault` / `ready`: keep existing test/debug behavior where applicable

Add capability-specialized base classes under `src/devices/switch/`:

- `BinarySwitchDeviceBase`: supports `Off` and `On`
- `TriStateSwitchDeviceBase`: supports `Off`, `On`, and `Disabled`

### GPIO switch class

Add `GpioSwitchDevice` under `src/devices/switch/gpio/` or `src/devices/gpio_switch/`.

Suggested config fields:

- all shared switch config fields
- `gpioPin`
- `mode`: output mode needed by platform driver, initially plain output
- `safeState`: output state applied when the device is disabled, deleted, or placed into safe handling

Suggested behavior:

- configure GPIO during Starting/Reconfiguring through `configureHardware(now)`
- write `On`/`Off` as computed physical output through `applyHardwareOutput(state, now)`
- apply `Disabled` by disabling the output/high-impedance when supported
- release or apply `safeState` during disable/delete through `releaseHardware(now)`

GPIO access should go through a small driver interface instead of direct calls scattered through runtime logic:

```cpp
class IGpioOutputDriver {
public:
    virtual bool configureOutput(uint8_t pin, bool initialLevel) = 0;
    virtual bool write(uint8_t pin, bool level) = 0;
    virtual bool disableOutput(uint8_t pin) = 0;
    virtual void release(uint8_t pin) = 0;
};
```

Production can wrap Arduino GPIO calls, and tests can use a fake driver.

### Config codecs

Keep config binary and versioned:

- `SwitchDeviceConfigV1` for shared fields with `OutputState` startup/safe states
- `GpioSwitchDeviceConfigV1` contains shared fields plus GPIO fields
- each released config struct remains immutable after release
- JSON parsing/writing stays type-specific for portal API adapters

`DummyDeviceConfigV2` remains supported. The refactor must not force a Dummy config format change.

### Type IDs

Keep `DummyDevice` as `type_id = 1`.

Reserve a stable new `type_id` for GPIO switch. Proposed value:

- `2`: `GpioSwitchDevice`

Future switch variants should get their own type IDs rather than overloading GPIO config:

- I2C expander switch
- shift-register switch
- virtual/group switch

## Risks / Trade-offs

- Base class can become too broad -> keep only lifecycle, parent/child wiring, and shared request/status storage in `DeviceRuntimeBase`.
- Switch base can leak hardware assumptions -> keep physical I/O behind hooks and driver interfaces.
- Refactoring `DummyDevice` may alter lifecycle behavior -> add parity tests around status transitions, retained state, commands, parent dependency, and delete/disable flows.
- GPIO output can be unsafe during boot -> config must include startup/fallback and optional safe physical state, and implementation must write a deterministic initial level before Ready.
- Command payload strings are simple but not very extensible -> keep current command style for compatibility, and defer structured commands until API requirements demand it.

## Migration Plan

1. Add `DeviceRuntimeBase` and move common parent/child/status request behavior behind tests.
2. Refactor `DummyDevice` to inherit from `DeviceRuntimeBase` without changing its descriptor, config payload, or external behavior.
3. Add `SwitchDeviceBase` and tests with a fake switch subclass.
4. Add `GpioSwitchDevice`, config codec, descriptor, and fake GPIO driver tests.
5. Register `GpioSwitchDevice` in the default type registry and update the type catalog.
6. Add portal API adapter/frontend catalog labels only after the backend type is test-covered.
