# Proposal: Modo Piso Reflectante, Recuperación Robusta de Bifurcaciones y Permisos Web Serial

## Why

En competencias de robótica y pistas con acabado brillante/coliseo, la luz cenital produce reflejos que ciegan los sensores infrarrojos o saturan la cinta negra, degradando el seguimiento del robot. Adicionalmente, el firmware actual sufre un bug crítico en la detección de bifurcaciones donde la bandera se apaga tras solo 1 ms debido al temporizador de enfriamiento (`cooldown`), causando que el robot ignore el cruce o promedie ambas ramas y se salga de la pista. 

Por último, en la suite web y pantallas pequeñas (como laptops de estudiantes), la barra superior de conexión colapsa verticalmente a una franja de ~25px debido a la compresión flexbox y recorte de tarjetas, ocultando los selectores de puerto COM; a su vez, los navegadores modernos exigen una interacción explícita (`user gesture`) para autorizar la Web Serial API y acceder al Arduino o ESP32.

Este cambio implementa el filtrado antirreflejos en firmware, corrige el algoritmo de bifurcación con sostenimiento de 160 ms y seguimiento de bordes, soluciona el layout responsivo del encabezado y añade la concesión interactiva de permisos Web Serial.

## What Changes

- **Firmware: Modo Piso Reflectante (Antirreflejos):**
  - Incorpora Noise Gate (suprime valores $\le 250$ en fondo blanco) y Techo dinámico (eleva valores $\ge 760$ sobre cinta negra iluminada).
  - Activa una ventana espacial móvil ($\pm 3$ sensores en 8L, $\pm 4$ en 16L) alrededor de la última posición conocida para ignorar reflejos lejanos.
  - Añade el campo `reflective_mode` a `RobotConfig` y actualiza la firma de EEPROM a `EEPROM_MAGIC = 0x4C47` ("LG").
- **Firmware: Detección y Sostenimiento de Bifurcaciones:**
  - Corrige el bug del pulso de 1 ms implementando un sostenimiento temporal real (`bifurcation_active_until = now_ms + 160`).
  - Detección de bifurcación multilínea (`segments >= 2` o línea ancha con `width_span >= min_fork_span`).
  - Seguimiento de bordes (*Edge Tracking*): para `FORK_LEFT` engancha los sensores contiguos a `last_active`; para `FORK_RIGHT` los contiguos a `first_active`; para `FORK_STRAIGHT` aísla el centro.
- **Protocolo Serial Dual-Mode:**
  - Soporte de 8vo parámetro en `$PID,kp,kd,base,max,brake,fork,color,reflective`.
  - Soporte para comandos directos `$CMD,REFLECT_ON` y `$CMD,REFLECT_OFF`.
  - Retrocompatibilidad codificada en `line_color` (2 = Negra Reflectante, 3 = Blanca Reflectante).
- **Interfaz de Usuario & Responsividad:**
  - Evita el aplastamiento vertical de `.workspace-header` y `ControlBar` en pantallas compactas/laptops mediante `flex-shrink: 0`, altura automática (`min-height: max-content`) y manejo de desbordamiento limpio.
  - Añade botón interactivo en `ControlBar.svelte` para solicitar permisos de puerto USB en navegador (`navigator.serial.requestPort()`) para Arduino y ESP32.
  - Añade selector en `TuningPanel.svelte` para alternar entre "Piso Mate / Normal" y "Piso Reflectante".
- **Backend & Persistencia SQLite:**
  - Actualiza el modelo de perfil de robot y servicio serial para enviar y persistir `reflective_mode`.
  - Migración automática de columna `reflective_mode INTEGER NOT NULL DEFAULT 0` en tabla `profiles`.

## Capabilities

### New Capabilities
- `reflective-floor-and-bifurcation`: Define el algoritmo de supresión de reflejos parásitos, la ventana de sostenimiento de 160 ms para bifurcaciones en Y/T, el seguimiento de bordes en sensores y el protocolo serial de conmutación de piso reflectante.

### Modified Capabilities
- `desktop-shell`: Añade la protección contra colapso vertical en barras de control para pantallas compactas, el botón de concesión de permisos Web Serial en el navegador y el selector de modo reflectante en el panel de sintonización.

## Impact

- **Firmware:** `firmware/include/config.h`, `firmware/include/sensors.h`, `firmware/include/protocol.h`, `firmware/src/main.cpp`.
- **Frontend Svelte:** `frontend/src/lib/ControlBar.svelte`, `frontend/src/lib/TuningPanel.svelte`, `frontend/src/App.svelte`.
- **Tauri Backend:** `frontend/src-tauri/src/serial.rs`, `frontend/src-tauri/src/models.rs`, `frontend/src-tauri/src/db.rs`, `frontend/src-tauri/src/lib.rs`.
- **Base de datos:** `follower.db` agrega columna `reflective_mode` mediante `ALTER TABLE` sin afectar perfiles previos.
