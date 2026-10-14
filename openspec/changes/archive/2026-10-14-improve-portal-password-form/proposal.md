# Proposal

## Why

When accessing the BC250 setup portal's initial password creation or unlock screen, pressing the "Enter" key does not submit the form, forcing the user to manually click the submit button. Furthermore, modern web browser credential managers fail to detect the login context and do not offer to save or autofill the portal password. Wrapping the input fields in a semantic form with appropriate autocomplete attributes resolves both UX pain points natively across desktop and mobile browsers.

## What Changes

- Wrap the initial password entry elements inside a semantic `<form>` with standard submit semantics.
- Support native `Enter` key submission from password and password confirmation inputs.
- Dynamically apply semantic `name` and `autocomplete` attributes (`current-password` when unlocking, `new-password` when creating) to enable browser password manager detection and saving without requiring a dummy username.
- Attach the submission workflow to the form's `submit` event with `preventDefault()` to preserve asynchronous API communication while signaling completion to browser credential managers upon authentication.

## Capabilities

### New Capabilities
<!-- None -->

### Modified Capabilities
- `setup-portal`: Enhances the password authentication interface to require standard keyboard form submission (Enter key) and browser credential management compatibility for password saving.

## Impact

- `app/index.html`: Password card DOM structure and JavaScript submission handlers.
- `include/portal_html.h`: Regenerated embedded gzipped HTML header via `scripts/build_html.py`.
- No backend C++ endpoints or breaking API changes are introduced.
