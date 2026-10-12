# Proposal: Responsive Header & Control Bar Fix and v1.2.0 Versioning

## Why
En pantallas de laptops (resoluciones comunes como 1366x768 o 1080p con escalado de 125%/150%), la barra superior del dashboard (`workspace-header` y `control-bar`) se aplastaba verticalmente a una altura mínima de ~15px debido a la compresión flexbox (`flex-shrink: 1`) y `overflow: hidden`, ocultando por completo los botones de selección de puerto COM, velocidad de baudios, conexión serial y acciones del robot.
Asimismo, tras la integración de soporte BLE para ESP32 DevKit V1, ESP32-S3 y flasheo web de seguidores de línea, se requiere unificar el versionamiento del proyecto a **v1.2.0** en todos los paquetes y entornos (frontend, tauri, backend y vistas).

## What Changes
- **Fijación Vertical Antipérdida (`flex-shrink: 0`)**: Asegurar que tanto la barra superior de categoría/flota (`workspace-header`) como la barra de control serial y comandos (`control-bar`) mantengan su altura íntegra y nunca se compriman por debajo de su tamaño natural.
- **Diseño Ergonómico y Accesible de Barra de Control**:
  - Incrementar el tamaño, contraste y legibilidad de los botones de conexión y selección de puerto COM (`font-size: 0.85rem`, `padding` confortable, selectores con altura mínima de 36px).
  - Distribuir los controles con flujo flexible responsivo (`flex-wrap: wrap`) evitando cortes y desbordamientos.
  - Asegurar que el scroll vertical de `.main-workspace` se active limpiamente cuando el contenido exceda la altura de la pantalla, sin recortar los controles superiores.
- **Actualización de Versión a v1.2.0**:
  - `frontend/package.json`: `1.2.0`
  - `frontend/src-tauri/Cargo.toml`: `1.2.0`
  - `frontend/src-tauri/tauri.conf.json`: `1.2.0`
  - `frontend/src/lib/Sidebar.svelte`: badge `v1.2.0`
  - `backend/LineFollower.Api.csproj`: `1.2.0` / `1.2.0.0`

## Capabilities

### Modified Capabilities
- `desktop-shell`: Actualizar los requisitos de visualización responsiva del dashboard y de la barra de control para garantizar que los elementos de conexión permanezcan 100% visibles y ergonómicos en laptops y pantallas con factor de escala.

## Impact
- **Frontend**: Modificaciones en `frontend/src/App.svelte` y `frontend/src/lib/ControlBar.svelte`, además del badge de versión en `frontend/src/lib/Sidebar.svelte`.
- **Configuraciones de compilación**: `frontend/package.json`, `frontend/src-tauri/tauri.conf.json`, `frontend/src-tauri/Cargo.toml` y `backend/LineFollower.Api.csproj`.
- **Compilaciones de producción**: Requiere `pnpm run build` y sincronización con `backend/wwwroot`.
