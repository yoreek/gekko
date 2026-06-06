## ADDED Requirements

### Requirement: Dependency-gated runtime services
Runtime services that depend on WiFi, TCP/IP, sockets, AsyncTCP, DNS, MQTT, OTA, or HTTP server backends SHALL gate backend initialization on explicit dependency readiness instead of assuming construction order or begin-time application ticks are sufficient.

#### Scenario: Dependent service waits for readiness
- **WHEN** a runtime service depends on WiFi or network stack functionality and its dependency is not ready
- **THEN** the service remains in an idle or dependency-wait state without starting its network backend

#### Scenario: Dependent service starts after readiness
- **WHEN** the required dependency becomes ready during a cooperative loop pass
- **THEN** the service advances through an explicit startup state and starts its backend from its own tick flow

#### Scenario: Dependency wait is diagnosable
- **WHEN** a service cannot start because a dependency is unavailable
- **THEN** the service provides bounded diagnostics through the local debug layer or test-visible state

#### Scenario: WiFi loss is handled by service policy
- **WHEN** a running WiFi-dependent service observes that a required WiFi dependency is lost
- **THEN** it either remains safely running if the backend supports reconnects or moves through an explicit stop/restart/fault state owned by that service

#### Scenario: WiFi stack stop invalidates network services
- **WHEN** WiFi is explicitly stopped or placed into `WIFI_OFF`, `WIFI_MODE_NULL`, or another down mode that invalidates WiFi/TCP-IP dependencies
- **THEN** WiFi-dependent services stop or fault their network backends and wait for dependency readiness before restarting

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

#### Scenario: Begin configures without runtime ticks
- **WHEN** `App::begin()` completes service construction and dependency wiring
- **THEN** it does not call timing-aware runtime service `tick(...)` methods to force dependency side effects before the Arduino `loop()` starts

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

#### Scenario: Runtime service ticks run from loop
- **WHEN** runtime services need to start, retry, stop, or recover
- **THEN** those transitions are advanced by `App::tick()` during the cooperative loop rather than by duplicate service ticks from `App::begin()`

#### Scenario: Runtime services use tick naming
- **WHEN** a runtime service exposes cooperative work to the application lifecycle
- **THEN** it uses `tick(uint32_t now)` for runtime advancement, optional `end()` for explicit shutdown, and reserves `loop()` naming for the Arduino entrypoint

#### Scenario: State machine services avoid trivial tick wrappers
- **WHEN** a runtime service inherits the shared `StateMachine` helper and needs only normal state advancement
- **THEN** it uses the inherited `StateMachine::tick(now)` directly rather than adding a service-local wrapper that only forwards to the state-machine implementation

#### Scenario: State machine services can add bounded tick policy
- **WHEN** a runtime service needs bounded pre- or post-state-machine policy during cooperative runtime
- **THEN** it may override `tick(uint32_t now)` and call `StateMachine::tick(now)` as part of that service-owned policy
