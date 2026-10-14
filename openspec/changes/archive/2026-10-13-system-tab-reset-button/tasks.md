# Tasks

## 1. Backend Configuration Reset Support

- [x] 1.1 Implement `resetConfig()` in `include/config.h` and `src/config.cpp` that clears the `"bc250"` NVS namespace via `prefs.clear()`, sets `forceSetup` to `true` in NVS, and resets the in-memory `config` struct to factory defaults. Verify compilation and symbol declarations.
- [x] 1.2 Implement authenticated HTTP POST `/api/reset` endpoint handler in `src/portal.cpp` that checks authentication with `authed(req)`, calls `resetConfig()`, clears `g_token`, returns HTTP 200 with `{"ok":true}`, and schedules device restart after 800 ms. Verify endpoint registration in `registerRoutes()`.

## 2. Web Portal User Interface

- [x] 2.1 Add the Reset Configuration card to the System tab (`#panel-sys`) in `app/index.html` with an initial trigger button, explanatory description, and an in-page confirmation section containing warning text, confirm action, and cancel action.
- [x] 2.2 Wire client-side event handlers in `app/index.html` to toggle the in-page confirmation, handle cancellation, submit `POST /api/reset`, stop active polling timers, and render a persistent reboot notice directing the user to reconnect to the SoftAP.
- [x] 2.3 Rebuild `include/portal_html.h` by running `python3 scripts/build_html.py` and verify the generated header reflects the updated `app/index.html`.

## 3. Verification & Documentation

- [x] 3.1 Run `pio run` across supported target environments to verify firmware compilation without warnings or errors.
- [x] 3.2 Update `README.md` to document the System tab Reset configuration action and behavior.
