# Spec Delta

## REMOVED Requirements

### Requirement: Local Bluetooth State Discovery
**Reason**: The `bc250-sync` utility is decommissioned and removed to eliminate runtime network dependencies and radio activity while the host PC is operating.
**Migration**: Pair controllers during onboarding using the Setup Mode web portal or the `bc250-setup` wizard over SoftAP.

### Requirement: REST API Synchronization
**Reason**: The web server is offline during normal operation and background synchronization is removed.
**Migration**: Configure controller pairings and host adapter MAC directly via the Setup Mode web portal.

### Requirement: Systemd Service Integration
**Reason**: Background synchronization service and timers on the host OS are no longer supported or needed.
**Migration**: Remove existing systemd service and timer units (`bc250-sync.service` and `bc250-sync.timer`) from the host OS.
