# Tasks: Hardware Flasher, Pinout & Drivers Suite

## 1. Preparación de Binarios y Recursos

- [x] 1.1 Compilar los firmwares `nano_im16` y `nano_codex8` con PlatformIO y copiar los binarios `firmware_im16.hex` y `firmware_codex8.hex` en `frontend/src-tauri/resources/firmwares/`.
- [x] 1.2 Copiar `avrdude.exe` y `avrdude.conf` en `frontend/src-tauri/resources/avrdude/` y configurar `tauri.conf.json` para incluir `resources` en el empaquetado.

## 2. Backend Nativo en Rust (Flasher & Drivers)

- [x] 2.1 Implementar el módulo `frontend/src-tauri/src/flasher.rs` con la lógica de invocación de `avrdude` para ATmega328P, captura en streaming de stdout/stderr hacia eventos de la ventana (`flash-progress`, `flash-log`), desconexión previa de puerto COM y auto-fallback de bootloader (115200 -> 57600).
- [x] 2.2 Implementar el comando de lanzamiento de instaladores de drivers USB (`install_driver`) en Rust usando `std::process::Command` para abrir el instalador correspondiente.
- [x] 2.3 Registrar los comandos en `frontend/src-tauri/src/lib.rs` (`flash_firmware`, `install_driver`, `get_available_firmwares`) y verificar compilación limpia con `cargo check`.

## 3. Componente Frontend: Panel Lateral Deslizante (*HardwareDrawer*)

- [x] 3.1 Crear `frontend/src/lib/HardwareDrawer.svelte` con diseño *Luxury Precision Instrument*, selector de 3 pestañas (*Flasheador*, *Pinout & Conexiones*, *Drivers USB*) y animación suave de apertura/cierre.
- [x] 3.2 Desarrollar la pestaña **Flasheador**: selectores de modelo (16L / 8L), selector de velocidad (115200 / 9600), checkbox de auto-fallback inteligente, botón "🚀 Flashear Firmware" y terminal de consola en vivo con barra de progreso.
- [x] 3.3 Desarrollar la pestaña **Esquema de Pines & Conexiones**: diagrama visual del Arduino Nano con pines resaltados y tabla comparativa detallada para 16L (multiplexado A0..A5) vs 8L (directo A0..A7), motores (D3, D4, D5 y D11, D10, D9), botón D2 y LED D13.
- [x] 3.4 Desarrollar la pestaña **Drivers USB**: botones de acción rápida para instalar CH340/CH341, CP2102/CP2104 y FTDI con guía paso a paso.
- [x] 3.5 Integrar `HardwareDrawer.svelte` en `frontend/src/App.svelte` agregando el botón de acceso `🛠️ Hardware & Flasher` en la barra de navegación superior.

## 4. Validación y Compilación de Producción

- [x] 4.1 Probar la interfaz visual y verificar la apertura del drawer lateral, conmutación reactiva entre esquemas de 16L y 8L, y visualización de pines.
- [x] 4.2 Compilar el ejecutable final con `pnpm tauri build --no-bundle` y verificar la generación de `LineFollowerPro.exe` en `release_desktop_v0.2.0-beta/`.
