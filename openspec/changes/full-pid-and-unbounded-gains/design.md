# Design: Ganancias PD Ilimitadas a 4 Decimales y Etiquetas de Sensibilidad / Corrección

## Context

El robot seguidor de línea (en configuraciones 16L multiplexado IM y 8L directo Codex) implementa un algoritmo de control PD que responde con gran velocidad al error de posición calculado sobre la pista. En la versión actual de la interfaz de usuario, los campos de $K_p$ y $K_d$ tenían restricciones rígidas de rango (por ejemplo $K_p \in [0.05, 1.0]$, $K_d \in [0.5, 15.0]$) y pasos de 2 o 3 decimales. El usuario requiere la eliminación de cualquier cota superior en las cajas de texto, una precisión de hasta 4 decimales (`step="0.0001"`), un límite inferior estricto en $0$, sliders adaptativos y el etiquetado intuitivo de **Sensibilidad** para $K_p$ y **Corrección** para $K_d$.

Ver motivación en [proposal.md](proposal.md).

## Goals / Non-Goals

**Goals:**
- Actualizar `TuningPanel.svelte` y `SimulationModal.svelte` con entradas numéricas sin atributo `max`, con `min="0"` y resolución `step="0.0001"`.
- Implementar sliders dinámicos en los que el atributo `max` se expanda automáticamente según el valor ingresado (`Math.max(baseMax, Math.ceil(val))`).
- Incluir etiquetas explícitas y badges visuales para identificar $K_p$ como **Sensibilidad** y $K_d$ como **Corrección**.
- Ajustar el formateo en la capa serial de Rust/Tauri (`frontend/src-tauri/src/serial.rs`) para emitir $K_p$ y $K_d$ con 4 decimales (`{:.4}`).
- Mantener la visualización de 4 decimales en las tarjetas de perfiles de `ProfileManager.svelte`.

**Non-Goals:**
- No se agregará término integral ($K_i$) al firmware ni a la base de datos, manteniendo el microcontrolador ATmega328P libre de sobrecarga computacional.
- No se alterará la estructura de la base de datos SQLite ni la memoria EEPROM física actual.
- No se modificarán los tiempos ni algoritmos de lectura de sensores ni los pines de hardware.

## Decisions

### 1. Entradas Numéricas Ilimitadas y Sliders Adaptativos
- **Decisión**: En los elementos `<input type="number">`, fijar `min="0"` y `step="0.0001"`, omitiendo el atributo `max`. Para los `<input type="range">`, establecer un valor base ($1.5$ para $K_p$, $20.0$ para $K_d$), pero vinculando reactivamente el valor máximo mediante una propiedad calculada:
  ```svelte
  $: maxKpSlider = Math.max(1.5, Math.ceil(kp * 2) / 2);
  $: maxKdSlider = Math.max(20.0, Math.ceil(kd / 5) * 5);
  ```
  Esto permite que si el usuario escribe `5.2` en $K_p$, el slider amplíe automáticamente su recorrido hasta `5.5` o `6.0` sin romperse ni desbordar.
- **Alternativas consideradas**:
  - *Quitar completamente los sliders*: Descartado, ya que el control deslizante proporciona ajuste táctil rápido muy útil en competencia.
  - *Slider logarítmico*: Puede ser poco intuitivo para ajustes finos en décimas de milésima.

### 2. Rotulado Semántico: Sensibilidad y Corrección
- **Decisión**: Modificar los encabezados de las tarjetas de sintonización y del modal de simulación para que muestren:
  - **Sensibilidad (Kp)**: Representa la fuerza reactiva proporcional inmediata al desviarse de la línea.
  - **Corrección (Kd)**: Representa el amortiguamiento derivativo para evitar sobrepasos y oscilaciones al corregir.
- **Alternativas consideradas**:
  - *Solo agregar texto en tooltip*: Se prefiere que la etiqueta esté visible directamente en la interfaz principal para lectura inmediata durante competencias.

### 3. Precisión Serial a 4 Decimales en Rust
- **Decisión**: En `SerialService::send_pid`, formatear tanto `kp` como `kd` con `{:.4}` en la cadena `$PID,{:.4},{:.4},...`.
- **Alternativas consideradas**:
  - *Mantener 2 o 3 decimales*: Provocaría truncamiento de las ganancias finas configuradas por el usuario.

## Risks / Trade-offs

- **[Riesgo] Usuario introduce valores excesivamente altos por error**:
  - *Mitigación*: En el firmware, la corrección del motor ya cuenta con saturación y acotamiento de velocidad entre `-brake_speed` y `+max_speed`. Además, el simulador en Svelte previene valores NaN o infinitos.
- **[Riesgo] Pérdida de resolución en el slider al expandirse demasiado**:
  - *Mitigación*: Si el usuario digita un valor alto y luego lo reduce a un valor estándar (ej. `0.24`), el slider recalcula y vuelve automáticamente al rango base estándar ($1.5$).

## Migration Plan

No requiere migraciones de base de datos ni modificaciones al firmware embebido. La actualización se despliega compilando la interfaz frontend y el backend Tauri.
