# Design: Modo Piso Reflectante, Recuperación Robusta de Bifurcaciones y Permisos Web Serial

## Context

Véase `proposal.md` para la motivación. Actualmente:
1. El firmware de ATmega328P (`sensors.h`) evalúa la bifurcación restableciendo `bifurcation_detected = false` en cada vuelta de bucle, lo que hace que el filtro de rama dure solo 1 ms antes de que el cooldown de 250 ms suprima la detección, provocando pérdida de trayectoria en curvas en "Y".
2. En superficies de alta reflectividad, los fototransistores captan destellos en fondo blanco y atenuación de contraste sobre cinta negra brillante.
3. En la interfaz web y pantallas de laptops (1366x768 o con escalado de Windows), `.workspace-header` y `ControlBar` sufren compresión vertical por Flexbox (`flex-shrink: 1`) y `overflow: hidden` en `.precision-card`, colapsando a franjas de ~25px que ocultan los selectores COM y botones.
4. En el navegador web (`!isTauri()`), la Web Serial API requiere una llamada interactiva (`navigator.serial.requestPort()`) ejecutada por un clic de usuario para otorgar permisos a dispositivos USB como Arduino Nano o ESP32.

## Goals / Non-Goals

**Goals:**
- Sostener la maniobra de bifurcación de manera ininterrumpida por 160 ms con seguimiento de bordes (`first_active` para `FORK_RIGHT`, `last_active` para `FORK_LEFT`).
- Filtrar destellos mediante compuerta de ruido ($\le 250$), techo de saturación ($\ge 760$) y ventana espacial móvil en modo reflectante.
- Preservar el 100% de la máquina de estados de 3 toques del pulsador de hardware y la retrocompatibilidad en EEPROM (`EEPROM_MAGIC = 0x4C47`).
- Garantizar que la barra superior de categoría y la barra de control serial mantengan su altura íntegra (`flex-shrink: 0`, `min-height: max-content`, `overflow: visible`) en cualquier resolución.
- Incorporar botón de autorización Web Serial en navegador para emparejar puertos USB/COM en un clic.
- Extender la base de datos local SQLite `follower.db` para almacenar `reflective_mode` en los perfiles de carrera.

**Non-Goals:**
- Reemplazar SQLite por PostgreSQL en la suite de escritorio: el ejecutable `.exe` debe ser 100% autónomo y portable en memorias USB sin dependencias de red o servidores en coliseos.
- Alterar la lógica básica de PID en pistas normales (`reflective_mode = 0`).

## Decisions

### 1. Sostenimiento Temporal (160 ms) y Seguimiento de Bordes en Bifurcaciones
- **Decisión:** Al detectar separación de líneas (`segments >= 2`) o ensanchamiento anormal (`width_span >= min_fork_span`), se fija `bifurcation_active_until = now_ms + 160` y `bifurcation_cooldown = now_ms + 400`.
- **Razón:** A velocidades de carrera (1.5 m/s - 2.5 m/s), 160 ms representa una distancia de recorrido de ~25 cm a 40 cm, tiempo exacto requerido para que el centroide atraviese la divergencia de la "Y" y quede firmemente posicionado sobre la rama elegida.
- **Alternativa considerada:** Algoritmo basado en contador de ciclos. Descartado porque la duración real variaría si cambia la frecuencia del loop o la telemetría.

### 2. Acondicionamiento Dinámico Antirreflejos (Noise Gate + Techo + Ventana Espacial)
- **Decisión:**
  - Si `reflective_mode == 1`:
    - Lecturas normalizadas $\le 250$ se fuerzan a $0$ (suprime luz especular en blanco).
    - Lecturas $\ge 760$ se fuerzan a $1000$ (compensa caída de reflectancia infrarroja en cinta negra bajo reflectores).
    - Escalado: `((val - 250) * 1000) / (760 - 250)`.
    - Umbral digital baja a 350 para detectar la línea atenuada.
    - Ventana espacial móvil: solo se evalúan $\pm 3$ sensores (8L) o $\pm 4$ sensores (16L) alrededor del último sensor activo.
- **Razón:** Elimina la necesidad de recalibrar físicamente los sensores o cambiar resistencias en pista.

### 3. Preservación de la Portabilidad Offline con SQLite
- **Decisión:** Mantener SQLite con migración suave (`ALTER TABLE profiles ADD COLUMN reflective_mode INTEGER NOT NULL DEFAULT 0;`) en `frontend/src-tauri/src/db.rs`.
- **Razón:** Permite que estudiantes y docentes usen la aplicación en coliseos sin acceso a internet. Para el hosting web centralizado (`robotica.ouroboros-software.com`), la base de datos SQLite con modo WAL es suficiente para perfiles de carrera; la integración con PostgreSQL se reserva para cuando se añada autenticación multiusuario.

### 4. Layout Header Antipérdida y Concesión de Permisos Web Serial
- **Decisión:**
  - En `.workspace-header` y `.control-bar`: aplicar `flex-shrink: 0;`, `min-height: max-content;` y remover el `overflow: hidden;` que recortaba los controles a ~25px.
  - En `ControlBar.svelte`: si `!isTauri()` y el navegador soporta Web Serial, mostrar el botón `🔑 Conceder Permiso USB` / `🌐 Conectar Web Serial` que llama a `navigator.serial.requestPort()`.

## Risks / Trade-offs

- **[Riesgo] Salto abrupto en el término derivativo ($K_d$) al enganchar una rama**  
  *Mitigación:* La ventana de cálculo alrededor de `last_active` / `first_active` conserva un radio de amortiguación (`branch_radius = 2 o 3`), suavizando la transición del centroide.
- **[Riesgo] Desbordamiento de EEPROM o lecturas corruptas con firmware anterior**  
  *Mitigación:* Se cambia la firma mágica a `EEPROM_MAGIC = 0x4C47` ("LG"). Si el Arduino contiene la versión anterior, inicializa los campos a valores seguros por defecto y guarda la nueva estructura.
- **[Riesgo] Navegadores sin soporte de Web Serial API (Firefox o Safari)**  
  *Mitigación:* La interfaz detecta `isWebSerialSupported()` y muestra un mensaje informativo indicando usar navegadores basados en Chromium (Chrome, Edge, Brave o la suite de escritorio Tauri).
