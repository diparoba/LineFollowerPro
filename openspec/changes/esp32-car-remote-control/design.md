# Design: Control Remoto Integrado para Carro ESP32

## Context

La aplicación de escritorio cuenta con comunicación serial bidireccional mediante el backend en Rust expuesto a través de [`tauriBridge.js`](file:///c:/PVIProy/Seguidor_Nano_16/frontend/src/lib/tauriBridge.js) (`connectSerial`, `disconnectSerial`, `listPorts`, `sendRaw`). Por su parte, el firmware del ESP32 ([`esp32_bt_car.cpp`](file:///c:/PVIProy/Seguidor_Nano_16/firmware/src/esp32_bt_car.cpp)) procesa comandos de un carácter tanto por Bluetooth Serial como por puerto USB serie tradicional (`F`, `B`, `L`, `R`, `G`, `I`, `H`, `J`, `S`, `0`..`9`, `q`).

## Goals / Non-Goals

**Goals:**
- Crear el componente `frontend/src/lib/Esp32RemoteModal.svelte` con aspecto visual de transmisor RC / Gamepad.
- Integrar acceso directo en [`Sidebar.svelte`](file:///c:/PVIProy/Seguidor_Nano_16/frontend/src/lib/Sidebar.svelte) (`🎮 Control Remoto`) y en [`HardwareDrawer.svelte`](file:///c:/PVIProy/Seguidor_Nano_16/frontend/src/lib/HardwareDrawer.svelte) (pestaña `🚗 Carro BT ESP32`).
- Soporte para conducción continua por teclado físico (WASD y flechas) con detección de diagonales combinadas.
- Mecanismo de seguridad *Dead-man's switch*: enviar `'S'` inmediatamente cuando el usuario suelte las teclas o los botones virtuales.
- Freno de emergencia instantáneo (Barra espaciadora y botón rojo central).
- Selector y slider de velocidad PWM con envío de comandos escalados.
- Selector de puerto COM y conexión serial integrado dentro del modal para conectar/desconectar sin abandonar la cabina de control.

**Non-Goals:**
- Modificar el protocolo del firmware (se reutilizan los comandos existentes ya probados).
- Modificar el backend de Rust (las APIs de `serial` son suficientes y operan a baja latencia).

## Decisions

### Decisión 1: Estructura como Modal Global (`Esp32RemoteModal.svelte`)
- **Elección:** Diseñar el control remoto como un modal flotante oscuro de alta visibilidad, montado en `App.svelte` (siguiendo el patrón establecido por `SimulationModal` y `AutoTuningModal`).
- **Alternativas:**
  - *Página dedicada:* Haría perder el contexto de la aplicación.
  - *Pestaña en Hardware Drawer:* Resultaría estrecha y limitaría el espacio visual para el D-Pad y los atajos de teclado.

### Decisión 2: Máquina de estados para teclado y prevención de fugas
- Se usará un `Set` reactivo de teclas presionadas (`pressedKeys`).
- Al presionar una tecla (`keydown`):
  - Se ignora si `event.repeat === true` para no saturar el buffer serie.
  - Se evalúa la combinación:
    - `W` solo $\rightarrow$ `'F'`
    - `S` solo $\rightarrow$ `'B'`
    - `A` solo $\rightarrow$ `'L'`
    - `D` solo $\rightarrow$ `'R'`
    - `W` + `A` $\rightarrow$ `'G'` (Diagonal Adelante-Izquierda)
    - `W` + `D` $\rightarrow$ `'I'` (Diagonal Adelante-Derecha)
    - `S` + `A` $\rightarrow$ `'H'` (Diagonal Atrás-Izquierda)
    - `S` + `D` $\rightarrow$ `'J'` (Diagonal Atrás-Derecha)
  - Solo se transmite el comando si difiere del último transmitido.
- Al soltar una tecla (`keyup`):
  - Si no quedan teclas direccionales activas, se envía `'S'` (Stop).
- Evento `window.onblur` y cierre de modal:
  - Si la ventana pierde foco (Alt+Tab) o el modal se cierra mientras se conducía, se emite forzosamente `'S'` por seguridad.

### Decisión 3: Interfaz Virtual del D-Pad
- Botones en cruz con diagonales en las esquinas y botón de parada en el centro.
- Manejo de `on:mousedown` y `on:mouseup` / `on:mouseleave`, así como `touchstart` y `touchend` para control táctil sin trabas.

## Risks / Trade-offs

- **[Riesgo] Retardo en envío si se satura el puerto serie:** → *Mitigación:* Se filtra la emisión para enviar datos solo en transiciones de estado (cuando cambia la combinación de teclas o al soltar).
- **[Riesgo] Accidente si el carro sale de control:** → *Mitigación:* Triple capa de seguridad: parada al soltar (`keyup`), freno en barra espaciadora y parada al perder foco (`blur`).
