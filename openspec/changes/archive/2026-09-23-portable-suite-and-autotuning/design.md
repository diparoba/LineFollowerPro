# Design: Suite Portable Definitiva & Asistente de Auto-Sintonización PID

## Context

Véase `proposal.md` para la motivación. Actualmente, la aplicación corre de forma autónoma pero abre enlaces web externos para la descarga de drivers y depende del directorio de trabajo actual para abrir `follower.db`. Adicionalmente, el ajuste de ganancias $K_p$ y $K_d$ se realiza mediante prueba y error manual en los sliders sin una guía analítica que relacione la geometría del robot y el voltaje de alimentación de la batería.

## Goals / Non-Goals

**Goals:**
- Empaquetar y ejecutar `CH341SER.EXE` localmente desde `resources/drivers/` con elevación de permisos UAC de Windows.
- Garantizar que la base de datos `follower.db` se ancle de manera estricta e invariable junto al ejecutable en cualquier memoria flash USB.
- Proveer un comando nativo en Rust y botón en la UI para respaldar `follower.db` en un clic.
- Diseñar un modelo analítico de estimación de ganancias $K_p$ y $K_d$ basado en geometría física ($L$, $W$), velocidad crucero ($V_{\text{base}}$), categoría (16L vs 8L) y voltaje de batería ($V_{\text{in}}$).
- Proveer un simulador interactivo de respuesta al escalón que anticipe el sobreimpulso y la estabilidad antes de transferir las ganancias a la memoria EEPROM.

**Non-Goals:**
- Generar instaladores MSI que requieran privilegios de administrador para instalar software en `C:\Program Files` (se mantiene el modelo de portabilidad absoluta).
- Auto-tuning en circuito cerrado físico autónomo (el robot no requiere algoritmos de perturbación en pista; el cálculo es analítico y asistido en la app).

## Decisions

### 1. Ejecución Local de Drivers con Elevación UAC en Rust
- **Decisión:** En `flasher.rs`, implementar la función `install_driver` para ubicar el ejecutable local en `resources/drivers/CH341SER.EXE` y lanzarlo mediante PowerShell:
  ```powershell
  Start-Process -FilePath "<path_to_driver>" -Verb RunAs
  ```
- **Razón:** Invoca la ventana nativa de UAC de Windows sin bloquear el hilo principal de la aplicación Tauri.
- **Alternativas consideradas:**
  - *Lanzar directamente `Command::new(driver_path)`*: Falla en Windows si el instalador requiere privilegios elevados y la app se ejecuta como usuario estándar. `Start-Process ... -Verb RunAs` garantiza la elevación limpia.

### 2. Anclaje Determinista de la Base de Datos a `current_exe()`
- **Decisión:** En `lib.rs`, resolver la ruta de `follower.db` usando:
  ```rust
  let exe_dir = std::env::current_exe()
      .ok()
      .and_then(|p| p.parent().map(|p| p.to_path_buf()))
      .unwrap_or_else(|| std::env::current_dir().unwrap_or_default());
  let db_path = exe_dir.join("follower.db");
  ```
- **Razón:** Garantiza que al conectar el pendrive en la letra `E:\` o `F:\` y lanzar la app desde cualquier contexto, `follower.db` siempre se abra en la flash USB.

### 3. Función de Respaldo Rápido de Base de Datos
- **Decisión:** Implementar el comando Tauri `backup_database()`, el cual genera una copia fiel del archivo SQLite con el patrón `backup_follower_YYYY-MM-DD_HHmmss.db` en el mismo directorio del ejecutable.
- **Razón:** Facilita a los competidores crear puntos de restauración antes de una ronda eliminatoria sin necesidad de abrir el Explorador de Windows.

### 4. Modelo Analítico de Auto-Sintonización PID
- **Decisión:** Modelar el seguidor de línea como un sistema de segundo orden amortiguado accionado por dirección diferencial:
  - **Sensibilidad de Voltaje ($S_V$):** La velocidad y el par de los motores DC sin núcleo o con reductora escalan con el voltaje de la batería. Se toma como referencia un voltaje nominal de 7.4V (2S LiPo). El factor de compensación de ganancia es inversamente proporcional a la tensión:
    $$\gamma_V = \frac{7.4}{V_{\text{in}}}$$
  - **Brazo de Palanca Geométrico ($L$ y $W$):**
    - Un mayor largo $L$ (distancia sensor a eje) detecta la curva con anticipación lineal, requiriendo un amortiguamiento derivativo $K_d$ superior para evitar oscilaciones antes del vértice.
    - Un ancho $W$ (trocha) mayor produce mayor momento de giro para una diferencia de PWM dada.
  - **Perfil de Amortiguamiento ($\zeta$):**
    - *Conservador:* $\zeta \approx 1.0$ (sobreamortiguado, sin sobrepaso).
    - *Equilibrado:* $\zeta \approx 0.707$ (óptimo de Butterworth, mínima integral de error cuadrático).
    - *Agresivo:* $\zeta \approx 0.5$ (subamortiguado de competencia, máxima rapidez en chicane).
  - **Escala de Categoría:**
    - Para 16L ($e_{\text{max}} = 7500$), las ganancias se calculan en base a la dinámica completa.
    - Para 8L ($e_{\text{max}} = 3500$), el error es numéricamente menor en un 53%, por lo que la ganancia proporcional se escala con factor $\approx 1.45$.

## Risks / Trade-offs

- **[Riesgo] El usuario ejecuta la app en un directorio de solo lectura en Windows**:
  - *Mitigación:* Si la unidad flash tiene switch de protección contra escritura o permisos restringidos, el intento de escritura en SQLite informará un error descriptivo en la interfaz en lugar de cerrarse abruptamente.
- **[Riesgo] El usuario ingresa un voltaje erróneo en el asistente**:
  - *Mitigación:* Se colocan límites seguros validados (rango de 5.0V a 13.0V) con selector predefinido de opciones típicas de robótica (2S LiPo 7.4V, 2S LiPo Full 8.4V, 3S LiPo 11.1V, 5V Regulado).
