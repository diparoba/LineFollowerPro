# Design: Arquitectura Tauri Native Desktop para Seguidor de Línea Pro

## Context

Actualmente el sistema cuenta con:
1. Firmware doble para Arduino Nano (`nano_im16` y `nano_codex8`), optimizado con PWM ultrasónico a 31.37 kHz y streaming serial a 115200 baudios.
2. Frontend Svelte 5 + Vite con estética Luxury Precision, visualizadores duales (16 canales multiplexados y 8 canales directos), simulador Hardware-in-the-Loop y selector de perfiles con nombres de carros.
3. Backend .NET 10 que actúa como puente serial y servidor web Kestrel, empaquetado en un binario portable de ~105 MB.

Para lograr un ejecutable ultraligero (~10–15 MB) y una experiencia nativa de escritorio sin abrir pestañas en navegadores externos, migramos a **Tauri v2 (Rust + WebView2)**.

## Goals / Non-Goals

**Goals:**
- Crear la estructura `src-tauri` con backend nativo en Rust.
- Implementar escaneo y conexión serial usando el crate `serialport` de Rust a 115200 baudios con lectura multihilo sin bloqueo.
- Replicar el servicio SQLite usando `rusqlite` preservando la base de datos `follower.db` y las columnas existentes (`car_name`, `car_category`, PID, velocidades, bifurcación, color).
- Exponer comandos Tauri (`invoke`) para `get_ports`, `connect_serial`, `disconnect_serial`, `send_pid`, `send_command`, `save_eeprom`, `read_eeprom`, `get_profiles`, `save_profile`, `delete_profile`.
- Emitir telemetría serial en tiempo real vía eventos de ventana Tauri (`window.emit("telemetry", data)`).
- Diseñar la capa cliente en Svelte con compatibilidad híbrida (Tauri IPC directo con fallback para desarrollo web en Vite).
- Empaquetar un ejecutable Windows autónomo de menos de 15 MB.

**Non-Goals:**
- Modificar el protocolo de tramas del firmware Arduino (`$TEL`, `$PID`, `$CMD`, `$EEPROM`).
- Modificar el diseño estético ni la lógica de visualización y cálculo de simulación en Svelte.
- Eliminar el proyecto `backend/` de C# (se conserva en el repositorio como alternativa headless/servidor remoto).

## Decisions

### 1. Tauri v2 vs C# Photino / Electron
- **Decisión:** Tauri (Rust + WebView2).
- **Razón:** Electron genera binarios de 180+ MB con alto consumo de memoria RAM (>150 MB). Photino con .NET mantiene el tamaño sobre 80-100 MB debido al runtime BCL de C#. Tauri compila a código de máquina x64 nativo en Windows, aprovechando el runtime WebView2 ya preinstalado en Windows 10/11, resultando en un binario de solo 10–14 MB y consumo de RAM de ~30 MB.
- **Alternativas consideradas:** Electron (descartado por peso excesivo), Photino.NET (descartado por no reducir suficientemente el tamaño).

### 2. Comunicación Serial Multihilo en Rust
- **Decisión:** Hilo de lectura dedicado (`std::thread`) con canal MPSC y despacho de eventos Tauri.
- **Razón:** La lectura continua de datos seriales a 115200 baudios (paquetes a ~60 Hz) requiere no bloquear el hilo principal de la UI ni el despachador de comandos async de Tokio/Tauri. Un bucle con `io::BufRead` procesa líneas completas terminadas en `\n`, parsea `$TEL` y despacha el payload deserializado a la ventana webview.
- **Alternativas consideradas:** Tokio serial asíncrono (agrega dependencias complejas y posibles incompatibilidades de drivers COM en Windows; un hilo dedicado con `serialport` es más robusto y probado para puertos virtuales USB-Serial CH340/CP210x).

### 3. Persistencia SQLite Nativa con `rusqlite`
- **Decisión:** `rusqlite` con `bundled` para compilar SQLite C internamente de forma autosuficiente.
- **Razón:** Garantiza cero dependencias de DLLs externas en la máquina del usuario final y mantiene 100% de compatibilidad binaria con la base `follower.db` existente generada por Microsoft.Data.Sqlite.

### 4. Capa de Abstracción en Frontend (`src/lib/api.js`)
- **Decisión:** Crear un adaptador de transporte que detecte si la aplicación corre dentro de la ventana de Tauri (`window.__TAURI_INTERNALS__`). Si está en Tauri, usa `@tauri-apps/api/core` (`invoke`) y `listen`. Si corre en navegador independiente (`pnpm run dev`), puede redirigir a un mock o backend HTTP/WS.
- **Razón:** Permite a los desarrolladores iterar rápidamente en el frontend con Vite Hot-Reload y ejecutar la suite nativa con `tauri dev` / `tauri build`.

## Risks / Trade-offs

- **[Toolchain de Rust en máquina local]** → Si Rust no está instalado, se debe ejecutar `winget install Rustlang.Rustup` una sola vez. Microsoft C++ Build Tools ya están instaladas en el sistema.
- **[Permisos de USB/Serial en Windows]** → Si el usuario no tiene los drivers CH340 instalados, el puerto COM no aparece. Esto se mitiga con la Fase 2 del roadmap (empaquetado de drivers en `drivers/`).
- **[Concurrencia de acceso a puerto serial]** → Proteger el `Arc<Mutex<Option<Box<dyn SerialPort>>>>` para evitar condiciones de carrera entre la escritura de comandos (`$PID`) y la lectura continua.

## Migration Plan

1. Documentar Roadmap general (`ROADMAP.md`) y Reglas de Versionamiento (`VERSIONING.md`).
2. Instalar toolchain de Rust mediante `winget install Rustlang.Rustup` y configurar el target `x86_64-pc-windows-msvc`.
3. Inicializar el módulo Tauri en el proyecto (`src-tauri/`).
4. Implementar el motor serial y SQLite en Rust (`src-tauri/src/main.rs`, `serial.rs`, `db.rs`).
5. Integrar la capa de invocación en `frontend/` y validar en vivo.
6. Compilar el ejecutable final en modo `release` y verificar tamaño y rendimiento.
