# Spec Delta: esp32-bluetooth-car

## MODIFIED Requirements

### Requirement: ESP32 Bluetooth Classic Car Firmware and Motor Control
The ESP32 firmware SHALL implement Bluetooth Classic SPP communication using `BluetoothSerial` and control a dual H-bridge motor driver (TB6612FNG) using the assigned GPIO pins and configurable motor direction polarity constants:
- Motor A (Izquierdo): PWMA en GPIO 32 con modulación LEDC a 1 kHz (8 bits, 0-255), AIN1 en GPIO 25 y AIN2 en GPIO 33.
- Motor B (Derecho): PWMB en GPIO 18 con modulación LEDC a 1 kHz (8 bits, 0-255), BIN1 en GPIO 19 y BIN2 en GPIO 27.
- Standby (STBY): GPIO 26 configurado en nivel lógico `HIGH` durante la operación para habilitar el puente H.
- Polarity Configuration: The firmware SHALL define `INVERT_MOTOR_A` and `INVERT_MOTOR_B` constants. When set to `true`, the directional pin states for forward (`speed > 0`) and reverse (`speed < 0`) SHALL be inverted so that physical rotation matches forward vehicle movement without rewiring.

The firmware SHALL parse single-character movement commands from Bluetooth:
- `'F'`: Marcha adelante (ambos motores hacia adelante físicamente con PWM actual).
- `'B'`: Marcha atrás (ambos motores en reversa física con PWM actual).
- `'L'`: Giro a la izquierda (motor izquierdo en reversa o detenido, motor derecho adelante, pivotando a la izquierda).
- `'R'`: Giro a la derecha (motor derecho en reversa o detenido, motor izquierdo adelante, pivotando a la derecha).
- `'G'`: Marcha adelante + izquierda.
- `'I'`: Marcha adelante + derecha.
- `'H'`: Marcha atrás + izquierda.
- `'J'`: Marcha atrás + derecha.
- `'S'`, `'D'`: Parada inmediata (ambos PWM en 0 y salidas de dirección en LOW).
- `'0'` a `'9'`: Ajuste lineal de velocidad de 0% a 100% de ciclo de trabajo PWM mediante la fórmula `(cmd - '0') * 255 / 9`.
- `'q'`, `'Q'`: Respaldo para velocidad máxima (100% PWM / 255).

#### Scenario: Forward motion command received
- **WHEN** the car is connected over Bluetooth Classic and receives the character `'F'`
- **THEN** both motors drive forward according to the configured polarity flags (`INVERT_MOTOR_A` and `INVERT_MOTOR_B`) with active PWM speed and STBY pin 26 remains HIGH

#### Scenario: Differential turn commands received
- **WHEN** the car receives `'L'` (left pivot) or `'R'` (right pivot)
- **THEN** motor outputs drive opposing directions respecting polarity flags such that `'L'` pivots left and `'R'` pivots right physically

#### Scenario: Stop command received
- **WHEN** the car receives the character `'S'` or loses Bluetooth connection
- **THEN** both motor PWM channels are immediately set to 0 duty cycle

#### Scenario: Universal speed command received
- **WHEN** the car receives character `'9'`
- **THEN** current speed is set to 255 (100% PWM duty cycle), and when receiving `'0'` it is set to 0 (0% duty cycle)

## ADDED Requirements

### Requirement: ESP32 DevKit V1 BLE 4.2 Support and Visual Activity Feedback
The system SHALL provide an alternative firmware option for ESP32 DevKit V1 based on Bluetooth Low Energy (BLE 4.2) using the Nordic UART Service (NUS) GATT profile for direct connection from iOS (iPhone/iPad) and Android without system pairing, toggling the onboard LED on GPIO 2 as visual confirmation of received commands.

#### Scenario: BLE connection and control on ESP32 DevKit V1
- **WHEN** a client device connects over BLE 4.2 and sends movement or speed commands
- **THEN** the firmware parses the command, drives the TB6612FNG driver, and pulses the onboard LED on GPIO 2
