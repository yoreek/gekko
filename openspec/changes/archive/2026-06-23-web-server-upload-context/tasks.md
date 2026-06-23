## 1. Request context plumbing

- [x] 1.1 Add a fixed-size request file table with `files[3]`, `fileCount`, tmp path, original filename, content type, size, and ownership state.
- [x] 1.2 Extend the shared controller/request layer so actions can access uploaded files alongside parsed params.
- [x] 1.3 Add helpers for claiming or moving an uploaded tmp file into permanent storage.

## 2. Streaming upload handling

- [x] 2.1 Add a configurable tmp directory for request uploads.
- [ ] 2.2 Generate unique tmp filenames with bounded random retries and collision checks.
- [ ] 2.3 Wire the web-server body/upload callbacks to stream incoming file bytes into tmp files.
- [ ] 2.4 Support chunked uploads without buffering the entire file in RAM.
- [ ] 2.5 Track multiple uploaded files per request up to the fixed file-table capacity.

## 3. Cleanup lifecycle

- [x] 3.1 Delete unclaimed tmp files when controller execution finishes.
- [x] 3.2 Preserve claimed files during cleanup and skip deleting files that were moved or adopted by the handler.
- [ ] 3.3 Handle disconnect and error paths so partially written tmp files are cleaned up safely.

## 4. Controller integration and tests

- [ ] 4.1 Update representative portal controllers to read uploaded files from the request context when needed.
- [ ] 4.2 Add unit tests for upload context visibility, file claiming, and cleanup behavior.
- [ ] 4.3 Add integration coverage for chunked upload handling and tmp file deletion after request completion.
