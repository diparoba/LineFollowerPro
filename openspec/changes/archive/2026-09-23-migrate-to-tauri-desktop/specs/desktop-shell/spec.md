# Spec Delta: Desktop Shell

## Purpose

Proporciona un contenedor de escritorio ligero, seguro y autónomo basado en Tauri (Rust + WebView2) para la suite Seguidor de Línea Pro, permitiendo telemetría en tiempo real a 60 FPS, control serial asíncrono y persistencia local de perfiles sin depender de navegadores externos ni servidores HTTP pesados.

## ADDED Requirements

### Requirement: Native Desktop Window Lifecycle
The system SHALL launch as a self-contained native Windows desktop application with an embedded WebView2 runtime and handle window events without requiring external web browsers or local TCP network port bindings.

#### Scenario: Application startup
- **WHEN** user launches the desktop executable `line-follower-pro.exe`
- **THEN** system opens a dedicated desktop window titled "LineFollower Pro - Dashboard" with minimum dimensions 1100x700 and loads the Svelte dashboard instantly.

#### Scenario: Application clean exit
- **WHEN** user closes the desktop window
- **THEN** system terminates all serial read threads and background workers cleanly without leaving orphan background processes.

---

### Requirement: Low-Latency Serial Port Communication
The system SHALL provide asynchronous serial port enumeration, connection at 115200 baud (or configurable baud rate), bidirectional packet transmission, and event-driven telemetry streaming.

#### Scenario: COM port listing
- **WHEN** user requests available communication ports
- **THEN** system returns an array of currently available system serial port names (e.g., `["COM3", "COM4"]`).

#### Scenario: Connecting to Arduino Nano
- **WHEN** user selects a valid serial port and clicks connect
- **THEN** system opens the port with 8-N-1 configuration, DTR/RTS reset pulse, starts background stream reader, and confirms connection status to the interface.

#### Scenario: Streaming telemetry to UI
- **WHEN** valid `$TEL` packets arrive from the microcontroller
- **THEN** system parses raw sensor channels, center-of-mass position, error, and motor PWMs, emitting a `telemetry` event to the Svelte frontend at up to 60 FPS.

---

### Requirement: Local SQLite Profile Storage
The system SHALL maintain a local SQLite database (`follower.db`) preserving profile records for both 16-channel (`IM_16`) and 8-channel (`CODEX_8`) robots with fleet management by car name.

#### Scenario: Reading profiles by category and car
- **WHEN** user filters profiles by category (e.g. `CODEX_8`) and car name (e.g. `Carro Codex-1`)
- **THEN** system queries `follower.db` and returns matching tuning profiles ordered by most recent.

#### Scenario: Saving tuning profile
- **WHEN** user saves current PID and dynamics parameters under a profile name
- **THEN** system persists or updates the record in SQLite and returns the saved entity with timestamp.

---

### Requirement: Two-Way EEPROM Synchronization and Verification
The system SHALL send EEPROM save/read commands to the robot and wait for physical microcontroller verification responses (`$EEPROM_OK` / `$EEPROM_DATA`) with timeout handling.

#### Scenario: Saving parameters to EEPROM
- **WHEN** user clicks "Guardar en EEPROM"
- **THEN** system transmits `$EEPROM,SAVE` over serial and waits up to 2500 ms for `$EEPROM_OK`, displaying a verification confirmation in the UI.

#### Scenario: Reading parameters from EEPROM
- **WHEN** user clicks "Leer EEPROM"
- **THEN** system transmits `$EEPROM,READ` over serial and updates UI sliders and values from the returned `$EEPROM_DATA` parameters.
