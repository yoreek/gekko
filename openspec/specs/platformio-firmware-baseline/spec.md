## Purpose

Define the PlatformIO and firmware baseline for the ESP32 WiFi manager.

## Requirements

### Requirement: PlatformIO ESP32 project baseline
The project SHALL provide a PlatformIO firmware project targeting the ESP32 development board with the Arduino framework and C++17 build settings.

#### Scenario: Build target is configured
- **WHEN** a developer opens the PlatformIO project
- **THEN** the project defines an `esp32dev` environment using `espressif32@6.9.0`, `framework = arduino`, MCU `esp32`, CPU frequency `240000000L`, flash frequency `40000000L`, and C++17-compatible build flags

#### Scenario: Existing project profile conventions are preserved
- **WHEN** the PlatformIO configuration is created
- **THEN** it uses shared `[env]` defaults, high-speed serial/upload settings, ESP32 exception decoder monitor filtering, custom partition file support, LittleFS filesystem configuration, and an OTA upload environment extending the base ESP32 environment

### Requirement: Baseline dependency profile
The project SHALL include the Arduino libraries needed for WiFi management, async HTTP portal behavior, JSON processing, and reuse of existing local project code while keeping domain-specific features optional.

#### Scenario: Required baseline libraries are declared
- **WHEN** a clean PlatformIO dependency install is performed
- **THEN** the project declares required libraries for ArduinoJson, StreamUtils, ESPAsyncWebServer, AsyncTCP, and the existing local ReefDuino dependency when available

#### Scenario: Domain libraries remain feature-gated
- **WHEN** libraries for MQTT, sensors, Home Assistant, OTA, time zones, or one-wire devices are present in the shared profile
- **THEN** their behavior is controlled by build flags so the WiFi manager baseline can compile without enabling unrelated runtime modules

### Requirement: Host Unity test baseline
The project SHALL include a PlatformIO Unity testing baseline for deterministic firmware logic that can run without a connected microcontroller.

#### Scenario: Unity tests are runnable
- **WHEN** a developer runs the host/off-device Unity test command
- **THEN** Unity test suites compile and run for logic that does not require ESP32 hardware state

#### Scenario: Hardware-dependent tests are separated
- **WHEN** a behavior requires real WiFi radio, BLE/mobile provisioning, captive portal client behavior, or actual OTA flashing
- **THEN** it is documented as manual integration coverage rather than mixed into host-only Unity tests

#### Scenario: Firmware builds from clean checkout
- **WHEN** a developer runs the PlatformIO build for the default ESP32 environment
- **THEN** the firmware compiles without requiring generated local files outside the repository

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

### Requirement: Resource-conscious runtime behavior
The firmware SHALL avoid unnecessary dynamic allocation, blocking loops, and unbounded input handling in boot-critical and network-critical paths.

#### Scenario: Long operations do not block core services
- **WHEN** WiFi scans, provisioning sessions, or connection attempts are running
- **THEN** the firmware keeps ESP-IDF event handling active and does not use indefinite busy-wait loops

#### Scenario: Delays are bounded
- **WHEN** application modules need timing delays, retry intervals, or timeouts
- **THEN** they use deadline-based state transitions instead of long `delay()` calls

#### Scenario: External input is bounded
- **WHEN** the firmware receives HTTP or JSON configuration input
- **THEN** it enforces size limits before parsing or persisting the input
