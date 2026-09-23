# Tasks: Migración a Tauri Desktop (Rust + WebView2)

## 1. Documentación de Estrategia y Entorno

- [x] 1.1 Crear `ROADMAP.md` formalizando visión, arquitectura, etapas de evolución (Fases 1, 2 y 3) y verificar que el archivo exista en la raíz del repositorio.
- [x] 1.2 Crear `VERSIONING.md` detallando las reglas formales de SemVer 2.0.0 para Desktop, Firmware y Esquema SQLite, y verificar que el archivo exista en la raíz del repositorio.
- [x] 1.3 Verificar e instalar el toolchain de Rust (`rustup` / `cargo`) con target `x86_64-pc-windows-msvc` ejecutando `cargo --version`.

## 2. Inicialización y Configuración de Tauri

- [x] 2.1 Agregar dependencias `@tauri-apps/api` y `@tauri-apps/cli` en el frontend e inicializar la estructura `src-tauri` verificando `pnpm tauri --version`.
- [x] 2.2 Configurar `src-tauri/tauri.conf.json` con título de ventana, dimensiones mínimas 1100x700, íconos y vinculación con la carpeta `../frontend/dist`.

## 3. Implementación del Backend Nativo en Rust

- [x] 3.1 Implementar el módulo de base de datos SQLite en `src-tauri/src/db.rs` con `rusqlite` manteniendo la estructura de la tabla `profiles` (`car_name`, `car_category`, PID, velocidades) y verificar compilación de módulo.
- [x] 3.2 Implementar el motor de comunicación serial en `src-tauri/src/serial.rs` usando `serialport` para escaneo de puertos COM, apertura a 115200 baudios, envío de `$PID` / `$CMD` / `$EEPROM` y streaming en segundo plano de telemetría hacia eventos de ventana.
- [x] 3.3 Registrar los comandos Tauri en `src-tauri/src/main.rs` (`get_ports`, `connect_serial`, `disconnect_serial`, `send_pid`, `send_command`, `save_eeprom`, `read_eeprom`, `get_profiles`, `save_profile`, `delete_profile`) y compilar con `cargo check`.

## 4. Adaptación del Frontend Svelte

- [x] 4.1 Crear un servicio puente `frontend/src/lib/tauriBridge.js` que invoque comandos IPC de Tauri y escuche el evento `telemetry` con fallback transparente a modo web.
- [x] 4.2 Conectar `frontend/src/App.svelte` al puente Tauri conservando todos los componentes visuales, temas Dark/Light, simulador de hardware y selector de flota de carros.
- [x] 4.3 Probar la interfaz en modo desarrollo de escritorio ejecutando `pnpm tauri dev` y verificando la apertura de la ventana nativa.

## 5. Compilación y Validación de Binario Portable

- [x] 5.1 Compilar el ejecutable de producción con `pnpm tauri build` y verificar la generación del binario `.exe` en `src-tauri/target/release/`.
- [x] 5.2 Verificar que el tamaño del ejecutable generado sea menor a 20 MB y que se ejecute de manera autónoma sin requerir servidores web en segundo plano.
