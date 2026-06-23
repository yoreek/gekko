## Context

The current portal controllers already use a Rails-like action model, but request bodies for JSON endpoints are often buffered into memory before dispatch. That is workable for small JSON payloads, but it is a poor fit for file uploads and makes it difficult to support controller-visible uploaded files with tmp lifecycle semantics.

This change is infrastructure-level: it affects request parsing, controller context, temporary storage, and cleanup behavior across the web-server layer.

## Goals / Non-Goals

**Goals:**
- Expose uploaded files to controller actions through request context.
- Stream file data into tmp storage instead of buffering whole files in RAM.
- Allow controllers to claim or move uploaded files before cleanup runs.
- Clean up any unclaimed tmp files automatically when controller execution finishes.
- Preserve existing query/body param behavior and controller routing style.
- Keep upload state bounded per request with a fixed-size file table.
- Allow the web server to configure the tmp directory and generate bounded unique tmp file names.

**Non-Goals:**
- Redesign portal business endpoints.
- Mandate multipart-only or raw-only uploads at the API boundary for every controller.
- Add a new storage backend beyond tmp file handling in LittleFS.
- Replace the existing controller dispatch model.

## Decisions

### Add upload state to request context
The controller request context will own uploaded file metadata and temporary file handles, just like it already owns request-scoped parsed data. The file table will be fixed-size, initially `files[3]`, with a separate `fileCount` that tracks how many slots are in use.

Why:
- Keeps file handling available to every action without inventing a separate upload service per controller.
- Matches the Rails-style idea that actions receive one request context with params and files.

Alternatives considered:
- Make each controller re-implement upload handling: rejected because it would duplicate lifecycle logic.
- Hide uploads behind a separate service only: rejected because actions need direct access to request-local files.

### Bound tmp storage and file naming
The web server will own a configurable tmp directory and generate unique tmp file names using bounded random retries plus existence checks.

Why:
- Keeps temporary file placement explicit and configurable.
- Avoids collisions without requiring large global registries.
- Lets the controller layer clean up by path and claim state.

Alternatives considered:
- Use a single hard-coded tmp path: rejected because it makes future storage changes harder.
- Use monotonically increasing global counters only: rejected because restarts and collisions complicate recovery.

### Stream bodies into tmp files
Uploaded file bytes will be written incrementally to tmp storage as they arrive.

Why:
- Prevents large file payloads from consuming RAM.
- Makes upload size proportional to chunk buffer size, not file size.
- Fits the existing `ESPAsyncWebServer` chunk callbacks.

Alternatives considered:
- Buffer entire body in `_tempObject`: rejected because it breaks down for file-sized payloads.
- Parse directly into final storage during receipt: rejected because controllers need a chance to validate before claiming data.

### Treat cleanup as controller-finalization behavior
Temporary files remain owned by the request until the controller action claims them or the request ends. At the end of controller execution, any remaining unclaimed tmp files are deleted.

Why:
- Prevents tmp leakage.
- Lets controller actions move or adopt files without fighting automatic cleanup.
- Keeps ownership semantics explicit and predictable.

Alternatives considered:
- Leave cleanup to individual controllers: rejected because it is easy to forget and leads to leaks.
- Delete all tmp files unconditionally: rejected because it would break file adoption workflows.

### Keep action semantics unchanged
Standard `index`, `create`, `update`, `destroy`, and custom controller actions will continue to define business behavior. Upload support is added to the request plumbing, not to route semantics.

Why:
- Preserves the current Rails-like controller shape.
- Avoids a separate upload-only API surface for core controller behavior.

Alternatives considered:
- Introduce upload-specific endpoints only: rejected because upload needs to work across existing controller actions.

## Risks / Trade-offs

- [Upload lifecycle bugs] -> Keep request-owned file metadata explicit and delete only files that remain unclaimed at finalize time.
- [Tmp storage exhaustion] -> Bound file size where possible and fail early if temp storage cannot be allocated.
- [Tmp name collisions] -> Generate names with bounded retries and reject the upload if no free tmp name can be reserved.
- [Controller complexity] -> Centralize upload plumbing in the shared controller layer rather than per-controller code.
- [Backward compatibility] -> Preserve current param parsing paths while adding files as an additional request-scoped input.

## Migration Plan

1. Add request-context structures for uploaded file metadata and tmp handles.
2. Add a fixed-size request file table with `fileCount` and per-file metadata.
3. Wire upload chunk callbacks into the shared controller/request layer.
4. Add claim/move semantics so controllers can adopt a tmp file.
5. Add controller-finalization cleanup for any unclaimed tmp files.
6. Update controllers that need file access to read from the request context.

## Open Questions

- Should the request context expose files as a fixed-size array only, or also a convenience iterator?
- Should uploaded files be addressed by field name, original filename, or both?
- Should tmp name retries use a hard limit per request or a shared per-boot entropy pool?
