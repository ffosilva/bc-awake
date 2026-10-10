# Spec Delta: setup-portal

## ADDED Requirements

### Requirement: Captive Portal User Interface Resilience
The setup web interface SHALL perform all user input entry, editing, and confirmations via in-page DOM elements without relying on browser-native dialog prompts (`window.prompt`, `window.alert`, `window.confirm`).

#### Scenario: Activating inline rename for bound controller
- **WHEN** a user clicks the rename button for a bound controller
- **THEN** the row switches to an inline editing state displaying an input pre-filled with the current friendly name alongside Save and Cancel actions, without invoking browser-native prompt dialogs

#### Scenario: Saving inline controller rename
- **WHEN** a user submits the inline edit with a valid non-empty name
- **THEN** the interface sends an authenticated POST request to /api/controllers/rename and updates the bound controller list with the new name upon success

#### Scenario: Canceling inline controller rename
- **WHEN** a user clicks the cancel action during an active inline edit
- **THEN** the row reverts to displaying the current name and standard action buttons without dispatching an API request
