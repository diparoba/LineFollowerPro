# Tasks

## 1. Normalización de Escala de Velocidad (0 a 9)

- [x] 1.1 Actualizar la fórmula de velocidad en `firmware/src/esp32_bt_car.cpp` utilizando `(cmd - '0') * 255 / 9` y verificar que `'0'` mapea a 0 PWM y `'9'` a 255 PWM.
- [x] 1.2 Actualizar la fórmula de velocidad en `firmware/src/esp32_s3_ble_car.cpp` utilizando `(cmd - '0') * 255 / 9` y verificar respuesta visual en NeoPixel.
- [x] 1.3 Actualizar la tabla de referencia de comandos en `frontend/src/lib/HardwareDrawer.svelte` indicando que `'0'` es 0% y `'9'` es 100%.

## 2. Firmware BLE 4.2 para ESP32 DevKit V1

- [x] 2.1 Crear `firmware/src/esp32_ble_car.cpp` implementando Nordic UART Service (NUS), pines originales TB6612FNG y retroalimentación en GPIO 2 (`LED_BUILTIN`).
- [x] 2.2 Agregar el entorno `[env:esp32_ble_car]` en `firmware/platformio.ini` y compilar con éxito mediante PlatformIO.
- [x] 2.3 Distribuir los binarios resultantes (`bootloader.bin`, `partitions.bin`, `boot_app0.bin`, `firmware_esp32_ble.bin`) en `frontend/public/firmwares/esp32_ble/` y `backend/wwwroot/firmwares/esp32_ble/`.

## 3. Servicio de Flasheo Web para Seguidores de Línea (Arduino Nano)

- [x] 3.1 Garantizar disponibilidad de `avrdude.exe` y `avrdude.conf` en el backend (`backend/resources/avrdude`).
- [x] 3.2 Implementar el endpoint `POST /api/firmware/flash` en `backend/Program.cs` con desconexión preventiva de `SerialService`, ejecución de `avrdude` y auto-fallback a 57600 baudios.
- [x] 3.3 Actualizar `frontend/src/lib/tauriBridge.js` para que `flashFirmware()` invoque el endpoint `/api/firmware/flash` en la versión web en lugar de lanzar una excepción.

## 4. Integración y Verificación en la Plataforma Web

- [x] 4.1 Actualizar `frontend/src/lib/HardwareDrawer.svelte` y `frontend/src/lib/esp32WebFlasher.js` para soportar la opción "ESP32 Clásico DevKit V1 (BLE 4.2)".
- [x] 4.2 Ejecutar `pnpm run build` en `frontend/` y sincronizar los archivos estáticos en `backend/wwwroot/`.
- [x] 4.3 Probar la subida de firmware de seguidores desde el navegador y verificar que el terminal reciba los registros en tiempo real.
