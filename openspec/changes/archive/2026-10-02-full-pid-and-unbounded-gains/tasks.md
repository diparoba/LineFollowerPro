# Tasks

## 1. Frontend UI Controls and Unbounded Inputs

- [x] 1.1 Update `TuningPanel.svelte` to add labels "Sensibilidad (Kp)" and "Corrección (Kd)", configure `min="0"` and `step="0.0001"` without `max` on numeric inputs, and implement adaptive dynamic sliders
- [x] 1.2 Update `SimulationModal.svelte` to add labels "Sensibilidad (Kp)" and "Corrección (Kd)", set 4-decimal precision (`0.0001`), remove upper limits, and apply adaptive sliders
- [x] 1.3 Update `ProfileManager.svelte` to display gains with 4-decimal precision (`toFixed(4)`) in profile cards
- [x] 1.4 Update `App.svelte` default initial gains and serial EEPROM reading logic to maintain 4 decimals

## 2. Tauri Backend Serial Transmission

- [x] 2.1 Update `SerialService::send_pid` in `frontend/src-tauri/src/serial.rs` to format both `kp` and `kd` with `{:.4}` in the `$PID` string
- [x] 2.2 Verify `cargo check` in `frontend/src-tauri` succeeds with zero errors

## 3. Build, Flashing, and Verification

- [x] 3.1 Build frontend assets with `pnpm run build` and compile Tauri binary with `pnpm tauri build --no-bundle`
- [x] 3.2 Verify serial communication with Arduino Nano on COM8 sending a 4-decimal gain (e.g. `$PID,0.3255,4.7820,...`) and confirming `$OK,PID_UPDATED`
- [x] 3.3 Git commit all changes and push to GitHub `origin main`
