# hardware-flasher Specification

## Purpose
Proporciona utilidades de hardware, flasheo autónomo de firmwares en microcontroladores ATmega328P con auto-fallback de bootloader, esquema visual de conexiones eléctricas y asistente de instalación de drivers USB desde un panel lateral integrado.

## Requirements

### Requirement: Firmware Flashing with Bootloader Auto-Fallback
The system SHALL flash precompiled robot firmwares (`16L Ingeniero Maker` or `8L Codex`) to connected microcontrollers over serial ports using `avrdude` (both from native desktop and web browsers through the local API service), with configurable baud rates (115200 or 9600), automatic fallback to alternate bootloader baud rates upon communication failure, and graceful environment validation when running in remote web browser contexts.

#### Scenario: Successful firmware flash at selected baud rate
- **WHEN** the user selects a target robot model, a COM port, and initiates the flashing process at 115200 baud
- **THEN** the system disconnects active serial telemetry, invokes `avrdude` with the corresponding `.hex` binary, streams terminal progress to the UI, and reports 100% completion upon exit code 0.

#### Scenario: Automatic fallback to alternate bootloader speed
- **WHEN** the flashing process fails at the initial baud rate (e.g. 115200) due to programmer synchronization timeout and auto-fallback is enabled
- **THEN** the system automatically retries the flashing process at the alternate bootloader speed (57600 baud for Old Bootloader or 9600 baud) and informs the user of the successful fallback.

#### Scenario: Flashing Arduino Nano from the web browser
- **WHEN** the user triggers the flash operation from the web interface
- **THEN** the web client communicates with the backend endpoint (`/api/firmware/flash`), receives streaming console logs in real time via WebSocket or server response, and displays progress in the terminal.

#### Scenario: Remote web host flasher restriction warning
- **WHEN** the user attempts to flash an Arduino Nano via `avrdude` while accessing the dashboard from a remote web domain (such as `robotica.ouroboros-software.com`) where local COM ports are physically disconnected from the server
- **THEN** the system intercepts the request or catches HTTP 405/network errors, displaying a clear actionable warning explaining that Arduino Nano `avrdude` flashing requires the native desktop application (`LineFollowerPro.exe`) or running the local companion backend (`http://localhost:5010`), while preserving in-browser Web Serial flashing for ESP32.

### Requirement: Interactive Hardware Pinout and Electrical Mapping
The system SHALL render an interactive visual schematic and reference table of the Arduino Nano microcontroller connections for both the 16-channel multiplexed robot and the 8-channel direct analog robot.

#### Scenario: Switching pinout view between 16L and 8L
- **WHEN** the user toggles between the "16L (Ingeniero Maker)" and "8L (Codex)" models in the pinout tab
- **THEN** the diagram and reference table dynamically highlight the active pins (A0..A5 for multiplexer lines vs A0..A7 for direct analog lines, motor pins 3, 4, 5 and 11, 10, 9, user button D2, and LED D13) with electrical descriptions.

### Requirement: USB Driver Installation Assistant
The system SHALL provide one-click triggers to launch certified driver installers for common USB-to-UART bridge controllers used in robotics competitions.

#### Scenario: Launching CH340 driver installation
- **WHEN** the user clicks "Instalar Driver CH340"
- **THEN** the system launches the bundled or targeted WCH installer (`CH341SER.EXE`) with user confirmation and displays instructions for driver verification.

### Requirement: Side Drawer Interface Integration
The system SHALL display the Hardware, Flasher, and Pinout suite inside a slide-out lateral drawer accessible via a dedicated button in the top navigation bar without disrupting telemetry monitoring.

#### Scenario: Opening and closing the hardware drawer
- **WHEN** the user clicks the "🛠️ Hardware & Flasher" button in the navigation header
- **THEN** the lateral drawer slides into view displaying the three tabs (Flasher, Pinout, Drivers) while keeping underlying telemetry data and controls accessible upon closure.
