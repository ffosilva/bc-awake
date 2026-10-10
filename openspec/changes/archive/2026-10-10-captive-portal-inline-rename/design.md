# Design: Captive-Portal Inline Controller Rename

## Context

See `proposal.md` for motivation. The captive setup portal runs as a local web server on the ESP32 (`192.168.4.1`) served to clients connecting to the SoftAP. Captive portal browsers (iOS CNA WebSheet, Android CaptivePortalLogin) strictly disable or suppress native browser dialogs (`window.prompt`, `window.alert`, `window.confirm`). Currently, `app/index.html` relies on `window.prompt` to rename bound controllers, causing clicks on the "Rename" button to silently do nothing.

## Goals / Non-Goals

**Goals:**
- Provide an inline editing UI within the bound controllers list that works reliably across all captive portal browsers.
- Support keyboard shortcuts (`Enter` to save, `Escape` to cancel) and touch/click actions for saving and canceling.
- Ensure error messages are displayed within the DOM rather than triggering `alert()`.
- Keep the bundle footprint minimal without external dependencies.

**Non-Goals:**
- Modifying the backend `/api/controllers/rename` REST endpoint or parameters.
- Changing controller NVS storage formats or capacities.
- Adding complex multi-step modal dialogs for basic text renaming.

## Decisions

### Decision 1: Inline Row Editing vs. Modal Dialog
- **Choice**: Inline row editing directly within `renderBoundList()`.
- **Rationale**: An inline editor maintains spatial context with the controller being renamed and requires fewer DOM nodes and z-index layers than a floating modal. It feels fast, responsive, and native on mobile touchscreens.
- **Alternatives Considered**:
  - *Dedicated Modal Overlay*: Reusing the modal styles from the pairing listener modal. While functional, it covers the list view and adds unnecessary modal state management for a single text field edit.

### Decision 2: State Tracking and DOM Rendering
- **Choice**: Track an `editingMac` string variable in client-side state.
- **Rationale**: Only one controller can be renamed at a time. When a user clicks "Rename", `editingMac` is set to `c.mac` and `renderBoundList()` re-renders the item in edit mode:
  ```html
  <div class="ctrl-item editing">
    <div class="ctrl-info">
      <input class="rename-input" value="DualSense" maxlength="32" />
      <small>aa:bb:cc:dd:ee:ff</small>
    </div>
    <div class="ctrl-actions">
      <button class="btn-sm btn-save">Save</button>
      <button class="btn-sm btn-cancel secondary">Cancel</button>
    </div>
  </div>
  ```
- **Alternatives Considered**:
  - *Dynamic DOM node replacement without re-render*: Modifying only inner elements of the specific `.ctrl-item`. While slightly faster, the list has at most 8 controllers, so re-rendering via `renderBoundList()` is instantaneous and avoids fragile manual DOM manipulation.

### Decision 3: In-DOM Error Feedback
- **Choice**: Display operation errors in `#dev-msg` or row-level text rather than invoking `alert(...)`.
- **Rationale**: Captive portal environments also silence `alert()`. In-DOM messaging ensures that validation or network errors are visible to the user.

## Risks / Trade-offs

- **[Risk] Multiple rapid edits or blur conflicts** → Mitigation: Disable save/cancel buttons while the asynchronous rename request is pending, and handle `Enter` / `Escape` keys cleanly.
- **[Risk] Long controller names overflowing row layout** → Mitigation: Set `maxlength="32"` matching the backend storage limit and use flex wrapping / `box-sizing: border-box`.
