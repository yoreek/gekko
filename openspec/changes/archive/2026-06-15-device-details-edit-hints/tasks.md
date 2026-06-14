## 1. Shared detail dialog structure

- [x] 1.1 Add shared edit-mode state to the device detail dialog and keep readonly as the default mode
- [x] 1.2 Use Vuetify `VTooltip` and `VInput`/`VTextField` hint props for device detail field guidance
- [x] 1.3 Extend the device UI registry so detail components can declare whether they support edit mode and field hints

## 2. Switch detail hints and edit form

- [x] 2.1 Add localized hints for `Safe state`, `Startup state`, and `Restore previous state`
- [x] 2.2 Render those hints with Vuetify-native helper text or tooltip patterns that stay compact
- [x] 2.3 Add editable switch detail controls for supported fields while keeping unsupported fields readonly
- [x] 2.4 Wire save and cancel actions to the existing device update flow and refresh the dialog from the returned snapshot

## 3. Entry-point consistency and tests

- [x] 3.1 Ensure the Devices page opens the same shared dialog behavior as the dashboard entry point
- [x] 3.2 Add an `Actions` column in the Devices table with `Edit` and `Delete`
- [x] 3.3 Ensure edit and delete mutations keep the dialog and table synchronized through the shared realtime store
- [x] 3.4 Add localization keys for the new hint text and edit-mode labels
- [x] 3.5 Add browser or component tests covering readonly view, edit mode, save, cancel, hint rendering, table actions, and realtime sync after mutation
