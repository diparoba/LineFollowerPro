# Spec Delta: esp32-bluetooth-car

## ADDED Requirements

### Requirement: Integrated Virtual Remote Controller for ESP32 Car
The system SHALL provide an integrated Virtual Remote Controller interface (`Esp32RemoteModal`) allowing the user to directly drive and test the ESP32 Car from the desktop application over a selected serial connection (Bluetooth SPP virtual COM port or tethered USB COM port).

The Remote Controller SHALL support the following operation modes and safety features:
- Keyboard Navigation: Driving using standard directional keys (W/A/S/D and Arrow keys) mapping to movement commands (`'F'`, `'B'`, `'L'`, `'R'`, and diagonals `'G'`, `'I'`, `'H'`, `'J'`).
- Dead-Man's Switch Safety: On keyboard release (`keyup`) or on-screen button release (`mouseup`/`touchend`), the system SHALL immediately transmit the stop command (`'S'`) to prevent runaway motion.
- Emergency Brake: Pressing the Spacebar or clicking the emergency stop button SHALL immediately send `'S'`.
- Interactive D-Pad: Virtual on-screen touch and mouse buttons for Forward, Reverse, Left, Right, Diagonals, and Center Stop.
- Dynamic Throttle: Quick-select buttons and speed slider to switch PWM speed levels (`0` through `9`, and `q` for 100%).
- Serial Port Management: An integrated COM port selector, baud rate selector (default 115200), and Connect/Disconnect toggle with live connection indicator and transmission monitor.

#### Scenario: Driving forward using keyboard
- **WHEN** the user holds down the 'W' or 'ArrowUp' key while the controller modal is active and connected
- **THEN** the system transmits the character `'F'` over the active serial port and highlights the Forward indicator

#### Scenario: Safety stop upon key release
- **WHEN** the user releases any active movement key
- **THEN** the system immediately transmits the character `'S'` over the serial port and clears the active movement indicators

#### Scenario: Diagonal steering combination
- **WHEN** the user presses 'W' and 'A' simultaneously
- **THEN** the system transmits the diagonal command `'G'` (Forward + Left)

#### Scenario: Throttle speed change
- **WHEN** the user selects a speed preset or adjusts the throttle slider to 50%
- **THEN** the system transmits the corresponding speed character `'4'` to the microcontroller

#### Scenario: Emergency brake triggered
- **WHEN** the user presses the Spacebar or clicks the Center Stop button
- **THEN** the system immediately sends `'S'`, cancels any active direction state, and logs the emergency stop in the telemetry monitor
