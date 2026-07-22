# Gekko REST API schemas

This directory is the versioned, repository-only contract for Gekko's REST
API. It is deliberately not compiled into the firmware and is not served by
the controller.

The schemas describe the JSON shape accepted by the public API. Firmware REST
adapters and the device-setup importer remain the final authority for runtime
and graph validation.

## Native adapter smoke checks

The corresponding native adapter tests load these schema files and validate
their own in-memory `JsonDocument` request and response values. This catches a
schema that no longer describes JSON actually accepted or emitted by an
adapter, without compiling schemas into the firmware or adding sample payloads
as a second source of truth.

`test/test_devices/JsonSchemaSmokeValidator.h` intentionally implements only
the JSON Schema vocabulary used here. When a schema needs another keyword, add
its test-only validation there before relying on that keyword in the contract.

## Layout

- `common/` contains reusable JSON Schema definitions.
- `devices/` contains one configuration schema per public `typeName`.
- `requests/` and `responses/` describe REST endpoint envelopes.
- `bundle/` describes the NDJSON device-setup transfer format.

`device.config` in a device-setup bundle uses the same type-specific schema as
the REST create request. Update requests are partial: omitted fields retain the
current device configuration, matching `TypedDeviceApiAdapter` semantics.

## What JSON Schema cannot validate

The schemas can validate JSON shape, primitive ranges, enums, and the declared
dependency record shape. They cannot prove that a dependency ID exists in the
same bundle or provides the required role. Gekko validates those cross-record
and runtime rules during create, update, and import.

## Versioning

`v1` versions this public schema set, independently from individual firmware
config versions and `transferSchemaVersion`. Additive compatible changes may
extend a schema; incompatible public changes require a new directory.
