# Spec Delta

## ADDED Requirements

### Requirement: Portal Password Form Submission and Credential Management
The portal web interface SHALL present the initial password creation and unlock screen within a semantic form that supports keyboard submission via the Enter key and provides standard credential attributes enabling web browser password managers to offer password saving without requiring a username.

#### Scenario: Submitting unlock via Enter key
- **WHEN** a user enters their password into the unlock input field and presses the Enter key
- **THEN** the interface dispatches the form submission, validates the input, and sends an authentication request to `/api/login` without requiring the user to click the Unlock button

#### Scenario: Submitting first-time password creation via Enter key
- **WHEN** a user enters matching passwords into the creation inputs and presses the Enter key in either field
- **THEN** the interface dispatches the form submission and sends the setup request to `/api/password` without requiring the user to click the Continue button

#### Scenario: Browser credential manager password saving compatibility
- **WHEN** a user successfully unlocks or creates a portal password
- **THEN** the password form provides semantic autocomplete attributes (`current-password` when unlocking, `new-password` when creating) and signals successful form submission to the browser to prompt the browser credential manager to save the password without requiring a username
