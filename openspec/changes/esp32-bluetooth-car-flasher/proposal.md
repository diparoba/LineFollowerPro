# Proposal: Flasheador y Firmware de Carro Controlado por Bluetooth Classic para ESP32 DevKit V1

## Why

Actualmente la suite de hardware y flasheo está orientada a microcontroladores ATmega328P (Arduino Nano) para seguidores de línea (IM-16 y CODEX-8). Sin embargo, en robótica y docencia es muy común disponer de plataformas móviles basadas en ESP32 DevKit V1 controladas de forma inalámbrica mediante Bluetooth Classic desde un teléfono móvil. 

Existe la necesidad de incorporar un apartado dedicado en la aplicación que permita subir un firmware de carro RC a un ESP32 DevKit V1 conectado por USB, permitiendo personalizar el nombre del dispositivo Bluetooth directamente desde la interfaz gráfica antes de flashear, configurando un puente H con pines específicos (PWMA en GPIO 32, pines de control en 25 y 33; PWMB en GPIO 18, pines de control en 19 y 27; y STBY en GPIO 26).

## What Changes

- **Nuevo Firmware para ESP32 DevKit V1 (`firmware/src_esp32` o PlatformIO env):**
  - Implementación con `BluetoothSerial` (Bluetooth Classic SPP).
  - Manejo de puente H dual (ej. TB6612FNG):
    - Motor A (Izq): PWMA en GPIO 32 (PWM LEDC a 20 kHz ultrasónico), AIN1 en GPIO 25, AIN2 en GPIO 33.
    - Motor B (Der): PWMB en GPIO 18 (PWM LEDC a 20 kHz ultrasónico), BIN1 en GPIO 19, BIN2 en GPIO 27.
    - Standby (STBY): GPIO 26 activado en `HIGH` para habilitar el puente H.
  - Soporte de protocolo de control universal RC Bluetooth (`F`, `B`, `L`, `R`, `G`, `I`, `H`, `J`, `S`, y velocidades `0`-`9`, `q`).
- **Inyección Dinámica de Nombre Bluetooth:**
  - El usuario escribe en la interfaz el nombre deseado para el Bluetooth antes de flashear (ej. `Carro_Pro_01`).
  - La aplicación inyecta el nombre en el binario template antes de transferirlo al ESP32.
- **Motor de Flasheo ESP32 en Tauri (`FlasherService`):**
  - Integración de `esptool` para flasheo de ESP32 (`--chip esp32 write_flash 0x1000 bootloader.bin 0x8000 partitions.bin 0xe000 boot_app0.bin 0x10000 firmware.bin`).
  - Streaming de progreso y logs en tiempo real hacia la interfaz web.
- **Nueva Pestaña / Sección en `HardwareDrawer.svelte`:**
  - Pestaña dedicada **🚗 Carro BT ESP32** con campo para el nombre del Bluetooth, selección de puerto COM, velocidad de subida, botón de flasheo y terminal de logs.
  - Tarjeta interactiva con el diagrama de conexiones eléctricas y tabla de pines (GPIOs del ESP32 conectados al puente H).
  - Guía rápida de comandos Bluetooth y aplicaciones móviles compatibles (ej. *Arduino Bluetooth RC Car*).

## Capabilities

### New Capabilities
- `esp32-bluetooth-car`: Proporciona la suite completa de firmware para ESP32 DevKit V1, control de motor con puente H (PWMA 32, AIN1 25, AIN2 33, PWMB 18, BIN1 19, BIN2 27, STBY 26), comunicación Bluetooth Classic SPP, inyección dinámica de nombre de dispositivo y flasheo autónomo con `esptool` desde la interfaz gráfica.

### Modified Capabilities
<!-- No se modifican los requisitos de capabilities existentes; las funciones del seguidor Nano IM-16 y Codex-8 se mantienen intactas. -->

## Impact

- **Frontend (`frontend/src/lib/HardwareDrawer.svelte`):** Nueva pestaña y componentes para configurar nombre BT, flasheo y visualización de conexiones.
- **Backend Tauri (`frontend/src-tauri`):** Nuevos comandos Tauri para invocar flasheo de ESP32 con nombre personalizado vía `esptool`.
- **Recursos (`frontend/src-tauri/resources` y `release_desktop_v1.0.0`):** Inclusión de binarios de ESP32 (`bootloader.bin`, `partitions.bin`, `boot_app0.bin`, `firmware_esp32_bt.bin`) y utilitario `esptool`.
- **Firmware (`firmware/`):** Código fuente C++/Arduino del firmware ESP32 y configuración en `platformio.ini`.
