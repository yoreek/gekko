## MODIFIED Requirements

### Requirement: Device widgets are split by component responsibility
The SPA SHALL organize device widgets in a dedicated component area with a base device widget component and type-specific extensions.

#### Card composition
- The dashboard card MUST show only the device name.
- The dashboard card MAY include an optional compact device-type icon only if it does not increase the card height or add extra metadata rows.
- The dashboard card MUST use `effective_status` only to derive visual readiness and MUST NOT render `Ready`, `!Ready`, or raw status text.
- The dashboard card MUST dim the card when the backend status is not `ready`.
- The dashboard card MUST use a fixed compact footprint and MUST NOT expose resize handles.
- The dashboard card MUST be implemented as a reusable base card shell with type-specific specialized components layered on top.
- Switch-like dashboard cards MAY add one compact Power action without adding metadata rows or increasing card height.

#### Scenario: Base widget is reused
- **WHEN** a device type does not need custom rendering
- **THEN** the SPA renders it with the base device widget component

#### Scenario: Type-specific widget can extend base
- **WHEN** a device type needs custom rendering
- **THEN** the SPA renders it with a type-specific component layered on top of the base widget component without duplicating shared name, readiness, edit-mode, or open-detail behavior

#### Scenario: GPIO switch widget uses registry mapping
- **WHEN** a dashboard panel contains a `GpioSwitchDevice`
- **THEN** the SPA resolves the GPIO switch dashboard component through the explicit device component registry

#### Scenario: GPIO switch widget stays compact
- **WHEN** a `GpioSwitchDevice` widget is displayed on the dashboard
- **THEN** it preserves the fixed compact card footprint and does not add multi-row controls or status text

#### Scenario: GPIO switch widget shows power action
- **WHEN** a ready `GpioSwitchDevice` widget is displayed on the dashboard
- **THEN** it shows one compact local-icon Power action whose visual state follows runtime `output.state`

#### Scenario: GPIO switch power visual states are theme based
- **WHEN** a GPIO switch Power action renders for `on`, `off`, or `disabled`
- **THEN** it uses Vuetify theme colors to show active/success for `on`, muted/inactive for `off`, and disabled/muted for `disabled`

#### Scenario: GPIO switch power action sends explicit command
- **WHEN** the user activates the GPIO switch Power action
- **THEN** the SPA sends `state=off` when runtime `output.state` is `on` and sends `state=on` when runtime `output.state` is `off`

#### Scenario: Disabled output is not toggled from dashboard
- **WHEN** a `GpioSwitchDevice` dashboard widget has runtime `output.state` equal to `disabled` or no known runtime output state
- **THEN** the Power action is disabled and does not send a command

#### Scenario: Full output states stay in details
- **WHEN** the user needs to set a GPIO switch to `on`, `off`, or `disabled`
- **THEN** the detail or Devices control surface provides the explicit three-state control

#### Scenario: Dashboard controls are disabled in edit mode
- **WHEN** dashboard edit mode is active
- **THEN** device widget controls do not execute commands and the card interaction is reserved for drag, remove, and layout editing

#### Scenario: Device widgets are rendered
- **WHEN** the dashboard loads the active panel layout
- **THEN** it renders one widget per device in the saved panel layout

#### Scenario: Widgets show primary state only
- **WHEN** a device widget is displayed
- **THEN** it shows the device name

#### Scenario: Not ready widgets are dimmed
- **WHEN** a device widget has any backend status other than `ready`
- **THEN** the card is visually dimmed to indicate that it is not working

#### Scenario: Widgets reflect live updates
- **WHEN** a realtime device update arrives
- **THEN** the affected widget updates without requiring a full page reload

#### Scenario: Switch output updates use generic device update messages
- **WHEN** a GPIO switch output state changes and the backend emits a realtime update
- **THEN** the SPA receives the updated `output.state` through the generic device update payload without requiring a switch-specific topic

## ADDED Requirements

### Requirement: GPIO switch dashboard opens typed details
The SPA SHALL let users reach GPIO switch details from the dashboard without adding controls to the compact card.

#### Scenario: GPIO switch card opens detail dialog
- **WHEN** the user selects a `GpioSwitchDevice` dashboard widget
- **THEN** the SPA opens the device detail dialog with the GPIO switch typed detail section
