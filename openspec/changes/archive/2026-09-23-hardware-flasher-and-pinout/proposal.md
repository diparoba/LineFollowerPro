# Proposal: Hardware Flasher, Pinout & Drivers Suite

## Why

En ambientes de competencia y calibración en pista, los pilotos y técnicos frecuentemente enfrentan dos obstáculos críticos al usar laptops ajenas o recién formateadas:
1. **Falta de drivers USB-Serial (CH340/FTDI/CP2102)** que impiden reconocer el microcontrolador.
2. **Fallos de flasheo de firmware** por discrepancia entre el bootloader tradicional (*Old Bootloader* a 57600 baudios) y el nuevo (*Optiboot* a 115200 baudios), o confusión al conectar cables en carros de 16 sensores multiplexados vs 8 sensores analógicos directos.

Esta suite integra un flasheador autónomo en un panel lateral de la aplicación de escritorio que permite subir firmwares de 16L y 8L con 1 solo clic (con auto-fallback de bootloader y soporte de 115200 y 9600 baudios), visualizar esquemas claros de cableado para cada placa y facilitar la instalación de drivers sin requerir PlatformIO, VS Code ni el Arduino IDE en la máquina de competencia.

## What Changes

- **Panel Lateral Deslizante (*Hardware & Flasher Suite*)**:
  - Incorporar un drawer lateral en la interfaz Svelte con 3 pestañas: *Flasheador de Firmware*, *Esquema de Pines & Conexiones* y *Drivers USB*.
- **Flasheador de Firmware con Auto-Fallback**:
  - Embeber los binarios `.hex` de producción para `nano_im16` (16L) y `nano_codex8` (8L).
  - Integrar invocación de `avrdude` empaquetado desde el backend en Rust.
  - Opciones de velocidad: `115200` y `9600` baudios, con algoritmo de auto-fallback a `57600` baudios si el microcontrolador tiene el *Old Bootloader*.
  - Liberación automática del puerto serial antes de flashear y reanudación posterior para evitar errores de puerto ocupado.
- **Esquema Interactivo de Conexiones & Pinout**:
  - Vista gráfica interactiva del pinout de Arduino Nano con tabla comparativa de conexionado para 16L (multiplexor CD4051 en A0..A5) vs 8L (entradas directas A0..A7).
- **Asistente de Drivers USB**:
  - Acceso directo e instalación asistida de los controladores más comunes en robots de competencia: WCH CH340/CH341, Silicon Labs CP2102/CP2104 y FTDI FT232R.

## Capabilities

### New Capabilities
- `hardware-flasher`: Flasheo autónomo de microcontroladores ATmega328P con selección de carro (16L/8L), velocidades configurables (115200 / 9600 baud), auto-fallback de bootloader, esquema visual de pines de conexionado y asistente de drivers USB desde un panel lateral.

### Modified Capabilities
- *(Ninguna; `desktop-shell` permanece intacto manteniendo su soporte de telemetría y perfiles).*

## Impact

- **Backend Rust (`src-tauri/`)**:
  - Nuevos comandos Tauri IPC: `flash_firmware(port, robot_type, baud_rate, auto_fallback)`, `get_embedded_hex_info()`, `install_driver(driver_type)`.
  - Inclusión de `avrdude.exe` y `avrdude.conf` en los recursos de Tauri.
- **Frontend Svelte (`frontend/src/`)**:
  - Nuevo componente `HardwareDrawer.svelte` integrado en `App.svelte`.
  - Botón de apertura en la barra de navegación superior.
- **Firmwares**:
  - Generación y empaquetado de los binarios precompilados `firmware_im16.hex` y `firmware_codex8.hex`.
