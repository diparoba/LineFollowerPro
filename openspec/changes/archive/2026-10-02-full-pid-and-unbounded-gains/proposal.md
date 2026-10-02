# Proposal: Ganancias PD Ilimitadas a 4 Decimales con Etiquetas de Sensibilidad y Corrección

## Why

Los competidores y desarrolladores de robots seguidores de línea de alta velocidad requieren ajustar sus ganancias de control con resolución ultrafina sin topar con restricciones artificiales en la interfaz gráfica (como los topes anteriores de Kp=1.0 o Kd=15.0). El lazo de control debe permanecer puramente PD (Proporcional-Derivativo), ya que la acción integral ($K_i$) no es necesaria y resulta contraproducente en seguidores veloces al introducir demoras de fase y sobre-oscilación en la salida de chicanas. Asimismo, para facilitar la sintonización intuitiva en boxes y pista, los parámetros deben identificarse claramente por su efecto físico real: **Sensibilidad** para $K_p$ y **Corrección** para $K_d$, con precisión de hasta 4 decimales (`0.0001`), límite inferior en $0$ y sin límite superior.

## What Changes

- **Entrada Numérica Ilimitada a 4 Decimales**: En `TuningPanel.svelte` y `SimulationModal.svelte`, las cajas de texto numéricas permitirán ingresar cualquier valor mayor o igual a 0 con paso de 0.0001 (`step="0.0001"`), eliminando los topes máximos fijos.
- **Sliders Auto-Escalables**: Las barras deslizantes de la interfaz mantendrán un rango cómodo inicial (ej. 0 a 1.5 para Kp, 0 a 20 para Kd), pero se adaptarán dinámicamente si el usuario digita un valor mayor (`max={Math.max(baseMax, Math.ceil(val))}`).
- **Etiquetas Semánticas de Control**:
  - $K_p$ estará claramente etiquetado como **Sensibilidad (Kp)** con indicador explicativo de respuesta inmediata al error.
  - $K_d$ estará claramente etiquetado como **Corrección (Kd)** con indicador explicativo de amortiguamiento y estabilidad dinámica.
- **Preservación de Firmware y Persistencia**:
  - Se mantiene la estructura óptima `PDController` y `RobotConfig` en el firmware (sin overhead ni variables innecesarias de Ki).
  - La trama serial `$PID,kp,kd,base,max,brake,fork,color` se mantiene estándar y transparente, formateando las ganancias a 4 decimales (`{:.4}`).
  - La base de datos SQLite y memoria EEPROM física conservan su esquema validado sin migraciones disruptivas.

## Capabilities

### New Capabilities
- `full-pid-and-unbounded-gains`: Sintonización PD ilimitada a 4 decimales con etiquetas intuitivas de Sensibilidad y Corrección en la interfaz gráfica y simulador.

## Impact

- **Frontend Svelte (`frontend/src/lib/`)**:
  - `TuningPanel.svelte`: Entradas a 4 decimales, remoción de `max`, sliders adaptativos y etiquetas de Sensibilidad y Corrección.
  - `SimulationModal.svelte`: Entradas numéricas a 4 decimales, sliders dinámicos y etiquetas actualizadas.
  - `ProfileManager.svelte`: Formato visual de 4 decimales en las tarjetas de perfiles.
- **Backend Rust (`frontend/src-tauri/`)**:
  - `serial.rs`: Formateo de trama serial `$PID` con precisión de 4 decimales (`{:.4}`).
