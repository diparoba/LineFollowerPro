# Tasks

## 1. Corrección de Responsividad y Visibilidad en Barra Superior

- [x] 1.1 Aplicar `flex-shrink: 0` y `min-height: 48px` a `.workspace-header` en `frontend/src/App.svelte` para evitar compresión vertical en pantallas compactas.
- [x] 1.2 En `frontend/src/lib/ControlBar.svelte`, aplicar `flex-shrink: 0`, `min-height: 52px`, incrementar el tamaño y padding de los selectores COM/baudios y del botón de conexión serial (`min-height: 36px`, `font-size: 0.85rem`).
- [x] 1.3 Optimizar el comportamiento de desbordamiento en `.main-workspace` (`overflow-y: auto`) para que el scroll vertical se active sin recortar las barras superiores.

## 2. Unificación de Versionamiento a v1.2.0

- [x] 2.1 Actualizar a versión `1.2.0` en `frontend/package.json`, `frontend/src-tauri/tauri.conf.json` y `frontend/src-tauri/Cargo.toml`.
- [x] 2.2 Actualizar `Version`, `FileVersion` e `InformationalVersion` a `1.2.0` en `backend/LineFollower.Api.csproj`.
- [x] 2.3 Actualizar el badge visible en `frontend/src/lib/Sidebar.svelte` a `v1.2.0`.

## 3. Compilación, Verificación y Empaquetado

- [x] 3.1 Compilar el frontend con `pnpm run build` y sincronizar los archivos estáticos en `backend/wwwroot/`.
- [x] 3.2 Probar la compilación del backend y validar la interfaz en el navegador local.
