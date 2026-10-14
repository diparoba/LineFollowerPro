# Spec Delta

## ADDED Requirements

### Requirement: Resilient Web Telemetry Reconnection with Backoff
The system SHALL manage WebSocket telemetry connections in web browser mode using exponential backoff and a maximum consecutive retry limit, preventing browser console flooding when the backend service is offline, unreachable, or behind an unconfigured reverse proxy.

#### Scenario: Exponential backoff on failed WebSocket connection
- **WHEN** the browser client attempts to connect to `wss://<host>/ws/telemetry` and the handshake fails
- **THEN** the system delays successive reconnect attempts progressively (e.g. 2s, 4s, 8s, up to 30s) instead of looping aggressively every 2 seconds.

#### Scenario: Maximum retry limit and offline state indication
- **WHEN** WebSocket reconnection fails repeatedly exceeding the maximum attempt threshold (e.g. 5 attempts)
- **THEN** the system halts automated reconnection loops, displays an unobtrusive "Telemetría Desconectada (Modo Offline)" banner in the UI, and provides a manual "Reintentar Conexión" button for the user.
