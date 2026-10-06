# Tasks: Inversión de Polaridad de Motores en Carro BT ESP32

## 1. Firmware ESP32 y Configuración de Polaridad

- [x] 1.1 Incorporar las constantes `#define INVERT_MOTOR_A true` e `#define INVERT_MOTOR_B true` en `firmware/src/esp32_bt_car.cpp` y actualizar la función `setMotors(speedA, speedB)` para que evalúe la inversión de polaridad garantizando que `speed > 0` produzca avance físico y `speed < 0` retroceso físico.
- [x] 1.2 Compilar el entorno `[env:esp32_bt_car]` con PlatformIO ejecutando `pio run -e esp32_bt_car` y verificar compilación exitosa sin errores.

## 2. Empaquetado y Distribución de Binarios

- [x] 2.1 Copiar el binario recién compilado `.pio/build/esp32_bt_car/firmware.bin` como `firmware_esp32_bt.bin` tanto a `frontend/src-tauri/resources/firmwares/esp32/` como a `release_desktop_v1.0.0/resources/firmwares/esp32/`, verificando la existencia del token `##BT_CAR_CUSTOM_NAME_TOKEN##`.
- [x] 2.2 Ejecutar las pruebas unitarias de Rust en `frontend/src-tauri` (`cargo test`) para verificar que el parcheador `patch_esp32_binary` valida correctamente el checksum y hash sobre la nueva imagen.
- [x] 2.3 Compilar la versión de distribución de escritorio con `pnpm tauri build --no-bundle`, actualizar `release_desktop_v1.0.0/LineFollowerPro.exe` y comprobar la ejecución del aplicativo.
