# Design: Hardware Flasher, Pinout & Drivers Suite

## Context

La aplicación de escritorio se ejecuta de forma nativa en Windows con Tauri v2 y Rust. La plataforma cuenta con compilaciones de firmware en `firmware/` para los dos modelos de robots soportados:
- `nano_im16`: Microcontrolador ATmega328P con 16 sensores infrarrojos multiplexados (CD4051) en pines A0..A5.
- `nano_codex8`: ATmega328P con 8 sensores infrarrojos analógicos directos en pines A0..A7.

Véase `proposal.md` para la justificación y motivación.

## Goals / Non-Goals

**Goals:**
- Proporcionar flasheo directo de firmwares en 1 clic sin dependencias de IDEs externos.
- Gestionar discrepancias de bootloader mediante auto-fallback transparente (115200 -> 57600 baudios).
- Ofrecer un esquema interactivo de pines y cableado para validar conexiones antes de subir código.
- Disponer de accesos directos para la instalación de controladores USB (CH340, CP2102, FTDI).
- Alojar todas estas herramientas en un panel lateral deslizante (*side drawer*) no invasivo.

**Non-Goals:**
- Compilación en tiempo real desde el código fuente C++ en la máquina de usuario (los `.hex` van precompilados y empaquetados).
- Flasheo de microcontroladores sin bootloader mediante programadores ISP (USBasp / Arduino ISP).

## Decisions

### 1. Invocación de `avrdude` desde el Backend en Rust
- **Decisión:** Empaquetar `avrdude.exe` y `avrdude.conf` dentro del directorio de recursos de Tauri (`src-tauri/resources/avrdude/`) y ejecutarlos mediante `std::process::Command` en un hilo secundario en Rust.
- **Alternativas consideradas:**
  - *Requerir avrdude instalado en el sistema*: Descartado porque violaría el principio de portabilidad "cero-configuración" en laptops de competencia.
  - *Librería nativa en Rust (e.g. avr-flasher)*: Descartado por inestabilidad y falta de soporte robusto para el protocolo STK500v1 de los clones chinos.

### 2. Auto-Fallback de Bootloader Inteligente
- **Decisión:** Implementar un bucle de reintento en Rust:
  1. Si la velocidad inicial configurada es 115200 baudios y el proceso falla con errores típicos de sincronización (`stk500_getsync()`, `timeout`, `programmer is not responding`), el backend reintenta automáticamente a 57600 baudios (tasa oficial del *Arduino Nano Old Bootloader*).
  2. Si el usuario seleccionó 9600 baudios, se utiliza dicha tasa para proyectos con bootloader o módulos especiales.
- **Alternativas consideradas:**
  - *Exigir que el usuario seleccione el bootloader exacto manualmente*: Propenso a errores humanos de principiantes en competencias con poco tiempo.

### 3. Liberación Atómica del Puerto Serial
- **Decisión:** El comando `flash_firmware` en Rust solicita a `SerialService` cerrar el descriptor de archivo del puerto COM seleccionado antes de invocar a `avrdude`. Al terminar el flasheo, emite un evento `flash_finished` con el código de salida y log completo.
- **Alternativas consideradas:**
  - *Confiar en que el usuario presione desconectar manualmente*: Riesgo alto de error `Access Denied` en Windows cuando el puerto sigue capturado.

### 4. Componente `HardwareDrawer.svelte`
- **Decisión:** Implementar un drawer deslizante por el lado derecho con CSS puro (`transform: translateX(0)` / `transition: transform 0.25s cubic-bezier(...)`), respetando la estética *Luxury Precision Instrument* (bordes sutiles, microgradientes y fuente JetBrains Mono).
- **Pestañas incluidas:**
  - `Flasher`: Selección de modelo (16L/8L), baudrate (115200/9600), toggle de auto-fallback, terminal de logs en vivo.
  - `Pinout`: Esquema SVG/CSS del chip con resaltado reactivo y tabla de correspondencia de pines.
  - `Drivers`: Botones con lanzamiento asistido de instaladores (`CH341SER.EXE`, etc.).

## Risks / Trade-offs

- **[Riesgo] Antivirus de Windows bloqueando la invocación de `avrdude.exe`** → *Mitigación:* Colocar `avrdude.exe` junto a la estructura de la aplicación y firmar o ejecutar como subproceso directo de `LineFollowerPro.exe`.
- **[Riesgo] Tamaño del ejecutable aumentando drásticamente con los instaladores de drivers** → *Mitigación:* Embeber `avrdude` (~400 KB) y los archivos `.hex` (~60 KB), manteniendo los instaladores pesados de drivers en una subcarpeta o descargables con 1 clic para no inflar innecesariamente el ejecutable principal.
