# Design

## Context

See `proposal.md` for background and motivation.
In production, users access the suite via `https://robotica.ouroboros-software.com` (served via Cloudflare and an upstream web server). Because the upstream web server lacks reverse-proxy definitions for `/api/` and `/ws/`, WebSocket handshakes and POST requests return `index.html` (200 OK) or 405 Method Not Allowed respectively. Moreover, microcontrollers connected via USB to the user's laptop cannot be flashed by `avrdude` running on a remote cloud server.

## Goals / Non-Goals

**Goals:**
- Eliminate continuous WebSocket retry spam in the browser console by introducing exponential backoff and maximum retry thresholds with a clean UI offline indicator.
- Prevent impossible remote `avrdude` flash attempts by detecting remote web execution, providing friendly guidance (use desktop app `LineFollowerPro.exe` or local companion service), while preserving 100% in-browser Web Serial flashing for ESP32/ESP32-S3 boards.
- Catch HTTP 405 / network errors gracefully with clear contextual messages rather than uncaught Promise rejections.
- Deliver a production-grade Nginx configuration file (`nginx-robotica.conf`) and update `manage-services.sh` to ensure self-hosted servers correctly proxy `/api/` and `/ws/` to the .NET service.

**Non-Goals:**
- Porting `avrdude` C code to WebAssembly in this phase.
- Modifying the native desktop Tauri application's direct serial communication stack.

## Decisions

### Decision 1: Resilient WebSocket Reconnection with Backoff
- **Approach:** In `frontend/src/lib/tauriBridge.js`, replace the fixed `setTimeout(connectWs, 2000)` loop with an exponential backoff formula:
  $$\text{delay} = \min(2000 \times 2^{\text{retries}}, 30000)$$
  After 5 consecutive failures, stop automated reconnection and notify subscribers of an offline state.
- **Alternatives Considered:** Infinite retry with fixed delay (current approach; spams console and wastes CPU/battery when server proxy is down).

### Decision 2: Environment-Aware Flasher Validation
- **Approach:** When flashing Arduino Nano (IM-16 / CODEX-8):
  - In native desktop Tauri (`isTauri()`): execute `invoke('flash_firmware')`.
  - In local browser (`window.location.hostname === 'localhost' || window.location.hostname === '127.0.0.1'`): execute `POST /api/firmware/flash` towards local backend.
  - In remote web browser (`robotica.ouroboros-software.com` or other external domains): check if backend responds with valid JSON; if 405 or network failure occurs, intercept and display a helpful modal explaining that Arduino Nano USB flashing via `avrdude` requires the desktop app (`LineFollowerPro.exe`) or running the local companion backend.
  - For ESP32 / ESP32-S3: continue utilizing Web Serial API (`esp32WebFlasher.js`), which runs directly in the client browser tab and has direct USB access.

### Decision 3: Production Nginx Reverse Proxy Template
- **Approach:** Add `deploy/nginx/linefollower.conf` containing the exact `proxy_pass` blocks for `/ws/` (with HTTP 1.1 and `Upgrade` headers) and `/api/` to port 5010, alongside the static SPA root fallback.

## Risks / Trade-offs

- **[Risk: User might think Arduino Nano cannot be flashed at all on web]** → Mitigation: Explicitly distinguish between ESP32 (supported directly in web via Web Serial) and Arduino Nano (requires desktop app or local backend for avrdude driver access).
- **[Risk: Stale UI state if WebSocket ceases retrying]** → Mitigation: Provide a visible "Reconectar Telemetría" button in the status bar/banner so the user can re-trigger connection on demand.
