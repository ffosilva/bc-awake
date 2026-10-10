# Tasks

## 1. Portal Web UI Implementation

- [x] 1.1 Add CSS styles for `.ctrl-item.editing`, inline rename inputs, and action buttons in `app/index.html`, and verify layout visually matches the existing dark theme.
- [x] 1.2 Implement `editingMac` state management, inline editor rendering, and Cancel/Escape key handling in `renderBoundList()`, and verify canceling reverts to static display without network requests.
- [x] 1.3 Implement inline Save action and Enter key trigger in `app/index.html` calling `/api/controllers/rename`, replacing `window.prompt()` and `window.alert()` with in-DOM error handling and list refresh upon success.

## 2. Verification & Asset Build

- [x] 2.1 Validate `app/index.html` with browser simulation or syntax check, confirming no references to `window.prompt` remain in the controller management flow.
- [x] 2.2 Verify firmware compilation and filesystem asset build (`pio run`) across supported targets.
