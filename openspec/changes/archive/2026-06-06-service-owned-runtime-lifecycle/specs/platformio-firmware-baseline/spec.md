## MODIFIED Requirements

### Requirement: Application lifecycle
The firmware SHALL expose a clear cooperative application lifecycle that initializes Arduino platform services, wires runtime services with explicit dependencies, and delegates service operational policy to the owning modules before WiFi provisioning or station mode are advanced.

#### Scenario: Boot initializes platform services
- **WHEN** the ESP32 boots
- **THEN** the firmware initializes serial/logging, Preferences/NVS access, WiFi services, and application modules in a deterministic order

#### Scenario: Initialization failure is visible
- **WHEN** a required platform service fails to initialize
- **THEN** the firmware logs the failed subsystem and avoids starting dependent services with invalid state

#### Scenario: Service startup policy is owned by services
- **WHEN** the application initializes configured runtime modules
- **THEN** it passes loaded configuration and explicit dependencies to those modules without directly deciding service-specific startup or restart policy such as mobile provisioning auto-start or development OTA connected gating

### Requirement: Cooperative runtime loop
The firmware SHALL use a cooperative non-blocking runtime model where `loop()` delegates bounded work to application and module tick methods.

#### Scenario: Loop delegates bounded work
- **WHEN** the Arduino `loop()` function runs
- **THEN** it delegates to the application lifecycle tick and returns without performing long blocking waits

#### Scenario: Multi-step flow uses state machine
- **WHEN** a firmware flow needs retries, timeouts, staged work, or waiting for external events
- **THEN** the flow is represented with the existing StateMachine library or an equivalent explicit state-machine adapter

#### Scenario: Application tick delegates service-owned work
- **WHEN** the application lifecycle advances runtime services during a loop pass
- **THEN** it computes the current runtime timestamp at the application boundary and delegates to service ticks without embedding service-specific branches for provisioning re-entry, provisioning timeout recovery, WiFi fallback provisioning startup, or OTA readiness

### Requirement: Modular firmware boundaries
The firmware SHALL separate platform startup, configuration storage, WiFi management, provisioning transports, OTA handling, and HTTP UI handling into distinct modules with explicit ownership of service policy.

#### Scenario: Module ownership is explicit
- **WHEN** implementation code is added
- **THEN** each module has a defined responsibility and does not directly modify unrelated module state

#### Scenario: Future features can extend baseline
- **WHEN** a future capability needs persisted settings or network lifecycle events
- **THEN** it can integrate through explicit configuration, dependency, and application lifecycle boundaries without rewriting the WiFi manager or expanding `App` with feature-specific runtime policy

#### Scenario: Service dependencies are explicit
- **WHEN** a service needs another module's state or actions to decide its own runtime behavior
- **THEN** those dependencies are provided through constructor or `begin(...)` interfaces instead of `App` reading one service and manually controlling another service
