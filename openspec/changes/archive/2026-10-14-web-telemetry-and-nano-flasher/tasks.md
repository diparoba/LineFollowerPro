# Tasks

## 1. Resilient WebSocket Telemetry in Web Mode

- [x] 1.1 In `frontend/src/lib/tauriBridge.js`, update `subscribeTelemetry` to use exponential backoff (`2s`, `4s`, `8s`, up to `30s`) and a max retry threshold (5 attempts), avoiding console error spam when the proxy or backend is offline.
- [x] 1.2 In `frontend/src/lib/tauriBridge.js` and `frontend/src/lib/ControlBar.svelte`, expose an offline state callback and a manual "Reintentar Conexión" function to allow users to reconnect on demand.

## 2. Web Flasher UX & Arduino Nano Guidance

- [x] 2.1 In `frontend/src/lib/tauriBridge.js` (`flashFirmware`), intercept HTTP 405 Method Not Allowed and fetch failures, transforming them into a clear message explaining that the remote server does not have access to local COM ports.
- [x] 2.2 In `frontend/src/lib/HardwareDrawer.svelte`, detect remote web hosting (`!isTauri()` and host is not `localhost`/`127.0.0.1`), displaying an informative badge/warning when an Arduino Nano is selected to guide the user to the native desktop app (`LineFollowerPro.exe`) or local companion server, while keeping ESP32 Web Serial flashing active.

## 3. Production Deployment Configuration

- [x] 3.1 Create `deploy/nginx/linefollower.conf` with production-ready `location /ws/` (with HTTP/1.1 and `Upgrade` headers) and `location /api/` reverse-proxy directives to `http://127.0.0.1:5010`.
- [x] 3.2 Update `manage-services.sh` to include a validation/help command for checking Nginx reverse proxy configuration.

## 4. Verification and Build

- [x] 4.1 Rebuild frontend via `pnpm run build` and synchronize assets to `backend/wwwroot/`.
- [x] 4.2 Validate the OpenSpec change using `npx openspec validate web-telemetry-and-nano-flasher`.
