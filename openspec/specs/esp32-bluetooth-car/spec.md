# esp32-bluetooth-car Specification

## Purpose
Proporciona la suite de firmware y flasheo para microcontroladores ESP32 DevKit V1 orientada a carros robóticos controlados mediante Bluetooth Classic SPP, incorporando inyección dinámica de nombre de dispositivo antes de la subida, control de puente H dual (TB6612FNG) y una interfaz gráfica dedicada con referencia visual de cableado.

## Requirements

### Requirement: ESP32 Bluetooth Classic Car Firmware and Motor Control
The ESP32 firmware SHALL implement Bluetooth Classic SPP communication using `BluetoothSerial` and control a dual H-bridge motor driver (TB6612FNG) using the assigned GPIO pins:
- Motor A (Izquierdo): PWMA en GPIO 32 con modulación LEDC a 20 kHz ultrasónico (8 bits, 0-255), AIN1 en GPIO 25 y AIN2 en GPIO 33.
- Motor B (Derecho): PWMB en GPIO 18 con modulación LEDC a 20 kHz ultrasónico (8 bits, 0-255), BIN1 en GPIO 19 y BIN2 en GPIO 27.
- Standby (STBY): GPIO 26 configurado en nivel lógico `HIGH` durante la operación para habilitar el puente H.

The firmware SHALL parse single-character movement commands from Bluetooth:
- `'F'`: Marcha adelante (ambos motores hacia adelante con PWM actual).
- `'B'`: Marcha atrás (ambos motores en reversa con PWM actual).
- `'L'`: Giro a la izquierda (motor izquierdo en reversa o detenido, motor derecho adelante).
- `'R'`: Giro a la derecha (motor derecho en reversa o detenido, motor izquierdo adelante).
- `'G'`: Marcha adelante + izquierda.
- `'I'`: Marcha adelante + derecha.
- `'H'`: Marcha atrás + izquierda.
- `'J'`: Marcha atrás + derecha.
- `'S'`: Parada inmediata (ambos PWM en 0 y salidas de dirección en LOW).
- `'0'` a `'9'`, `'q'`: Ajuste de nivel de velocidad de 0% a 100% de ciclo de trabajo PWM.

#### Scenario: Forward motion command received
- **WHEN** the car is connected over Bluetooth Classic and receives the character `'F'`
- **THEN** both motors drive forward with the current active PWM speed and STBY pin 26 remains HIGH

#### Scenario: Stop command received
- **WHEN** the car receives the character `'S'` or loses Bluetooth connection
- **THEN** both motor PWM channels are immediately set to 0 duty cycle

### Requirement: Dynamic Bluetooth Device Name Configuration
The system SHALL provide an input in the user interface to specify the Bluetooth broadcast name prior to flashing. The name MUST be between 1 and 31 ASCII characters. The backend SHALL replace the placeholder signature inside the precompiled firmware template binary with the sanitized custom name (null-terminated and zero-padded) before flashing to the target ESP32.

#### Scenario: Custom Bluetooth name specified
- **WHEN** the user inputs "Carro_Rayo_01" and clicks flash
- **THEN** the system generates a customized firmware binary containing "Carro_Rayo_01\0" at the device name location and proceeds to flash

#### Scenario: Empty Bluetooth name validation
- **WHEN** the user attempts to flash with an empty or whitespace-only name
- **THEN** the UI blocks flashing and notifies the user to enter a valid Bluetooth name

### Requirement: Autonomous ESP32 Flashing via UI
The system SHALL flash the ESP32 DevKit V1 microcontroller over the selected USB COM port using `esptool`. The system SHALL flash the bootloader at `0x1000`, partition table at `0x8000`, boot_app0 at `0xe000`, and the customized application binary at `0x10000` at a configurable baud rate (default 460800 baud with fallback to 115200 baud). Real-time progress and output lines MUST be streamed to the UI terminal.

#### Scenario: Successful ESP32 flash
- **WHEN** the user selects the ESP32 COM port and clicks "Flashear Carro ESP32"
- **THEN** `esptool` writes the four binary components, reports completion progress in the UI log, and displays a success notification upon completion

#### Scenario: Flashing failure on serial port
- **WHEN** the ESP32 fails to enter bootloader mode or serial communication fails
- **THEN** the system displays a clear error message in the UI log indicating the failure reason and suggesting to hold the BOOT/IO0 button if required

### Requirement: Dedicated ESP32 Car Tab and Visual Wiring Reference
The system SHALL display a dedicated `🚗 Carro BT ESP32` tab inside the `HardwareDrawer` component. This section SHALL include:
- An input for the custom Bluetooth broadcast name.
- A COM port selector with refresh capability.
- A baud rate selector (460800 / 115200).
- An interactive pinout card and wiring table displaying ESP32 DevKit V1 connections (GPIO 32, 25, 33, 18, 19, 27, 26, GND, VIN).
- A quick reference cheatsheet of supported Bluetooth control commands and compatible mobile apps.
- Action button to trigger flashing with live terminal output.

#### Scenario: Accessing the ESP32 Car Tab
- **WHEN** the user opens the Hardware Suite and clicks on the "Carro BT ESP32" tab
- **THEN** the dedicated ESP32 configuration form, wiring schematic, and flasher console are displayed
