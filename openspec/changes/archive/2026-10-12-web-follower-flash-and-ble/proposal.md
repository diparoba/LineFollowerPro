# Proposal: Web Follower Flashing, ESP32 DevKit V1 BLE, and Universal Speed Scaling

## Why

1. Al intentar subir el firmware de los robots seguidores de línea (Arduino Nano ATmega328P `IM_16` / `CODEX_8`) desde la interfaz web, el sistema arroja el error: `Error: El flasheo de firmware solo está disponible en la versión de escritorio nativa`. Esto impide a los usuarios de la versión web programar sus seguidores de línea.
2. Los usuarios con dispositivos Apple (iPhone / iPad) no pueden controlar el carro ESP32 DevKit V1 mediante Bluetooth Classic debido a restricciones propietarias de iOS (requerimiento de autenticación MFi). El ESP32 DevKit V1 posee hardware de radio dual compatible con BLE 4.2, lo que permite ofrecer conectividad BLE universal sin requerir emparejamiento manual.
3. La escala de velocidad previa utilizaba el carácter `'q'` para el 100% y `'9'` para el ~94%, mientras que los mandos y aplicaciones de control robótico estándar operan con dígitos de `'0'` (0%) a `'9'` (100%).

## What Changes

- **Flasheo de Seguidores de Línea desde la Web:**
  - Agregar un endpoint en el backend local (`POST /api/firmware/flash`) que ejecute `avrdude.exe` con detección automática de bootloader (115200 con auto-fallback a 57600 baudios) y transmita los registros en tiempo real por WebSocket.
  - Actualizar `tauriBridge.js` para que en modo Web invoque el endpoint del backend local en lugar de lanzar una excepción restrictiva.
- **Opción BLE 4.2 para ESP32 DevKit V1 Clásico:**
  - Crear el firmware `esp32_ble_car.cpp` compilado para `esp32doit-devkit-v1` utilizando Nordic UART Service (NUS).
  - Emplear el LED integrado en GPIO 2 (`LED_BUILTIN`) para proporcionar pulsos de confirmación visual al recibir comandos BLE.
  - Incorporar la opción "ESP32 Clásico DevKit V1 (BLE 4.2 - Compatible con iOS)" en el selector de hardware de la interfaz web.
- **Normalización de Escala de Velocidad (0 = 0% a 9 = 100%):**
  - Actualizar la fórmula de velocidad a `(cmd - '0') * 255 / 9` en todos los firmwares de carros (`esp32_bt_car.cpp`, `esp32_ble_car.cpp`, `esp32_s3_ble_car.cpp`).
  - Mantener `'q'` y `'Q'` como sinónimos de 255 (100%) para retrocompatibilidad.
  - Actualizar la documentación y tabla de comandos en `HardwareDrawer.svelte`.

## Capabilities

### Modified Capabilities
- `hardware-flasher`: Añade soporte para flasheo de firmwares ATmega328P desde la aplicación web comunicándose con el servicio local de `avrdude`.
- `esp32-bluetooth-car`: Incorpora soporte para perfil BLE 4.2 en ESP32 DevKit V1 (con retroalimentación en GPIO 2) y normaliza la escala de acelerador de `'0'` (0%) a `'9'` (100% PWM).

## Impact

- **Backend:** `backend/Program.cs` expone el endpoint `POST /api/firmware/flash` e integra `avrdude`.
- **Frontend:** `frontend/src/lib/tauriBridge.js` enruta el flasheo de seguidores al backend; `HardwareDrawer.svelte` agrega la opción BLE 4.2 para DevKit V1 y actualiza la guía de comandos.
- **Firmware:** Nuevo archivo `firmware/src/esp32_ble_car.cpp` y actualización de `firmware/src/esp32_bt_car.cpp` y `firmware/src/esp32_s3_ble_car.cpp`.
- **PlatformIO:** Configuración del nuevo entorno `[env:esp32_ble_car]` en `firmware/platformio.ini`.
