## Purpose

Define the shared web-server upload context used by portal controllers.

## Requirements

### Requirement: Controller request context includes uploaded files
The web server MUST attach uploaded file metadata to the controller request context so any action can inspect files alongside parsed params.

#### Scenario: Action sees request files
- **WHEN** a controller action is invoked after an upload request completes
- **THEN** the action can read the uploaded file list from the request context

#### Scenario: Action can ignore files
- **WHEN** a controller action does not need uploaded files
- **THEN** it can continue using the request context without special upload handling

#### Scenario: File table is bounded
- **WHEN** a request receives uploaded files
- **THEN** the request context stores them in a fixed-size file table that supports up to three files per request

#### Scenario: File count is tracked
- **WHEN** a request receives uploaded files
- **THEN** the request context tracks the number of populated file slots separately from the file metadata array

### Requirement: Uploaded files are stored in tmp during request processing
The web server MUST stream uploaded file data into temporary storage using unique tmp file names instead of buffering entire files in RAM.

#### Scenario: File upload is chunked
- **WHEN** upload data arrives in chunks
- **THEN** the server appends each chunk to the corresponding tmp file without requiring the full file in memory

#### Scenario: Tmp file name is unique
- **WHEN** multiple uploads are processed
- **THEN** each temporary file is written to a unique tmp path

#### Scenario: Tmp directory is configurable
- **WHEN** the web server is configured for upload handling
- **THEN** it uses a configured tmp directory as the root for uploaded file storage

#### Scenario: Tmp name collisions are bounded
- **WHEN** the server generates a tmp filename
- **THEN** it retries with a new random candidate and rejects the upload after a bounded number of failed collision checks

### Requirement: Controller actions may claim uploaded files
The web server MUST allow a controller action to claim or move a tmp file so it is no longer subject to automatic cleanup.

#### Scenario: Action claims file ownership
- **WHEN** a controller action moves an uploaded file to permanent storage
- **THEN** the web server marks that tmp file as claimed and does not delete it during request cleanup

#### Scenario: Unclaimed file remains temporary
- **WHEN** a controller action does not claim an uploaded file
- **THEN** the file remains temporary until request cleanup

#### Scenario: File metadata includes optional original name
- **WHEN** the client supplies an original filename for an upload
- **THEN** the request context stores that filename in the uploaded file metadata

#### Scenario: Missing original name is tolerated
- **WHEN** the client does not supply an original filename
- **THEN** the request context still records the uploaded tmp file and leaves the original filename field empty

### Requirement: Unclaimed tmp files are deleted after controller execution
The web server MUST delete any uploaded tmp files that remain unclaimed when controller execution finishes.

#### Scenario: Request cleanup removes tmp files
- **WHEN** a controller action returns or fails and the request finishes
- **THEN** the web server deletes all uploaded tmp files that were not claimed by the action

#### Scenario: Claimed files survive cleanup
- **WHEN** a controller action claimed one or more uploaded files
- **THEN** cleanup leaves the claimed files in their new location
