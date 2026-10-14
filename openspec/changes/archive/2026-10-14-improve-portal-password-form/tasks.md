# Tasks

## 1. Form Markup & Dynamic Attributes

- [x] 1.1 Convert `#screen-pass` in `app/index.html` to a semantic `<form>` element containing an empty username input and `<button type="submit" id="pass-btn">`, verifying the resulting DOM markup.
- [x] 1.2 Update `initPassScreen()` in `app/index.html` to dynamically configure the form `action`, input `name`, and `autocomplete` attributes (`current-password`/`password` for unlock; `new-password`/`new-password` for creation), verifying attributes match the current mode.

## 2. Form Submission Handling & Asset Build

- [x] 2.1 Replace the button `onclick` listener with a form `onsubmit` event listener that invokes `e.preventDefault()`, performs validation, sends the API request, and transitions the UI on success, verifying that pressing `Enter` inside password inputs triggers submission.
- [x] 2.2 Run `python3 scripts/build_html.py` to regenerate `include/portal_html.h` and verify the compressed C array is updated cleanly.

## 3. Verification & Credential Management Testing

- [x] 3.1 Verify native `Enter` key submission on both initial creation and unlock flows in a browser.
- [x] 3.2 Verify that browser credential managers recognize the login and creation forms and offer to save the password with an empty username.
