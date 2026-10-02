# Design: Suite de Flasheo y Firmware para Carro Bluetooth ESP32 DevKit V1

## Context

La aplicación de escritorio LineFollowerPro actualmente cuenta con un módulo de flasheo (`FlasherService`) en Rust/Tauri que utiliza `avrdude` para microcontroladores AVR (ATmega328P / Arduino Nano). La interfaz de usuario en `HardwareDrawer.svelte` provee pestañas para flashear, ver pinouts de Nano e instalar drivers.

Para soportar carros robóticos basados en ESP32 DevKit V1 controlados por Bluetooth Classic, se requiere una cadena de flasheo para arquitectura Xtensa/ESP32 (`esptool`), un firmware específico para control de puente H (TB6612FNG) y un mecanismo que permita personalizar el nombre del dispositivo Bluetooth directamente desde la interfaz de usuario antes de la subida.

## Goals / Non-Goals

**Goals:**
- Implementar firmware en C++/Arduino para ESP32 DevKit V1 con `BluetoothSerial.h` y control de puente H en los pines indicados (PWMA: 32, AIN1: 25, AIN2: 33, PWMB: 18, BIN1: 19, BIN2: 27, STBY: 26).
- Utilizar el periférico de hardware LEDC del ESP32 a 20 kHz ultrasónico para eliminar el ruido audible en motores.
- Permitir al usuario ingresar el nombre Bluetooth en la interfaz antes de flashear, inyectándolo en el binario sin requerir compilación externa en tiempo de ejecución.
- Integrar `esptool` en `FlasherService` para flashear bootloader, partitions, boot_app0 y firmware con streaming de logs hacia el frontend.
- Crear una pestaña dedicada `🚗 Carro BT ESP32` en `HardwareDrawer.svelte` con formulario, consola de logs y diagrama de conexiones visual.

**Non-Goals:**
- No se implementa Bluetooth Low Energy (BLE) ni servicios GATT; se utiliza estrictamente Bluetooth Classic SPP para compatibilidad con mandos y aplicaciones universales.
- No se implementa control PID de lazo cerrado con encoders en este modo; opera como carro radiocontrolado por comandos directos de velocidad y dirección.
- No se requiere conexión a internet ni dependencias externas de compilación durante el uso por parte del usuario final.

## Decisions

### 1. Inyección de Nombre Bluetooth mediante Parcheo de Binario (Template)
- **Decisión:** El firmware del ESP32 se compila con un búfer de 32 bytes con una firma única conocida:
  ```cpp
  const char BT_NAME_PLACEHOLDER[32] = "##BT_CAR_CUSTOM_NAME_TOKEN##\0";
  ```
  Al solicitar el flasheo desde la interfaz web con el nombre deseado (ej. `Carro_Pro_01`), el backend en Rust lee `firmware_esp32_bt.bin`, localiza la subsecuencia de bytes del token y la sustituye en memoria por el nombre ingresado terminado en `\0` y rellenado con ceros hasta completar los 32 bytes. Luego guarda el archivo temporal y procede al flasheo.
- **Alternativas consideradas:**
  - *Compilación al vuelo con PlatformIO CLI:* Descartada porque tarda más de 30 segundos en cada flasheo y obliga al usuario a tener instalado Python, Git y toda la cadena de herramientas de Espressif en su máquina.
  - *Configuración por Serial en tiempo de ejecución:* Descartada porque el usuario solicitó explícitamente definir el nombre en la interfaz *antes* de subir el programa.
- **Ventaja:** Flasheo ultra-rápido (< 5 segundos), 100% portable y sin requerir compiladores en la máquina destino.

### 2. Cadena de Flasheo con `esptool`
- **Decisión:** Integrar en `FlasherService::flash_esp32` la ejecución de `esptool`.
  Parámetros de comando:
  ```text
  esptool --chip esp32 --port <PORT> --baud <BAUD> --before default_reset --after hard_reset write_flash -z --flash_mode dio --flash_freq 40m --flash_size detect 0x1000 bootloader.bin 0x8000 partitions.bin 0xe000 boot_app0.bin 0x10000 firmware_esp32_bt.bin
  ```
  Se implementará resolución en cascada: primero buscará `esptool.exe` standalone embebido en `resources/esptool/`, y si no existe, buscará `python` con `tool-esptoolpy/esptool.py`.
- **Streaming:** La salida estándar y de error de `esptool` se capturará línea a línea y se emitirá mediante el evento Tauri `flash-log`, mostrando el porcentaje de escritura de bloques en la consola de la UI.

### 3. Asignación de Pines y Generación de PWM (LEDC)
- **Decisión:**
  - Motor A: GPIO 32 asignado al canal LEDC 0 (frecuencia 20,000 Hz, resolución 8 bits, ciclo 0-255). Direcciones AIN1 en GPIO 25 y AIN2 en GPIO 33.
  - Motor B: GPIO 18 asignado al canal LEDC 1 (frecuencia 20,000 Hz, resolución 8 bits, ciclo 0-255). Direcciones BIN1 en GPIO 19 y BIN2 en GPIO 27.
  - Standby (STBY): GPIO 26 inicializado en `OUTPUT` y llevado a nivel `HIGH` en el `setup()`.
- **Giro:**
  - Giro pivote en `'L'` y `'R'` (un motor hacia adelante y el opuesto en reversa) para maniobrabilidad ágil de 360°.
  - Movimientos combinados `'G'`, `'I'`, `'H'`, `'J'` con relación 3:1 de potencia entre ruedas para trazar curvas suaves en carrera.

### 4. Interfaz de Usuario en `HardwareDrawer.svelte`
- **Decisión:** Crear una pestaña independiente `🚗 Carro BT ESP32` (`activeTab === 'esp32'`) en el navegador de pestañas.
- Contendrá:
  - Input reactivo para el nombre del Bluetooth con validación (`maxlength="31"`).
  - Selector de puerto COM y velocidad de subida (460800 baud para flasheo rápido, 115200 como alternativa).
  - Botón de acción con estado de progreso (`isFlashing`).
  - Terminal de logs integrada.
  - Tarjeta de diagrama visual de conexiones ESP32 DevKit V1 <-> Driver TB6612.
  - Tabla de referencia de comandos Bluetooth y enlace a apps de control Android/iOS.

## Risks / Trade-offs

- **[Riesgo] Detección de arranque en bootloader del ESP32:** Algunos clones de ESP32 DevKit V1 con circuitos de auto-reset capacitivos defectuosos requieren presionar el botón `BOOT` (GPIO 0) para entrar en modo flasheo.
  - *Mitigación:* Se agregará un mensaje claro en la UI y en la consola indicando: "Si el proceso queda esperando en 'Connecting...', mantén presionado el botón BOOT en la placa ESP32 por 2 segundos".
- **[Riesgo] Longitud y caracteres especiales en el nombre Bluetooth:** Nombres con caracteres fuera de ASCII estándar o mayores a 31 bytes pueden corromper el stack de `BluetoothSerial`.
  - *Mitigación:* Validación estricta en el frontend con regex `^[a-zA-Z0-9_-]+$` y recorte a 31 caracteres con advertencia visual.

## Migration Plan

1. Los recursos de flasheo de Arduino Nano (`avrdude`, `.hex`) permanecen inalterados.
2. Los nuevos binarios para ESP32 se alojan en `resources/firmwares/esp32/`.
3. La interfaz agrega una pestaña adicional sin interferir con las pestañas existentes de flasheo, pines o drivers.
