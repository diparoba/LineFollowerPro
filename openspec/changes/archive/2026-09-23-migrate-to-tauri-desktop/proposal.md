# Proposal: Migración a Tauri Desktop (Rust + WebView2) para Seguidor de Línea Pro

## Why

El empaquetado portable actual basado en .NET 10 Kestrel (`LineFollowerPro.exe`) pesa ~105 MB debido a que incluye el runtime completo de ASP.NET Core, y depende de levantar un servidor HTTP/WebSocket local y abrir el navegador web por defecto del usuario.
La migración a **Tauri (Rust + Windows WebView2)** reduce drásticamente el tamaño del ejecutable a ~10–15 MB, reduce el consumo de RAM a ~30 MB, elimina la dependencia de puertos de red locales (evitando alertas de firewall), y proporciona una experiencia nativa de escritorio de grado industrial con acceso serial asíncrono directo de latencia ultrabaja y SQLite integrado.

## What Changes

- **Desktop Shell con Tauri (Rust):** Sustituir el servidor Kestrel C# por un ejecutable nativo de escritorio Tauri (`line-follower-pro.exe`) aprovechando el motor WebView2 ya incorporado en Windows 10/11.
- **Motor Serial en Rust (`serialport`):** Implementar en el backend de Rust el escaneo de puertos COM, conexión/desconexión a 115200 baudios, emisión de telemetría a 60 FPS hacia el frontend mediante eventos nativos Tauri, e intercambio de tramas `$PID`, `$CMD` y `$EEPROM`.
- **Gestor SQLite en Rust (`rusqlite`):** Portar el servicio de base de datos local SQLite (`follower.db`) manteniendo el mismo esquema relacional (`profiles`: ID, nombre, car_name, car_category, PID, velocidades, modo bifurcación, color de línea).
- **Integración Transparente en Frontend:** El cliente Svelte (`App.svelte`, visualizadores de 16 y 8 líneas, modal de simulación hardware-in-the-loop) se comunica mediante comandos IPC (`invoke`) y escuchadores de eventos (`listen`) cuando corre dentro de Tauri, conservando el diseño Luxury Precision (Outfit + JetBrains Mono, radios 9px/6px/4px, temas Dark y Light).
- **Gestión de Drivers y Flasheo (Roadmap Fase 2):** Preparar la arquitectura para empaquetar instaladores de drivers (CH340 y CP210x) y selector de bootloader (Old Bootloader 57600 vs New Bootloader 115200).

## Capabilities

### New Capabilities
- `desktop-shell`: Aplicación nativa de escritorio ligera para Windows con IPC de alta velocidad, streaming serial continuo de telemetría hacia la interfaz, y persistencia local de perfiles en SQLite.

### Modified Capabilities
<!-- No existing capabilities were previously spec'd in openspec/specs -->

## Impact

- **Frontend (`frontend/`):** Adición de `@tauri-apps/api` y `@tauri-apps/cli`. Adaptación de la capa de comunicación para usar IPC nativo de Tauri con fallback reactivo.
- **Backend:** Se añade `src-tauri/` (código Rust con `Cargo.toml`, `main.rs`, comandos serial y SQLite). El backend C# (`backend/`) se mantiene como referencia y opción de servidor independiente o headless.
- **Distribución:** El ejecutable final se genera en `src-tauri/target/release/` como un binario Windows `.exe` ultraligero (~12 MB).
- **Herramientas de Desarrollo:** Requiere el compilador de Rust (`rustup` / `cargo`), que se conectará con los Microsoft Visual Studio C++ Build Tools ya presentes en el sistema.
