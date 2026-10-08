# Tasks: Control Remoto Integrado para Carro ESP32

## 1. Componente de Control Remoto Virtual

- [ ] 1.1 Crear el componente `frontend/src/lib/Esp32RemoteModal.svelte` con D-Pad interactivo (Adelante, Reversa, Izquierda, Derecha, Diagonales, Stop central), selector de velocidad PWM ('0' a '9', 'q'), selector de puerto COM integrado y monitor de comandos transmitidos.
- [ ] 1.2 Implementar en `Esp32RemoteModal.svelte` la captura de eventos de teclado físico (WASD / Flechas), detección de diagonales simultáneas, mecanismo de parada de seguridad (*dead-man switch* al soltar teclas o botones) y freno de emergencia en barra espaciadora.

## 2. Integración en Navegación y Shell de la Aplicación

- [ ] 2.1 Añadir el botón `🎮 Control Remoto` en [`Sidebar.svelte`](file:///c:/PVIProy/Seguidor_Nano_16/frontend/src/lib/Sidebar.svelte) y un botón de acceso directo en [`HardwareDrawer.svelte`](file:///c:/PVIProy/Seguidor_Nano_16/frontend/src/lib/HardwareDrawer.svelte) dentro de la pestaña `🚗 Carro BT ESP32`.
- [ ] 2.2 Montar el componente `Esp32RemoteModal` en `frontend/src/App.svelte` gestionando el estado reactivo de apertura, selección de puerto y emisión de comandos seriales.

## 3. Compilación, Empaquetado y Verificación

- [ ] 3.1 Compilar el frontend con `pnpm build` y compilar el binario nativo de escritorio con `pnpm tauri build --no-bundle`, verificando la integridad del bundle.
- [ ] 3.2 Actualizar el ejecutable en `release_desktop_v1.1.0/LineFollowerPro.exe` y comprobar la apertura y respuesta interactiva del control remoto.
