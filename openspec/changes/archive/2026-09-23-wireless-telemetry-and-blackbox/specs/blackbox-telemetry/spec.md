# Spec Delta

## Purpose

Define el protocolo de telemetría bajo demanda con arranque silencioso para optimizar el rendimiento del procesador y el módulo Caja Negra con auto-grabación de carreras, historial de 5 vueltas y exportación a CSV.

## ADDED Requirements

### Requirement: Demand-Driven Telemetry Protocol (Zero-Print Startup)
The microcontroller firmware SHALL boot with telemetry transmissions disabled (`telemetry_active = false`) and SHALL NOT emit periodic serial telemetry prints until an explicit `$CMD,STREAM_ON` command is received from the host desktop application over serial or Bluetooth.

#### Scenario: Silent startup on battery power
- **WHEN** the robot powers on via battery without an active desktop application connection
- **THEN** the microcontroller loop executes at 1 kHz without emitting any `$TEL` serial packets or UART transmission interrupts.

#### Scenario: Enabling telemetry on connection
- **WHEN** the desktop application establishes a serial connection (USB or Bluetooth COM) and transmits `$CMD,STREAM_ON`
- **THEN** the microcontroller enables `telemetry_active` and begins emitting `$TEL` packets at the configured sampling frequency.

#### Scenario: Disabling telemetry on disconnection
- **WHEN** the desktop application disconnects or sends `$CMD,STREAM_OFF`
- **THEN** the microcontroller immediately resets `telemetry_active = false` and ceases all serial telemetry output.

### Requirement: Automatic Race Lap Recording (Black Box)
The desktop application SHALL automatically record high-resolution telemetry samples into memory when the robot enters the active running state (`STATE_RUNNING`) and finalize the lap when the state returns to `STATE_READY` or stopped.

#### Scenario: Auto-start recording when robot launches
- **WHEN** telemetry is streaming and the robot state transitions from `STATE_READY` to `STATE_RUNNING`
- **THEN** the Black Box module automatically initializes a new recording session, capturing timestamped error, position, motor PWM, and raw sensor readings.

#### Scenario: Auto-stop recording on lap finish
- **WHEN** the robot transitions from `STATE_RUNNING` to `STATE_READY` (braked or stopped)
- **THEN** the Black Box module terminates the recording, computes the exact lap duration, root-mean-square error (RMSE), and stores the lap into the history buffer.

### Requirement: Five-Lap History and Comparative Analysis
The desktop application SHALL maintain a memory buffer of the last five completed race laps per session and render an interactive multi-lap comparison view.

#### Scenario: Viewing and comparing recent laps
- **WHEN** the user opens the Black Box modal and selects two laps from the five-lap history
- **THEN** the system overlays the error trajectory curves on an interactive time-series chart and displays delta comparisons for lap time and RMSE.

### Requirement: Telemetry Session Export to CSV
The desktop application SHALL allow users to export individual laps or entire recorded sessions to formatted Comma-Separated Values (CSV) files.

#### Scenario: Exporting a recorded lap to CSV
- **WHEN** the user clicks "Exportar a CSV" on a recorded lap
- **THEN** the system prompts for a save location and generates a CSV file with columns: `Timestamp_ms, Error, Position, LeftMotor, RightMotor, State, RawSensors...`.
