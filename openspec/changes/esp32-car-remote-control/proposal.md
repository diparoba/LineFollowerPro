# Proposal: Control Remoto Integrado para Carro ESP32

## Why

Actualmente, para probar y maniobrar el Carro Robótico Bluetooth con ESP32 es necesario utilizar un smartphone externo mediante apps de terceros. Los usuarios requieren poder probar, diagnosticar y conducir el carro directamente desde la computadora donde ejecutan la suite de escritorio, tanto mediante conexión inalámbrica Bluetooth (puerto COM virtual de Windows) como mediante cable USB en banco de pruebas, con controles intuitivos de teclado (WASD / Flechas) y un pad virtual con parada de seguridad automática (*dead-man's switch*).

## What Changes

- Creación de un componente modal interactivo de Control Remoto (`Esp32RemoteModal.svelte`) con diseño de transmisor RC / Gamepad.
- Integración en la barra lateral [`Sidebar.svelte`](file:///c:/PVIProy/Seguidor_Nano_16/frontend/src/lib/Sidebar.svelte) con el botón `🎮 Control Remoto` y acceso rápido desde [`HardwareDrawer.svelte`](file:///c:/PVIProy/Seguidor_Nano_16/frontend/src/lib/HardwareDrawer.svelte) en la pestaña `🚗 Carro BT ESP32`.
- Soporte para conducción por teclado físico:
  - `W` / `↑`: Adelante (`'F'`)
  - `S` / `↓`: Reversa (`'B'`)
  - `A` / `←`: Izquierda (`'L'`)
  - `D` / `→`: Derecha (`'R'`)
  - Diagonales suaves combinadas: `W+A` (`'G'`), `W+D` (`'I'`), `S+A` (`'H'`), `S+D` (`'J'`)
  - Parada automática de seguridad al soltar cualquier tecla (`keyup` envía `'S'`).
  - Barra espaciadora: Freno de emergencia instantáneo.
- D-Pad interactivo en pantalla con soporte de eventos de ratón (`mousedown`/`mouseup`) y táctiles.
- Selector dinámico de velocidad / acelerador con envío de comandos escalados (`'0'` a `'9'`, `'q' = 100%`).
- Selector de puerto COM y baudrate integrado en el modal con estado de conexión en vivo y monitor de comandos transmitidos (`[TX]`).

## Capabilities

### Modified Capabilities
- `esp32-bluetooth-car`: Incorpora el requerimiento de control remoto virtual integrado en la interfaz de escritorio para envío de comandos de movimiento, diagonales, parada de seguridad y aceleración en tiempo real sobre puertos seriales (Bluetooth SPP / USB).

## Impact

- Frontend: Nuevo componente `frontend/src/lib/Esp32RemoteModal.svelte`, actualización de `Sidebar.svelte`, `HardwareDrawer.svelte` y `App.svelte`.
- Backend Tauri / Serial: Utiliza la API serial existente (`send_raw`, `connect`, `disconnect`, `get_available_ports`).
- Compatibilidad: Compatible al 100% con el firmware existente `esp32_bt_car.cpp` sin requerir modificaciones en el microcontrolador.
