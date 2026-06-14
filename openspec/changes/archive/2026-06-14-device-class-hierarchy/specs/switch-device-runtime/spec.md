## ADDED Requirements

### Requirement: Switch devices share explicit output-state behavior
The firmware SHALL provide reusable switch-device base runtimes with explicit output-state capabilities, inversion, startup state policy, safe state policy, command handling, and retained-state integration.

#### Scenario: Switch exposes output state
- **WHEN** a switch runtime is ready
- **THEN** it exposes the current `OutputState` independently from the physical output level

#### Scenario: Switch applies inversion
- **WHEN** a switch config enables inversion
- **THEN** the runtime maps `On` and `Off` to the opposite physical output level before writing hardware

#### Scenario: Switch disabled state does not use inversion
- **WHEN** a switch output state is `Disabled`
- **THEN** the runtime disables the output when supported instead of applying inversion

#### Scenario: Switch uses configured startup state
- **WHEN** a switch starts without valid retained state or restore-previous-state is disabled
- **THEN** it applies the configured fallback startup state

#### Scenario: Switch restores previous retained state
- **WHEN** a switch starts with restore-previous-state enabled and valid retained state exists
- **THEN** it applies the retained `OutputState` instead of the fallback startup state

#### Scenario: Switch state changes are explicit set commands
- **WHEN** a caller sends a supported switch command to set `On`, `Off`, or `Disabled`
- **THEN** the switch updates its output state, writes the corresponding hardware output, and reports the command as accepted

#### Scenario: Unsupported switch command is rejected
- **WHEN** a caller sends an unsupported command payload to a switch runtime
- **THEN** the runtime rejects the command without changing output state

#### Scenario: Unsupported output state is rejected
- **WHEN** a caller sends a command for an output state not supported by the concrete switch class
- **THEN** the runtime rejects the command without changing output state

### Requirement: Switch classes expose output-state capabilities
The firmware SHALL split switch base classes by supported output-state capabilities so concrete switch devices inherit only the behavior they support.

#### Scenario: Binary switch supports on and off
- **WHEN** a runtime inherits from `BinarySwitchDeviceBase`
- **THEN** it supports `OutputState::Off` and `OutputState::On`

#### Scenario: Tri-state switch supports disabled output
- **WHEN** a runtime inherits from `TriStateSwitchDeviceBase`
- **THEN** it supports `OutputState::Off`, `OutputState::On`, and `OutputState::Disabled`

#### Scenario: Disabled output can be ready
- **WHEN** a concrete switch class supports `OutputState::Disabled` and the runtime applies that state successfully
- **THEN** the device can report `Ready`

### Requirement: Switch retained state is separate from config
The firmware SHALL persist switch output restore values as retained runtime state instead of rewriting the switch configuration payload for ordinary output-state changes.

#### Scenario: Output change records retained state
- **WHEN** a switch output state changes, restore-previous-state is enabled, and the type supports retained state
- **THEN** the firmware records the latest `OutputState` through retained-state persistence rather than changing the device config payload

#### Scenario: Restore disabled does not record retained state
- **WHEN** a switch output state changes and restore-previous-state is disabled
- **THEN** the firmware does not save switch retained state

#### Scenario: Config revision is not incremented for output command
- **WHEN** a switch output state command is applied without changing its configuration
- **THEN** the firmware does not increment the device config revision just because the output state changed

#### Scenario: Retained state is saved after successful apply
- **WHEN** a switch output state command is accepted but hardware application fails
- **THEN** the firmware does not mark the retained state for saving

#### Scenario: Retained state writes are debounced
- **WHEN** switch output state changes repeatedly
- **THEN** retained-state persistence coalesces writes instead of writing flash for every command immediately

### Requirement: GPIO switch device drives a configured GPIO output
The firmware SHALL provide a concrete GPIO switch device type that inherits the switch base behavior and writes the computed physical output to a configured GPIO pin through a bounded driver interface.

#### Scenario: GPIO switch configures output pin
- **WHEN** a GPIO switch runtime starts
- **THEN** it configures the selected GPIO pin as an output and applies a deterministic initial physical level

#### Scenario: GPIO switch writes physical output
- **WHEN** the GPIO switch logical state changes
- **THEN** it writes the computed physical output level to the configured GPIO pin

#### Scenario: GPIO switch can be disabled
- **WHEN** a GPIO switch runtime is disabled
- **THEN** it stops normal runtime output activity and applies the configured safe output state

#### Scenario: GPIO switch supports output disabled
- **WHEN** a GPIO switch applies `OutputState::Disabled`
- **THEN** it disables the GPIO output or places it into the platform-supported high-impedance mode

#### Scenario: GPIO switch rejects invalid pin config
- **WHEN** a GPIO switch config references an unsupported or invalid GPIO pin
- **THEN** the firmware rejects the config before creating or reconfiguring the runtime

### Requirement: Future switch hardware variants share the base
The firmware SHALL model future switch implementations such as I2C port expanders as concrete switch runtimes that inherit shared switch behavior and implement only their hardware access hooks.

#### Scenario: Hardware-specific switch keeps common behavior
- **WHEN** a new switch transport such as I2C expander output is added
- **THEN** it reuses the shared switch state, startup, inversion, command, and retained-state behavior
