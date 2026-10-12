# Design: Responsive Header & Control Bar Fix and v1.2.0 Versioning

## Context & Root Cause Analysis

### Problema Visual en Laptops
En pantallas con altura útil reducida (como laptops estándar 1366x768 o pantallas Full HD con escalado de DPI en Windows de 125% o 150%), la altura visible dentro de la ventana de la aplicación o navegador oscila entre 600px y 720px.

Al estar configurado el contenedor principal `.main-workspace` con:
```css
display: flex;
flex-direction: column;
height: 100%;
```
Todos sus elementos hijos directos (`header.workspace-header`, `div.control-bar`, `div.status-banner`, `div.content-grid`) se comportan como flex items. Por especificación CSS, el valor por defecto de `flex-shrink` es `1`. 

Dado que `.content-grid` posee un tamaño intrínseco considerable para albergar los 16 sensores, los motores, los controles de PID y la tabla de perfiles, Flexbox comprimía verticalmente a los elementos previos (`workspace-header` y `control-bar`) hasta dejarlos en alturas mínimas de 12px a 18px. En combinación con la clase `.precision-card { overflow: hidden; }`, todo el contenido interno (textos, selects, botones) quedaba recortado y totalmente invisible.

## Arquitectura de la Solución

### 1. Inmunidad a la Compresión Vertical
- Añadir `flex-shrink: 0;` a:
  - `.workspace-header`: altura mínima garantizada (`min-height: 48px`).
  - `.control-bar`: altura mínima garantizada (`min-height: 52px`).
  - `.status-banner`: para que el texto de estado del robot nunca se trunque.
- Asegurar que `.main-workspace` active de manera fluida su `overflow-y: auto;` permitiendo scroll vertical si la pantalla no alcanza a mostrar todo el contenido simultáneamente.

### 2. Ergonomía y Legibilidad de Controles
- **Grupo Serial (`.serial-section`)**:
  - Selector COM con altura de 36px, tipografía nítida y contraste mejorado.
  - Botón de refresco `🔄` accesible y proporcional.
  - Selector de velocidad en baudios claro (115200 / 9600).
  - Botón principal de conexión (`.btn-connect` / `.btn-disconnect`) con `padding: 0.45rem 1rem`, `font-size: 0.85rem`, tipografía en negrita y color azul brillante `#0284c7` para visibilidad inmediata.
- **Grupo de Acciones de Carrera (`.robot-actions`)**:
  - Botones con `min-height: 34px`, iconos nítidos y espaciado consistente.

### 3. Unificación de Versionamiento a v1.2.0
El proyecto ha incorporado funcionalidades mayores:
1. Flasheo web de seguidores de línea (`IM_16` y `CODEX_8`) mediante backend `avrdude` y fallback a 57600 baudios.
2. Soporte BLE 4.2 para ESP32 DevKit V1 con pulsos en GPIO 2.
3. Soporte BLE 5.0 para ESP32-S3 con NeoPixel en GPIO 48.
4. Normalización lineal de velocidad de `0` a `9` (0% a 100% PWM).
5. Corrección de responsividad en cabecera y barra de control.

Por consiguiente, la versión pasa de `v1.1.0` a **`v1.2.0`** en:
- `frontend/package.json`
- `frontend/src-tauri/Cargo.toml`
- `frontend/src-tauri/tauri.conf.json`
- `frontend/src/lib/Sidebar.svelte`
- `backend/LineFollower.Api.csproj`
