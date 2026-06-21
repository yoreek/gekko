## ADDED Requirements

### Requirement: Mixed icon registry resolution
The web app SHALL resolve icons through a registry that supports multiple icon sources. Standard UI and action icons SHALL be backed by `@mdi/js`, while project-specific domain icons SHALL remain available through local SVG definitions.

#### Scenario: Standard UI icon resolves from mdi
- **WHEN** a component requests a standard action icon such as close, edit, refresh, or menu
- **THEN** the registry SHALL return the corresponding `@mdi/js` icon definition

#### Scenario: Domain icon resolves from local SVG
- **WHEN** a component requests a project-specific icon such as portal, wifi, device, ota, or system
- **THEN** the registry SHALL return the local SVG definition for that icon

### Requirement: Selected mdi imports only
The web app SHALL import only the specific `@mdi/js` icon exports that are used by the application. The implementation SHALL NOT use wildcard imports or include the full mdi icon set.

#### Scenario: Build includes only used mdi icons
- **WHEN** the application bundle is built for production
- **THEN** only the explicitly imported mdi icons SHALL be present in the bundle

### Requirement: Bundle size budget
The web app SHALL preserve the project bundle size budget of 500 kB maximum for the relevant production bundle after the icon migration.

#### Scenario: Post-migration bundle check
- **WHEN** the migration is complete and the production build is generated
- **THEN** the bundle size SHALL remain at or below 500 kB
- **AND** the bundle verification SHALL be performed as part of the change validation

### Requirement: Unknown icon fallback
The web app SHALL provide a deterministic fallback icon when a requested icon name is not registered.

#### Scenario: Unknown icon name
- **WHEN** a component requests an icon name that is not present in the registry
- **THEN** the registry SHALL return the default portal icon instead of failing
