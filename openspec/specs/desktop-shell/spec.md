# desktop-shell Specification

## Purpose
Proporciona un contenedor de escritorio ligero, seguro y autónomo basado en Tauri (Rust + WebView2) para la suite Seguidor de Línea Pro, permitiendo telemetría en tiempo real a 60 FPS, control serial asíncrono y persistencia local de perfiles sin depender de navegadores externos ni servidores HTTP pesados.

## Requirements

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

### Requirement: Fluid Full-Width Dashboard with 3-Column Ultrawide Layout
The system SHALL display a fully responsive, 100% fluid desktop interface without fixed maximum width constraints, adapting its dashboard layout to the available display dimensions using an ergonomic 3-column grid on wide monitors.

#### Scenario: Rendering on wide and ultrawide displays
- **WHEN** the application window width is $\ge 1500\text{px}$ (such as 2K, 4K, or 21:9 Ultrawide monitors)
- **THEN** system arranges the dashboard into 3 distinct columns: Column 1 for Telemetry (Sensors and Motors), Column 2 for Tuning (PD gains and speeds), and Column 3 for Fleet & Profiles (SQLite management).

#### Scenario: Rendering on laptop and standard displays
- **WHEN** the application window width is between $1040\text{px}$ and $1499\text{px}$
- **THEN** system smoothly reorganizes into 2 columns with Sensors/Motors on the left and Tuning/Profiles on the right.

#### Scenario: Rendering on compact displays
- **WHEN** the application window width is $< 1040\text{px}$
- **THEN** system stacks all sections into a single scrollable vertical column without visual overlap or clipping.

---

### Requirement: Ergonomic Left Navigation Sidebar
The system SHALL provide a dedicated left vertical navigation sidebar hosting suite-wide tools (Dashboard, Auto-Tuning, Caja Negra, Hardware Flasher, Simulador), theme toggling, and live connection status, freeing top-level horizontal space for serial connectivity and robot controls.

#### Scenario: Navigating tools via left sidebar
- **WHEN** user clicks on any tool button in the left sidebar (such as Auto-Tuning, Caja Negra, or Hardware Flasher)
- **THEN** system activates the corresponding view or modal seamlessly while maintaining live background serial streaming

#### Scenario: Toggling theme and viewing connection status in sidebar
- **WHEN** user views the lower sidebar section
- **THEN** system displays the current theme toggle and real-time serial link status (ONLINE/OFFLINE) with visual indicator
