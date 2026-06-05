## ADDED Requirements

### Requirement: Single loop timestamp propagation
The firmware SHALL compute the current runtime timestamp once per cooperative application loop pass and pass that timestamp to timing-aware runtime managers.

#### Scenario: Application tick computes one timestamp
- **WHEN** the application lifecycle advances runtime services during a loop pass
- **THEN** it reads the clock once for that pass and provides the same `now` value to each timing-aware manager tick

#### Scenario: Timing-aware manager accepts explicit time
- **WHEN** a runtime manager evaluates retries, deadlines, session timeout, or StateMachine transitions
- **THEN** its cooperative tick API accepts the loop timestamp as an explicit `uint32_t now` argument

### Requirement: Domain timing avoids hidden clock reads
Domain managers SHALL avoid reading `millis()` or `clock_.millis()` internally for cooperative timing decisions when the application loop has already supplied a timestamp.

#### Scenario: WiFi retry timing uses supplied timestamp
- **WHEN** WiFi connection retry or fallback logic is evaluated
- **THEN** the WiFi manager uses the supplied `now` value for deadline and timeout checks

#### Scenario: Provisioning timeout uses supplied timestamp
- **WHEN** mobile provisioning session timeout or completion state is evaluated
- **THEN** provisioning logic uses the supplied `now` value for state-machine timing

#### Scenario: Platform boundary may read clock
- **WHEN** application startup or the top-level runtime loop needs the current time
- **THEN** the platform clock abstraction MAY read Arduino time and pass the value into domain code

### Requirement: Deterministic host tests for timing flows
Host/off-device tests SHALL be able to drive cooperative timing flows by passing explicit timestamps into runtime manager ticks.

#### Scenario: Test controls retry deadline
- **WHEN** a Unity test verifies retry or timeout behavior
- **THEN** the test advances mocked time and passes the selected timestamp to the manager under test

#### Scenario: Test avoids real-time dependency
- **WHEN** a host test runs without ESP32 hardware
- **THEN** it does not depend on real elapsed wall-clock time for cooperative state-machine transitions
