# Design

## Context

The BC250 setup portal is a single-page web application (`app/index.html`) embedded directly into firmware memory via `scripts/build_html.py` and served by ESPAsyncWebServer.

Currently, the initial password screen (`#screen-pass`) is rendered as a plain `<div>` containing `<input id="pw" type="password" autocomplete="new-password">` and `<button id="pass-btn">`. Because there is no enclosing `<form>` element:
- Pressing `Enter` inside the input does not submit the form.
- The hardcoded `autocomplete="new-password"` prevents browser password managers from identifying the unlock screen as a credential login.
- Browsers' credential managers do not observe standard form submission events or trigger "Save password?" prompts.

See [proposal.md](file:///Users/ffosilva/repos/ffosilva/bc250-esp32-switch/openspec/changes/improve-portal-password-form/proposal.md) for background motivation and [specs/setup-portal/spec.md](file:///Users/ffosilva/repos/ffosilva/bc250-esp32-switch/openspec/changes/improve-portal-password-form/specs/setup-portal/spec.md) for requirements.

## Goals / Non-Goals

**Goals:**
- Convert `#screen-pass` or its inner controls into a semantic `<form id="pass-form">` with `<button type="submit" id="pass-btn">`.
- Enable native `Enter` key submission from password inputs (`#pw` and `#pw2`).
- Dynamically toggle input attributes between `current-password` (unlock) and `new-password` (creation).
- Support password-only credential capture without requiring or imposing a dummy username.
- Intercept form submission with `e.preventDefault()` to maintain asynchronous `fetch()` API calls while preserving inline validation and UI transitions.

**Non-Goals:**
- Modifying backend C++ endpoints (`/api/login` and `/api/password` JSON payloads remain `{ "password": "..." }`).
- Introducing multi-user management or user accounts.
- Changing the password modification card in the System tab (already a separate post-authentication action).

## Decisions

### 1. Semantic Form Wrapper and Submit Event Handling
- **Decision:** Convert `#screen-pass` into a `<form>` element:
  ```html
  <form class="card hidden" id="screen-pass" action="/api/login" method="post" autocomplete="on">
    ...
    <button type="submit" id="pass-btn">Unlock</button>
  </form>
  ```
  In JavaScript, attach the handler to `$("screen-pass").onsubmit = async (e) => { e.preventDefault(); ... }`.
- **Rationale:** Browsers natively trigger the form `submit` event when the user presses `Enter` on any text/password input within a `<form>`. Calling `e.preventDefault()` prevents standard page reload while letting the asynchronous `api()` fetch execute. When login succeeds, hiding `#screen-pass` signals successful completion to the browser's credential manager.
- **Alternatives considered:** Adding custom `keydown` listeners for keyCode 13 / `key === 'Enter'`. Rejected because it does not satisfy browser password manager heuristics and requires manual keyboard handling across mobile and desktop browsers.

### 2. Credential Identity & Empty Username
- **Decision:** Use a single-password form structure with no pre-filled dummy username. An empty username input `<input type="text" name="username" value="" autocomplete="username" style="display:none" tabindex="-1" aria-hidden="true" />` or direct password-only form will be used to signal an empty/blank username identity.
- **Rationale:** Modern password managers (iCloud Keychain, Chrome Password Manager, Bitwarden, 1Password) support single-password credentials scoped to the web origin (e.g. `bc-awake.local` or `192.168.4.1`). If the browser prompts for a username, the field remains empty for the user to confirm or leave blank, per the user's explicit preference.
- **Alternatives considered:** Hardcoding `value="admin"`. Rejected per user decision.

### 3. Dynamic Autocomplete & Name Configuration
- **Decision:** In `initPassScreen(passwordSet)`:
  - When creating a password (`creating == true`):
    - `form.action = "/api/password"`
    - `#pw` attributes: `name="new-password"`, `autocomplete="new-password"`
    - `#pw2` attributes: `name="new-password"`, `autocomplete="new-password"`
  - When unlocking (`creating == false`):
    - `form.action = "/api/login"`
    - `#pw` attributes: `name="password"`, `autocomplete="current-password"`
- **Rationale:** Using `autocomplete="current-password"` during unlock allows browsers to suggest autofilling existing saved credentials, while `autocomplete="new-password"` during creation allows password managers to generate strong passwords and offer to save the newly created secret.

## Risks / Trade-offs

- **[Risk] Accidental page reload if form submit handler throws synchronously before `e.preventDefault()`**
  → *Mitigation:* Call `e.preventDefault()` as the very first line of the `onsubmit` handler.
- **[Risk] Premature submission on mobile virtual keyboards when tapping 'Next' vs 'Go'**
  → *Mitigation:* Retain existing validation checks (`pw.length >= 6`, password matching when creating) before sending the network request; display errors inline without hiding the form.
