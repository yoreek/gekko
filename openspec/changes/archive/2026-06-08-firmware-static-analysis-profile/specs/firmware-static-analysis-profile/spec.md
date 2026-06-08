## ADDED Requirements

### Requirement: Shared static analysis profile
The firmware project SHALL keep a repo-local static analysis profile that is consumed by PlatformIO and documented for IDE use.

#### Scenario: Tooling reads the same profile
- **WHEN** a developer runs static analysis from the CLI or configures the IDE for the project
- **THEN** the warning profile comes from the repo-local `.clang-tidy` file rather than ad hoc inline analyzer settings

#### Scenario: Local code is the primary target
- **WHEN** static analysis runs
- **THEN** the analysis focuses on repository source and test code instead of treating vendor and system headers as the primary review surface

### Requirement: Lint entrypoint covers both firmware environments
The project SHALL provide a lint entrypoint that runs formatting checks and static analysis for both the `esp32dev` firmware build and the `native` host verification build.

#### Scenario: Firmware environment is checked
- **WHEN** `scripts/lint.sh` runs
- **THEN** it validates code formatting and runs `pio check -e esp32dev` against the firmware source tree

#### Scenario: Native environment is checked
- **WHEN** `scripts/lint.sh` runs
- **THEN** it runs `pio check -e native` against the source and test trees used by host-side verification

### Requirement: Test entrypoint gates on lint
The project SHALL run the lint flow before executing the native Unity test suite.

#### Scenario: Native tests reuse the same verification gate
- **WHEN** `scripts/test.sh` runs
- **THEN** it invokes the lint entrypoint before `pio test -e native`

### Requirement: Static analysis workflow is documented
The project SHALL document the static-analysis workflow, including how to run the checks and how to keep IDE and CLI configuration aligned.

#### Scenario: Developer can follow the documented workflow
- **WHEN** a contributor needs to verify local code quality
- **THEN** the documentation explains the lint and test scripts and the expected static-analysis configuration file
