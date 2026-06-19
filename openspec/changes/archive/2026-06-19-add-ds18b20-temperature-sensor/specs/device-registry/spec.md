## ADDED Requirements

### Requirement: Parent child lifecycle propagation
The device registry SHALL propagate parent lifecycle changes to child runtime availability and effective status in a way that distinguishes disabled parents from broken or transitional parents.

#### Scenario: Disabled parent makes child effectively disabled
- **WHEN** a parent device is disabled and a child device depends on that parent
- **THEN** the child device stops runtime work and its `effective_status` is reported as `disabled`

#### Scenario: Non-ready parent blocks child dependency
- **WHEN** a parent device is missing, faulted, deleting, reconfiguring, or otherwise not ready for reasons other than being disabled
- **THEN** the child device stops runtime work and its `effective_status` is reported as `dependency_blocked`

#### Scenario: Parent reconfiguration cascades to children
- **WHEN** a parent runtime accepts a config change that reinitializes parent hardware
- **THEN** each attached child runtime is requested to reconfigure after the parent relationship is refreshed

#### Scenario: Parent deletion is rejected with dependent children
- **WHEN** a caller tries to delete a device that still has child devices
- **THEN** the registry rejects deletion, leaves the parent and children unchanged, and reports the dependent child device ids

#### Scenario: Child parent compatibility is enforced
- **WHEN** a child device descriptor declares compatible parent types
- **THEN** create and parent reassignment mutations reject parents whose type id is not in that compatible parent list
