## Why

The web server currently treats request bodies as fully buffered payloads in several controller paths, which does not scale for file uploads and makes it hard to support Rails-like controller flows. We need a first-class upload context so controllers can receive files chunk-by-chunk, store them in tmp, and decide whether to claim or discard them after the action runs.

## What Changes

- Add a web-server upload context that is available to all controller actions, not just a dedicated import endpoint.
- Stream uploaded file bodies into tmp storage using a bounded request-local file table with unique temporary names instead of buffering whole files in RAM.
- Attach uploaded file metadata to the request context so handlers can access files alongside query/body params.
- Support claiming or moving tmp files into permanent storage from inside a controller action.
- Remove any remaining unclaimed tmp files automatically when controller execution finishes.
- Preserve Rails-like controller routing semantics where `index`, `create`, `update`, `destroy`, and custom actions can all inspect request files when needed.
- **BREAKING**: controllers that currently assume body data is always fully buffered may need to switch to the new request file context.

## Capabilities

### New Capabilities
- `web-server-upload-context`: request-scoped file upload capture, bounded file table, tmp file lifecycle, and controller-visible file metadata.

## Impact

- `ESPAsyncWebServer` request handling integration in the portal/web-server layer.
- `BaseController` and controller dispatch flow for request context cleanup.
- Portal controllers that may accept file uploads in standard actions.
- Tmp storage and file-claim semantics in LittleFS-backed storage.
