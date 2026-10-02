# Tasks: Flasheador y Firmware de Carro Bluetooth ESP32 DevKit V1

## 1. Firmware ESP32 y Configuración PlatformIO

- [x] 1.1 Configurar entorno `[env:esp32_bt_car]` en `firmware/platformio.ini` y crear el firmware en C++/Arduino con `BluetoothSerial.h`, control de puente H dual (PWMA: 32, AIN1: 25, AIN2: 33, PWMB: 18, BIN1: 19, BIN2: 27, STBY: 26) usando modulación ultrasónica LEDC a 20 kHz y el token `##BT_CAR_CUSTOM_NAME_TOKEN##`. Verificar compilación exitosa con `pio run -e esp32_bt_car`.
- [x] 1.2 Exportar los binarios generados (`bootloader.bin`, `partitions.bin`, `boot_app0.bin`, `firmware_esp32_bt.bin`) a `frontend/src-tauri/resources/firmwares/esp32/` y a `release_desktop_v1.0.0/resources/firmwares/esp32/`. Verificar existencia y tamaño de los 4 archivos.

## 2. Backend Rust y Servicio de Flasheo ESP32 (Tauri)

- [x] 2.1 Implementar función en Rust (`flasher.rs`) para buscar y sustituir en memoria el token `##BT_CAR_CUSTOM_NAME_TOKEN##` por el nombre Bluetooth personalizado terminado en `\0` con relleno de ceros hasta 32 bytes, escribiendo el archivo temporal parcheado. Verificar con test unitario en Rust.
- [x] 2.2 Implementar método `flash_esp32` en `FlasherService` invocando `esptool` con los offsets (`0x1000`, `0x8000`, `0xe000`, `0x10000`) y streaming de logs línea por línea vía evento `flash-log`.
- [x] 2.3 Registrar comando Tauri `flash_firmware_esp32(port: String, bt_name: String, baud: u32)` en `lib.rs` y crear la función correspondiente en `frontend/src/lib/tauriBridge.js`. Verificar con `cargo check` en `frontend/src-tauri`.

## 3. Interfaz de Usuario en Hardware Suite (`HardwareDrawer.svelte`)

- [x] 3.1 Agregar la pestaña `🚗 Carro BT ESP32` (`activeTab === 'esp32'`) en la barra de navegación de `HardwareDrawer.svelte` con input de texto para el nombre del Bluetooth, selector de puerto COM y selector de baudrate (460800 / 115200).
- [x] 3.2 Diseñar e integrar la tarjeta visual con el diagrama de conexiones eléctricas y tabla de pines (ESP32 DevKit V1 <-> TB6612: 32, 25, 33, 18, 19, 27, 26, GND, VIN) y la guía de comandos Bluetooth universales (`F`, `B`, `L`, `R`, `S`, etc.).
- [x] 3.3 Conectar el botón "Flashear Carro ESP32" con validación de nombre no vacío, gestión del estado `isFlashing` y visualización de progreso y mensajes de alerta (incluyendo indicación del botón BOOT) en la consola integrada.

## 4. Compilación, Empaquetado y Verificación

- [x] 4.1 Compilar la interfaz web con `pnpm build` y compilar el binario Tauri con `pnpm tauri build --no-bundle`, verificando ausencia de errores.
- [x] 4.2 Actualizar el ejecutable en `release_desktop_v1.0.0/LineFollowerPro.exe` y verificar la ejecución correcta del aplicativo.
- [x] 4.3 Realizar commit de todos los cambios y sincronizar con GitHub `origin main`.
