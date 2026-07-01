# SPI Device Presence Check Specification

## ADDED Requirements

### Requirement: Probe rejects CS pins claimed by other devices on same bus
The system SHALL reject probe commands if the target CS pin is already in use by another SPI-consumer device on the same SPI bus.

#### Scenario: Collision detection prevents dual-use of CS pin
- **WHEN** user attempts to probe CS pin that another device on the bus already uses
- **THEN** system rejects command with error `InvalidCommand` / "CS pin already in use"
- **AND** no probe operation is performed
- **AND** device state remains unchanged

### Requirement: Probe saves and restores CS pin state
The system SHALL preserve the CS pin's original mode (OUTPUT/INPUT) and level (HIGH/LOW) before and after probe, ensuring that an already-initialized device continues to operate normally post-probe.

#### Scenario: CS state preserved for configured device
- **WHEN** user probes an existing device where CS pin is already configured as OUTPUT
- **THEN** system saves current pin mode (OUTPUT) and current driven level (HIGH or LOW)
- **AND** after probe completes, system restores mode and level exactly
- **AND** device operation resumes without disruption

#### Scenario: CS state preservation succeeds even if probe fails
- **WHEN** probe encounters an error during MISO-activity check (e.g., bus contention)
- **THEN** CS pin is restored to saved state regardless of probe outcome
- **AND** no orphaned pin state is left

### Requirement: Probe acquires exclusive SPI bus access
The system SHALL acquire a `DependencyTransaction` on the SPI bus before modifying CS, ensuring no other consumer can perform SPI transfers during probe.

#### Scenario: Probe rejected if dependency transaction active
- **WHEN** user initiates probe while another device holds active `DependencyTransaction`
- **THEN** command is rejected
- **AND** probe is not attempted

#### Scenario: Probe rejected if bus not in Ready state
- **WHEN** SPI bus is Faulted, Disabled, Reconfiguring, or DependencyBlocked
- **THEN** probe command is rejected
- **AND** no probe operation is performed

### Requirement: Probe CS pin via MISO-activity (primary method)
The system SHALL support probing by monitoring MISO line response when CS is toggled, provided `SpiBusDeviceConfigV1.misoPin >= 0`.

**Probe logic:**
- Acquire `DependencyTransaction` and save CS state.
- Transfer idle byte (0xFF) with CS at current level (likely HIGH for unselected).
- Transfer idle byte (0xFF) with CS toggled to opposite level (likely LOW for selected).
- Observe MISO responses: different bytes → device responding; identical bytes → inconclusive.
- Restore CS state and release transaction.

#### Scenario: MISO-activity detects responsive device
- **WHEN** MISO-activity probe runs on bus with misoPin configured
- **AND** connected device drives MISO differently when CS=HIGH (unselected) vs CS=LOW (selected)
- **THEN** outcome is `Detected` with method `MisoActivity`

#### Scenario: Identical MISO bytes yield Inconclusive (not NotDetected)
- **WHEN** MISO-activity probe captures identical idle bytes regardless of CS state
- **THEN** outcome is `Inconclusive` with method `MisoActivity`
- **AND** system does not assume device absence (MISO may be stuck or floating)

#### Scenario: MISO-activity test works on free or configured pins
- **WHEN** probe runs on CS pin that is either unconfigured (free) or already configured
- **THEN** MISO-activity method works in both cases
- **AND** no special handling needed; CS save/restore covers both

### Requirement: Fallback to CS pull-resistor heuristic when MISO unavailable
The system SHALL support secondary probe method using CS pin pull-resistor sensing if MISO is not configured (`misoPin < 0`).

**Probe logic:**
- Acquire `DependencyTransaction` and save CS state.
- Configure CS as INPUT_PULLUP, read level.
- Configure CS as INPUT_PULLDOWN, read level.
- Compare readings: difference or strong external pull → weak evidence of device.
- Restore CS state and release transaction.

#### Scenario: CS pull-heuristic applied when MISO not available
- **WHEN** probe runs on bus without misoPin configured
- **THEN** system attempts CS pull-resistor heuristic
- **AND** outcome is returned with method `CsPullHeuristic` 

#### Scenario: CS pull-heuristic returns Inconclusive by default
- **WHEN** CS pull-resistor test shows ambiguous reading (floating, weak pull, or no pull)
- **THEN** outcome is `Inconclusive` with method `CsPullHeuristic`
- **AND** outcome is never `Detected` with CS pull (inherently unreliable)

#### Scenario: CS pull-heuristic only safe on free or restorable pins
- **WHEN** CS pin is actively controlled by initialized device
- **THEN** CS pull-heuristic still runs (save/restore ensures safe state recovery)
- **AND** result marked clearly as heuristic-only in UI

### Requirement: Probe respects bus ready state and active transactions
The system SHALL reject probe commands if the SPI bus is not in Ready state or has an active dependency transaction.

#### Scenario: Probe rejected when bus not Ready
- **WHEN** user initiates probe on SPI bus in Faulted, Disabled, or Reconfiguring state
- **THEN** system rejects command with validation error
- **AND** no probe operation is performed

### Requirement: Probe result is transient runtime state
The system SHALL NOT persist probe results to NVS, config blobs, or setup transfers. Probe results are pure runtime diagnostic state.

#### Scenario: Probe result appears in runtime JSON snapshot
- **WHEN** probe completes
- **THEN** `SpiBusRuntimeSnapshot.probe` contains: `ready` (bool), `csPin` (number), `outcome` (detected|not_detected|inconclusive|unknown), `method` (miso_activity|cs_pull_heuristic|none), `checkedAtMs` (uint32)
- **AND** result is transmitted via WebSocket device-updated event
- **AND** result expires or updates on next probe; not persisted

#### Scenario: Probe result never affects config validation
- **WHEN** device form saves configuration after probe returns `NotDetected` or `Inconclusive`
- **THEN** validation proceeds normally; no blocker
- **AND** probe result is purely informational

### Requirement: Probe is exposed via CheckDevice HTTP command
The system SHALL accept `checkDevice` command on SPI bus devices via HTTP controller.

**Request format:**
- Method: `POST /device/<deviceId>/command`
- Body: `{"command":"checkDevice", "csPin":<number>}`

**Validation:**
- csPin MUST be integer in range [0, 39]
- csPin MUST NOT be device's already-configured pin (or secondary check to prevent collision)

#### Scenario: Valid checkDevice command is routed to SpiBusDevice
- **WHEN** HTTP controller receives valid `{"command":"checkDevice", "csPin":12}`
- **THEN** controller validates csPin range
- **AND** routes to `DeviceRegistry::command(DeviceCommandType::CheckDevice, ...)`
- **AND** command reaches `SpiBusDevice::handleCommand()`

#### Scenario: Invalid csPin rejected with HTTP 400
- **WHEN** csPin is outside range [0, 39]
- **THEN** system responds HTTP 400 with error code `BAD_ARGS`
- **AND** probe is not executed

#### Scenario: Collision detected at HTTP level
- **WHEN** csPin is already in use by another device
- **THEN** HTTP returns 400 with error code `BAD_ARGS` / "CS pin already in use"
- **AND** no probe is attempted

### Requirement: Frontend provides reusable SpiChipSelectProbe component
The system SHALL provide `SpiChipSelectProbe.vue` in `portal-spa/src/components/devices/common/` for reuse by any SPI-consumer form.

**Component interface:**
- Props: `busDeviceId` (SPI bus device ID), `csPin` (draft form value, not saved config), `disabled` (optional).
- Behavior: onClick invokes `commandDevice(busDeviceId, {command:'checkDevice', csPin})`, displays loading state, error alert, and result chip.
- Reactivity: Subscribes to device registry; re-renders when `runtime.probe` updates.

#### Scenario: Probe button reflects draft (unsaved) CS pin value
- **WHEN** user edits CS pin field in form without saving
- **AND** clicks "Check Device" button
- **THEN** probe uses draft pin value, not previously saved value
- **AND** allows verification before commit

#### Scenario: Probe result shows method and confidence level
- **WHEN** probe returns outcome=`Inconclusive` with method=`CsPullHeuristic`
- **THEN** component displays "Inconclusive (CS pull heuristic)" with visual/textual caveat
- **AND** tooltip explains "heuristic-only; manual verification recommended"

#### Scenario: Probe button disabled when no SPI bus selected
- **WHEN** form has no valid SPI bus dependency (`spiBusDeviceId = 0`)
- **THEN** button is disabled
- **AND** displays message "Select an SPI bus first"

### Requirement: ST7735 form integrates probe component
The system SHALL include `SpiChipSelectProbe` adjacent to CS pin field in ST7735 device edit form.

#### Scenario: Probe button visible in ST7735 edit form
- **WHEN** user opens ST7735 device edit dialog
- **THEN** "Check Device" button is displayed next to CS pin input field
- **AND** button enabled if SPI bus is selected

### Requirement: Mock support for development
The system SHALL provide mock handler for `checkDevice` command in `portal-spa/src/mock/handlers.ts`.

#### Scenario: Mock checkDevice returns simulated probe result
- **WHEN** mock transport receives `{command:'checkDevice', csPin:10}`
- **THEN** mock returns probe result
- **AND** updates mock device `runtime.probe`
- **AND** WebSocket subscribers receive device-updated event

## REMOVED Requirements

None.

## MODIFIED Requirements

None. (SpiBusDevice spec itself unchanged; only new transient runtime operation added.)
