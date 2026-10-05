# Proposal: Inversión de Polaridad de Motores en Firmware Carro BT ESP32

## Why

Al probar el carro RC con ESP32 DevKit V1 y driver TB6612FNG, la orientación del cableado y reductores de ambos motores provoca que el avance y retroceso estén físicamente invertidos. Como consecuencia del esquema de tracción diferencial, el comando de avance `'F'` mueve el carro hacia atrás, y los comandos de giro `'L'` (izquierda) y `'R'` (derecha) hacen que pivote en el sentido contrario al deseado. Se requiere alternar la polaridad lógica de ambos motores mediante banderas configurables en el firmware sin requerir alteraciones al cableado físico.

## What Changes

- Incorporación de constantes de configuración de polaridad `#define INVERT_MOTOR_A true` e `#define INVERT_MOTOR_B true` en `firmware/src/esp32_bt_car.cpp`.
- Corrección de la función `setMotors(speedA, speedB)` para que evalúe las banderas de inversión y aplique los estados adecuados en `AIN1`/`AIN2` y `BIN1`/`BIN2`, asegurando que `speed > 0` produzca avance físico y `speed < 0` retroceso físico.
- Recompilación del entorno `[env:esp32_bt_car]` mediante PlatformIO para generar la imagen de firmware corregida `firmware_esp32_bt.bin`.
- Actualización de los binarios empaquetados en `frontend/src-tauri/resources/firmwares/esp32/` y en `release_desktop_v1.0.0/resources/firmwares/esp32/`.
- Recompilación y actualización del ejecutable de la suite de escritorio `LineFollowerPro.exe`.

## Capabilities

### Modified Capabilities
- `esp32-bluetooth-car`: Actualización del comportamiento de dirección de giro y polaridad en el control de motores para asegurar que los comandos `'F'`, `'B'`, `'L'`, `'R'`, `'G'`, `'I'`, `'H'`, `'J'` respondan con la cinemática física correcta sin modificar el cableado.

## Impact

- Código afectado: `firmware/src/esp32_bt_car.cpp`.
- Binarios afectados: `firmware_esp32_bt.bin` en `frontend/src-tauri/resources/firmwares/esp32/` y `release_desktop_v1.0.0/resources/firmwares/esp32/`.
- Aplicación de escritorio: Actualización del artefacto compilado final `LineFollowerPro.exe`.
- Compatibilidad: Mantiene intactos el protocolo de comandos Bluetooth, la inyección dinámica de nombre en tiempo de flasheo y la asignación de pines GPIO.
