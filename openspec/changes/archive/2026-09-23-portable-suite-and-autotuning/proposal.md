# Proposal: Suite Portable Definitiva & Asistente de Auto-Sintonización PID

## Why

En competencias de robótica de seguidores de línea, los equipos operan frecuentemente en coliseos y pistas deportivas sin conexión a internet y utilizando computadoras prestadas o de jueces donde no existen permisos de Administrador para instalar software en `Program Files` ni ensuciar carpetas del sistema.

Para alcanzar la versión definitiva de la suite (Fase 4 - v1.0.0), es crítico que la aplicación mantenga **portabilidad absoluta plug-and-play en memoria flash USB** (asegurando que el ejecutable y la base de datos `follower.db` viajen juntos en el pendrive), que los instaladores de drivers USB (CH340/FTDI) se ejecuten directamente de manera local y offline con elevación de privilegios de Windows sin redirigir a internet, y que se incorpore un **Asistente Analítico de Auto-Sintonización PID** que estime matemáticamente $K_p$ y $K_d$ a partir del voltaje de batería y la geometría física del robot.

## What Changes

- **Instalador de Drivers Offline Embebido:**
  - Empaquetado de `CH341SER.EXE` dentro de los recursos de la suite (`resources/drivers/`).
  - Ejecución local directa del instalador con permisos de Administrador de Windows (`runas`) desde el panel lateral, eliminando la dependencia de enlaces web y conexión a internet.
- **Portabilidad de Base de Datos & Respaldo con 1 Clic:**
  - Anclaje determinista de la ruta de SQLite `follower.db` relativa a `std::env::current_exe()`, garantizando que la base de datos siempre resida en la raíz de la memoria USB junto al ejecutable.
  - Nuevo botón "💾 Crear Respaldo / Backup" en la interfaz de gestión de perfiles para clonar la base de datos con un nombre fechado (ej: `backup_follower_YYYY-MM-DD.db`).
- **Asistente de Auto-Sintonización PID (Auto-Tuning):**
  - Nuevo modal interactivo `AutoTuningModal.svelte` con formulario de parámetros físicos:
    * Voltaje de batería ($V_{\text{in}}$: 2S LiPo 7.4V–8.4V, 3S LiPo 11.1V–12.6V, 5V Regulado).
    * Distancia de la barra de sensores al eje de tracción ($L$ en mm).
    * Ancho de vía / trocha entre ruedas ($W$ en mm).
    * Velocidad base crucero ($V_{\text{base}}$ 0–255).
    * Categoría activa: 16 Canales Multiplexados ($0–15000$) u 8 Canales Directos ($0–7000$).
    * Perfil de conducción (Conservador, Equilibrado, Agresivo).
  - Cálculo matemático en tiempo real de sugerencias para $K_p$, $K_d$, velocidad base, velocidad máxima y frenos.
  - Simulador de respuesta al escalón interactivo para visualizar el comportamiento dinámico antes de aplicar a la RAM o guardar en la EEPROM.

## Capabilities

### New Capabilities
- `autotuning-and-portability`: Define los requisitos de ejecución offline de instaladores de drivers USB con permisos de administrador, la persistencia obligatoria de la base de datos SQLite en el mismo directorio del ejecutable portable, la función de respaldo rápido de perfiles y el cálculo analítico de ganancias PID según geometría del chasis y voltaje de alimentación.

### Modified Capabilities
*(Ninguna. Las capacidades previas de flasheo y caja negra permanecen intactas).*

## Impact

- **Frontend (`frontend/src/`):**
  - Nuevo componente `AutoTuningModal.svelte`.
  - Botón de apertura de Auto-Tuning en la barra de control o encabezado.
  - Botón de respaldo en `ProfileManager.svelte`.
  - Actualización de `HardwareDrawer.svelte` para invocar el instalador local.
- **Backend Tauri / Rust (`frontend/src-tauri/`):**
  - Anclaje de ruta en `db.rs` y `lib.rs` vía `current_exe()`.
  - Nuevo comando `backup_database` en Rust.
  - Modificación de `install_driver` en `flasher.rs` para invocar `CH341SER.EXE` local con elevación UAC.
- **Recursos (`resources/`):**
  - Incorporación de `resources/drivers/CH341SER.EXE`.
