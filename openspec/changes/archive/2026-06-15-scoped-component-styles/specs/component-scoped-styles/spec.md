## ADDED Requirements

### Requirement: Component-owned styles use scoped blocks
The SPA SHALL define component-owned styles in `<style scoped>` blocks within the Vue component that owns the markup.

#### Scenario: A component owns its local layout
- **WHEN** a Vue component defines selectors for its own classes
- **THEN** those selectors SHALL live in a scoped style block in that component file

#### Scenario: A shared global selector is not required
- **WHEN** a style only affects markup owned by one component
- **THEN** the style SHALL NOT be added to the global stylesheet

### Requirement: Global stylesheet is reserved for shared rules
The SPA SHALL keep `portal-spa/src/styles/main.css` limited to global reset, theme tokens, app shell styles, and shared primitives used across multiple views or components.

#### Scenario: Theme tokens are global
- **WHEN** the portal defines layout tokens, theme variables, or app-shell surface rules
- **THEN** those rules SHALL remain in `main.css`

#### Scenario: Component selectors stay local
- **WHEN** a selector applies only to a single component or view
- **THEN** that selector SHALL be moved into the owning component instead of remaining in `main.css`

