# Tasks: Modo Piso Reflectante, Recuperación Robusta de Bifurcaciones y Permisos Web Serial

## 1. Firmware Arduino (Modo Reflectante, Bifurcación y Protocolo)

- [x] 1.1 In `firmware/include/config.h`, add `uint8_t reflective_mode;` to `RobotConfig`, update `EEPROM_MAGIC` to `0x4C47` ("LG"), and verify struct alignment
- [x] 1.2 In `firmware/include/sensors.h`, implement noise gate ($\le 250$), ceiling ($\ge 760$), dynamic scaling, and spatial window ($\pm 3$ / $\pm 4$ channels) for `reflective_mode`
- [x] 1.3 In `firmware/include/sensors.h`, implement sustained 160 ms fork detection (`bifurcation_active_until = now_ms + 160`), multi-segment line detection (`segments >= 2`), and edge tracking (`first_active` / `last_active`)
- [x] 1.4 In `firmware/include/protocol.h`, update `$PID` parser to support optional 8th parameter, add `$CMD,REFLECT_ON` / `$CMD,REFLECT_OFF`, support `line_color >= 2` fallback, and update EEPROM serialization
- [x] 1.5 In `firmware/src/main.cpp`, update `setup()` default config and pass `config.reflective_mode != 0` into `sensors.getPosition(...)` in `loop()`

## 2. Responsive Header Layout & Web Serial Permissions

- [x] 2.1 In `frontend/src/App.svelte`, apply `flex-shrink: 0; min-height: max-content; overflow: visible;` to `.workspace-header` to prevent vertical collapsing on laptops and compact viewports
- [x] 2.2 In `frontend/src/lib/ControlBar.svelte`, apply `flex-shrink: 0; min-height: max-content; overflow: visible;` and responsive wrapping for `.serial-section` and `.robot-actions`
- [x] 2.3 In `frontend/src/lib/ControlBar.svelte`, add interactive `🔑 Conceder Permiso USB` button when `!isTauri()` that invokes `requestWebSerialPort()` to prompt the browser dialog for Arduino and ESP32

## 3. Tuning Panel & Backend / SQLite Persistence

- [x] 3.1 In `frontend/src/lib/TuningPanel.svelte`, add surface mode selector ("Piso Mate / Normal" vs "Piso Reflectante") and dispatch `reflectiveMode` with PID events
- [x] 3.2 In `frontend/src/lib/tauriBridge.js`, forward `reflectiveMode` in `sendPid` for both Tauri invoke and Web API fetch
- [x] 3.3 In `frontend/src-tauri/src/models.rs` and `frontend/src-tauri/src/serial.rs`, add `reflective_mode` to `RobotProfile` and format `$PID` command string with 8 parameters
- [x] 3.4 In `frontend/src-tauri/src/db.rs`, implement automatic SQLite schema migration (`ALTER TABLE profiles ADD COLUMN reflective_mode INTEGER NOT NULL DEFAULT 0;`) and update CRUD queries

## 4. Build, Verification & Web Deployment Sync

- [x] 4.1 Run frontend build (`npm run build` in `frontend/`) and verify 0 compilation errors
- [x] 4.2 Run `cargo check` in `frontend/src-tauri` and verify 0 compiler/linker errors
- [x] 4.3 Sync production build assets to `backend/wwwroot/` so web deployment serves the responsive header fix and Web Serial authorization button
