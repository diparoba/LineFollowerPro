# Proposal

## Why

When deploying LineFollowerPro to self-hosted production domains (such as `https://robotica.ouroboros-software.com`), web clients experience repeated WebSocket connection failures (`wss://.../ws/telemetry failed`) and HTTP 405 Method Not Allowed errors when invoking `/api/firmware/flash`. These errors stem from two root causes:
1. Web server / reverse proxy configurations (Nginx / Cloudflare) missing dedicated `proxy_pass` blocks for `/api/` and `/ws/` towards the .NET Kestrel service (port 5010), causing requests to fall back to static HTML file handling.
2. In web mode, invoking backend-side `avrdude` via `/api/firmware/flash` fails because a remote cloud server cannot physically access local USB ports (`COM*`) connected to the client machine. Additionally, failed WebSockets trigger an aggressive 2-second reconnect loop that spams the browser console.

## What Changes

- **Resilient Web Telemetry:**
  - Update `tauriBridge.js` to implement exponential backoff and maximum retry attempts for WebSocket reconnections when operating in web mode.
  - Surface a clear offline / disconnected indicator in the UI instead of flooding the browser console with unhandled WebSocket reconnect failures.
- **Web-Aware Hardware Flasher UX & Fallback:**
  - In `HardwareDrawer.svelte` and `tauriBridge.js`, detect when running in remote web mode vs desktop native / local mode.
  - For ESP32 / ESP32-S3 boards, preserve the pure in-browser Web Serial flasher (`esp32WebFlasher.js`).
  - For Arduino Nano (IM-16 / CODEX-8), intercept flash attempts on remote web hosts and clearly inform the user that local hardware flashing requires either the native desktop application (`LineFollowerPro.exe`) or running the local companion backend (`http://localhost:5010`), preventing impossible HTTP 405 / network failures.
- **Production Reverse Proxy Blueprint:**
  - Provide and document an official Nginx reverse-proxy configuration snippet in `manage-services.sh` or a dedicated deployment config file that properly routes `/api/` and `/ws/` with HTTP/1.1 and `Upgrade` headers to the .NET backend.

## Capabilities

### Modified Capabilities
- `desktop-shell`: Add resilient telemetry connectivity with backoff in web mode and production reverse proxy support.
- `hardware-flasher`: Add web-aware hardware flashing constraints and descriptive feedback for local microcontrollers.

## Impact

- **Frontend:** `frontend/src/lib/tauriBridge.js` (WebSocket backoff, flasher error handling), `frontend/src/lib/HardwareDrawer.svelte` (web flasher alerts and validation).
- **Deployment Scripts:** `manage-services.sh` and deployment documentation for Nginx reverse proxy routing.
- **No breaking changes** to Tauri desktop app or serial protocol.
