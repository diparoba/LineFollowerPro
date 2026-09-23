<script>
  import { createEventDispatcher, onMount, onDestroy } from 'svelte';

  export let isOpen = false;
  export let activeCategory = 'IM_16'; // 'IM_16' o 'CODEX_8'
  export let currentKp = 0.24;
  export let currentKd = 4.2;
  export let currentBaseSpeed = 180;
  export let isConnected = false;

  const dispatch = createEventDispatcher();

  // Parámetros de entrada físicos
  let voltage = 7.4; // Voltios (2S LiPo nominal)
  let sensorDistance = 140; // mm (Distancia centro sensor a eje de tracción)
  let wheelTrack = 160; // mm (Ancho entre ruedas)
  let baseSpeed = currentBaseSpeed || 180;
  let tuningProfile = 'balanced'; // 'conservative', 'balanced', 'aggressive'

  // Ganancias calculadas
  let calcKp = 0.24;
  let calcKd = 4.2;
  let calcBaseSpeed = 180;
  let calcMaxSpeed = 255;
  let calcBrakeSpeed = 130;

  // Métricas de simulación
  let overshootPercent = 0;
  let settlingTimeMs = 0;
  let stabilityStatus = 'ÓPTIMO'; // 'ESTABLE', 'ÓPTIMO', 'REACTIVO'

  let canvasElem = null;

  // Presets de voltaje típicos de robótica de competencia
  const voltagePresets = [
    { label: '2S LiPo Nominal (7.4V)', value: 7.4 },
    { label: '2S LiPo Llena (8.4V)', value: 8.4 },
    { label: '3S LiPo Nominal (11.1V)', value: 11.1 },
    { label: '3S LiPo Llena (12.6V)', value: 12.6 },
    { label: 'Regulado 5V', value: 5.0 }
  ];

  $: if (isOpen) {
    baseSpeed = currentBaseSpeed || 180;
    recalculateGains();
  }

  $: {
    voltage;
    sensorDistance;
    wheelTrack;
    baseSpeed;
    activeCategory;
    tuningProfile;
    recalculateGains();
  }

  function recalculateGains() {
    const v = Math.max(4.5, Math.min(14.0, Number(voltage) || 7.4));
    const L = Math.max(60, Math.min(260, Number(sensorDistance) || 140));
    const W = Math.max(90, Math.min(260, Number(wheelTrack) || 160));
    const spd = Math.max(80, Math.min(255, Number(baseSpeed) || 180));

    // 1. Compensación por Tensión de Alimentación (referencia 7.4V)
    // A mayor voltaje, el motor tiene mayor torque y RPM por PWM; requiere menor ganancia proporcional
    const voltageFactor = 7.4 / v;

    // 2. Factores Geométricos
    // Brazo sensor-eje: a mayor L, mayor anticipación lineal (requiere mayor amortiguamiento Kd)
    const lengthFactor = L / 140.0;
    // Ancho de vía: a mayor W, mayor momento de inercia y palanca diferencial
    const trackFactor = 160.0 / W;

    // 3. Ganancias Base según Categoría
    let baseKp = activeCategory === 'CODEX_8' ? 0.35 : 0.24;
    let baseKd = activeCategory === 'CODEX_8' ? 5.0 : 4.2;

    // 4. Modificadores por Perfil Dinámico (Factor de Amortiguamiento)
    let styleP = 1.0;
    let styleD = 1.0;
    if (tuningProfile === 'conservative') {
      styleP = 0.82;
      styleD = 1.25;
      stabilityStatus = 'ALTA ESTABILIDAD (Suave)';
    } else if (tuningProfile === 'aggressive') {
      styleP = 1.22;
      styleD = 0.88;
      stabilityStatus = 'REACTIVO (Competencia)';
    } else {
      styleP = 1.0;
      styleD = 1.0;
      stabilityStatus = 'EQUILIBRADO (Óptimo)';
    }

    // 5. Ajuste por Velocidad Lineal
    const speedRatio = spd / 180.0;
    const speedKdBonus = 1.0 + (speedRatio - 1.0) * 0.35;

    // Cálculo final
    calcKp = Number((baseKp * voltageFactor * trackFactor * styleP).toFixed(4));
    calcKd = Number((baseKd * voltageFactor * lengthFactor * speedKdBonus * styleD).toFixed(2));

    // Límites de seguridad
    calcKp = Math.max(0.08, Math.min(0.95, calcKp));
    calcKd = Math.max(1.0, Math.min(18.0, calcKd));

    calcBaseSpeed = spd;
    calcMaxSpeed = Math.min(255, Math.round(spd * 1.35));
    calcBrakeSpeed = Math.max(60, Math.round(spd * 0.65));

    // Simular respuesta al escalón en el canvas
    simulateStepResponse(calcKp, calcKd, spd, v, L, W);
  }

  function simulateStepResponse(kp, kd, spd, v, L, W) {
    if (!canvasElem) return;
    const ctx = canvasElem.getContext('2d');
    const width = canvasElem.width;
    const height = canvasElem.height;

    ctx.clearRect(0, 0, width, height);

    // Fondo oscuro
    ctx.fillStyle = '#090d16';
    ctx.fillRect(0, 0, width, height);

    const padLeft = 45;
    const padRight = 20;
    const padTop = 20;
    const padBottom = 30;
    const plotW = width - padLeft - padRight;
    const plotH = height - padTop - padBottom;
    const midY = padTop + plotH / 2;

    // Simulación física discretizada a 1000 Hz por 1.2 segundos
    const dt = 0.001; // 1 ms
    const totalSteps = 600;
    let error = 1000.0; // Perturbación inicial de 1000 pts de error
    let lastError = error;
    let theta = 0.0; // Ángulo de orientación
    const trajectory = [];

    let peakError = 0;
    let settledIndex = -1;

    for (let k = 0; k < totalSteps; k++) {
      const pTerm = kp * error;
      const dTerm = kd * (error - lastError) / dt * 0.001;
      const correction = pTerm + dTerm;

      // Dinámica de giro de ruedas
      const vLinear = (spd / 255.0) * 2.2 * (v / 7.4); // m/s estimado
      const omega = (correction * 0.012) / (W * 0.001); // rad/s

      theta += omega * dt;
      error -= (vLinear * Math.sin(theta) * 1000.0) * (L / 140.0) * dt * 45.0;

      trajectory.push(error);

      // Detección de sobrepaso negativo (overshoot)
      if (error < 0 && Math.abs(error) > peakError) {
        peakError = Math.abs(error);
      }

      // Tiempo de estabilización dentro del 5% (banda de ±50 pts)
      if (Math.abs(error) <= 50 && settledIndex === -1 && k > 50) {
        settledIndex = k;
      }

      lastError = error;
    }

    overshootPercent = Math.round((peakError / 1000.0) * 100);
    settlingTimeMs = settledIndex !== -1 ? settledIndex : totalSteps;

    // Cuadrícula y referencias
    ctx.strokeStyle = '#1e293b';
    ctx.lineWidth = 1;

    // Línea Setpoint (cero error)
    ctx.strokeStyle = 'rgba(56, 189, 248, 0.4)';
    ctx.setLineDash([4, 4]);
    ctx.beginPath();
    ctx.moveTo(padLeft, midY);
    ctx.lineTo(width - padRight, midY);
    ctx.stroke();
    ctx.setLineDash([]);

    // Etiquetas
    ctx.font = '10px JetBrains Mono, monospace';
    ctx.fillStyle = '#64748b';
    ctx.textAlign = 'right';
    ctx.fillText('0', padLeft - 6, midY + 3);
    ctx.fillText('+1000', padLeft - 6, padTop + 10);
    ctx.fillText('-500', padLeft - 6, height - padBottom - 5);

    ctx.textAlign = 'center';
    ctx.fillText('0 ms', padLeft, height - padBottom + 16);
    ctx.fillText('300 ms', padLeft + plotW * 0.5, height - padBottom + 16);
    ctx.fillText('600 ms', width - padRight, height - padBottom + 16);

    // Dibujar curva de respuesta
    ctx.strokeStyle = overshootPercent < 18 ? '#22c55e' : overshootPercent < 35 ? '#f59e0b' : '#ef4444';
    ctx.lineWidth = 2.4;
    ctx.beginPath();

    const maxScale = 1200;
    for (let i = 0; i < trajectory.length; i++) {
      const x = padLeft + (i / totalSteps) * plotW;
      const clamped = Math.max(-600, Math.min(maxScale, trajectory[i]));
      const y = midY - (clamped / maxScale) * (plotH / 2);

      if (i === 0) ctx.moveTo(x, y);
      else ctx.lineTo(x, y);
    }
    ctx.stroke();
  }

  function handleApply() {
    dispatch('applyGains', {
      kp: calcKp,
      kd: calcKd,
      baseSpeed: calcBaseSpeed,
      maxSpeed: calcMaxSpeed,
      brakeSpeed: calcBrakeSpeed
    });
    close();
  }

  function close() {
    dispatch('close');
  }

  $: if (isOpen && canvasElem) {
    setTimeout(recalculateGains, 30);
  }
</script>

{#if isOpen}
  <div 
    class="modal-backdrop" 
    on:click|self={close}
    on:keydown={(e) => e.key === 'Escape' && close()}
    role="dialog"
    aria-modal="true"
    tabindex="-1"
  >
    <div class="modal-container precision-card">
      <!-- Encabezado Modal -->
      <div class="modal-header">
        <div class="header-brand">
          <div class="autotune-icon">🎯</div>
          <div>
            <h2>Asistente de Auto-Sintonización PID Analítica</h2>
            <p class="subtitle">
              Calcula matemáticamente las ganancias óptimas de Kp y Kd según el chasis, voltaje de batería y velocidad
            </p>
          </div>
        </div>
        <button class="close-btn" on:click={close} title="Cerrar modal">✕</button>
      </div>

      <div class="modal-body">
        <!-- Columna Izquierda: Entradas Físicas -->
        <div class="inputs-column">
          <div class="section-box">
            <span class="box-title">1. ALIMENTACIÓN ELÉCTRICA</span>
            
            <div class="field-group">
              <label for="voltageInput">Voltaje de Batería (V):</label>
              <div class="input-with-presets">
                <input 
                  id="voltageInput" 
                  type="number" 
                  step="0.1" 
                  min="4.5" 
                  max="14.0" 
                  bind:value={voltage} 
                  class="precision-mono"
                />
                <span class="unit-tag">Voltios</span>
              </div>
            </div>

            <div class="preset-buttons">
              {#each voltagePresets as vp}
                <button 
                  class="preset-chip {voltage === vp.value ? 'active' : ''}" 
                  on:click={() => voltage = vp.value}
                >
                  {vp.label}
                </button>
              {/each}
            </div>
            <p class="field-hint">
              💡 A mayor voltaje, los motores tienen más par. La ganancia Kp se atenúa proporcionalmente para evitar oscilaciones violentas.
            </p>
          </div>

          <div class="section-box">
            <span class="box-title">2. GEOMETRÍA DEL CHASIS</span>
            
            <div class="field-group">
              <div class="flex-label">
                <label for="distInput">Distancia Barra Sensor a Eje (L):</label>
                <span class="val-badge precision-mono">{sensorDistance} mm</span>
              </div>
              <input 
                id="distInput" 
                type="range" 
                min="70" 
                max="220" 
                step="5" 
                bind:value={sensorDistance} 
              />
            </div>

            <div class="field-group">
              <div class="flex-label">
                <label for="trackInput">Ancho de Vía entre Ruedas (W):</label>
                <span class="val-badge precision-mono">{wheelTrack} mm</span>
              </div>
              <input 
                id="trackInput" 
                type="range" 
                min="100" 
                max="220" 
                step="5" 
                bind:value={wheelTrack} 
              />
            </div>
          </div>

          <div class="section-box">
            <span class="box-title">3. CRUCERO & ESTILO DINÁMICO</span>

            <div class="field-group">
              <div class="flex-label">
                <label for="spdInput">Velocidad Base Deseada:</label>
                <span class="val-badge precision-mono">{baseSpeed} PWM</span>
              </div>
              <input 
                id="spdInput" 
                type="range" 
                min="100" 
                max="240" 
                step="5" 
                bind:value={baseSpeed} 
              />
            </div>

            <div class="profile-selector">
              <button 
                class="style-btn {tuningProfile === 'conservative' ? 'active' : ''}" 
                on:click={() => tuningProfile = 'conservative'}
              >
                🛡️ Conservador
              </button>
              <button 
                class="style-btn {tuningProfile === 'balanced' ? 'active' : ''}" 
                on:click={() => tuningProfile = 'balanced'}
              >
                ⚖️ Equilibrado
              </button>
              <button 
                class="style-btn {tuningProfile === 'aggressive' ? 'active' : ''}" 
                on:click={() => tuningProfile = 'aggressive'}
              >
                ⚡ Agresivo
              </button>
            </div>
          </div>
        </div>

        <!-- Columna Derecha: Resultados y Simulación de Respuesta al Escalón -->
        <div class="results-column">
          <div class="results-card precision-card">
            <div class="results-header">
              <span class="calc-badge">RESULTADOS RECOMENDADOS</span>
              <span class="status-indicator precision-mono {overshootPercent < 20 ? 'good' : 'warn'}">
                ● {stabilityStatus}
              </span>
            </div>

            <div class="gains-grid">
              <div class="gain-box primary">
                <span class="g-name">Proporcional (Kp)</span>
                <span class="g-val precision-mono">{calcKp.toFixed(4)}</span>
                <span class="g-sub">Sugerido (Actual: {currentKp.toFixed(4)})</span>
              </div>

              <div class="gain-box primary">
                <span class="g-name">Derivativo (Kd)</span>
                <span class="g-val precision-mono">{calcKd.toFixed(2)}</span>
                <span class="g-sub">Sugerido (Actual: {currentKd.toFixed(2)})</span>
              </div>

              <div class="gain-box">
                <span class="g-name">Velocidad Base</span>
                <span class="g-val precision-mono">{calcBaseSpeed}</span>
                <span class="g-sub">Crucero en rectas</span>
              </div>

              <div class="gain-box">
                <span class="g-name">Velocidad Máxima</span>
                <span class="g-val precision-mono">{calcMaxSpeed}</span>
                <span class="g-sub">Límite de saturación</span>
              </div>

              <div class="gain-box">
                <span class="g-name">Freno Dinámico</span>
                <span class="g-val precision-mono">{calcBrakeSpeed}</span>
                <span class="g-sub">Intensidad en curvas</span>
              </div>

              <div class="gain-box">
                <span class="g-name">Categoría Activa</span>
                <span class="g-val precision-mono">{activeCategory === 'CODEX_8' ? '8L CODEX' : '16L IM'}</span>
                <span class="g-sub">Escala de setpoint aplicada</span>
              </div>
            </div>
          </div>

          <!-- Gráfica de Simulación de Respuesta -->
          <div class="sim-card precision-card">
            <div class="sim-header">
              <span class="sim-title">Previsualización Dinámica: Respuesta al Escalón (1000 pts)</span>
              <div class="metrics-row precision-mono">
                <span>Sobreimpulso: <strong>{overshootPercent}%</strong></span>
                <span>Estabilización: <strong>{settlingTimeMs} ms</strong></span>
              </div>
            </div>

            <div class="canvas-box">
              <canvas 
                bind:this={canvasElem}
                width={560}
                height={200}
              ></canvas>
            </div>

            <p class="sim-desc">
              Curva teórica de recuperación ante una curva abrupta de 90° o chicane. La zona verde indica rápida convergencia al centro de la línea sin pérdida de adherencia.
            </p>
          </div>

          <!-- Botones de Acción -->
          <div class="actions-row">
            <button class="precision-btn btn-cancel" on:click={close}>
              Cancelar
            </button>
            <button class="precision-btn btn-apply" on:click={handleApply}>
              {isConnected ? '🚀 Aplicar Ganancias y Enviar al Robot (En Vivo)' : '🚀 Cargar Ganancias a la Suite'}
            </button>
          </div>
        </div>
      </div>
    </div>
  </div>
{/if}

<style>
  .modal-backdrop {
    position: fixed;
    top: 0;
    left: 0;
    width: 100vw;
    height: 100vh;
    background: rgba(0, 0, 0, 0.78);
    backdrop-filter: blur(5px);
    display: flex;
    justify-content: center;
    align-items: center;
    z-index: 10000;
    padding: 1rem;
    animation: fadeIn 0.18s ease-out;
  }

  @keyframes fadeIn {
    from { opacity: 0; transform: scale(0.98); }
    to { opacity: 1; transform: scale(1); }
  }

  .modal-container {
    width: 100%;
    max-width: 1080px;
    max-height: 92vh;
    background: var(--bg-surface);
    border: 1px solid var(--border-medium);
    border-radius: 12px;
    display: flex;
    flex-direction: column;
    overflow-y: auto;
    box-shadow: 0 20px 50px rgba(0, 0, 0, 0.55);
  }

  .modal-header {
    display: flex;
    justify-content: space-between;
    align-items: center;
    padding: 1rem 1.25rem;
    border-bottom: 1px solid var(--border-subtle);
    background: var(--bg-subtle);
  }

  .header-brand {
    display: flex;
    align-items: center;
    gap: 0.75rem;
  }

  .autotune-icon {
    font-size: 1.5rem;
    line-height: 1;
  }

  h2 {
    font-size: 1.05rem;
    font-weight: 700;
    margin: 0;
    color: var(--text-primary);
  }

  .subtitle {
    font-size: 0.76rem;
    color: var(--text-secondary);
    margin: 0.15rem 0 0 0;
  }

  .close-btn {
    background: transparent;
    border: none;
    color: var(--text-secondary);
    font-size: 1.1rem;
    cursor: pointer;
    padding: 0.25rem 0.5rem;
    border-radius: 4px;
  }

  .close-btn:hover {
    color: var(--text-primary);
    background: var(--bg-hover);
  }

  .modal-body {
    display: grid;
    grid-template-columns: 420px 1fr;
    gap: 1.25rem;
    padding: 1.25rem;
  }

  @media (max-width: 900px) {
    .modal-body {
      grid-template-columns: 1fr;
    }
  }

  .inputs-column {
    display: flex;
    flex-direction: column;
    gap: 1rem;
  }

  .section-box {
    background: var(--bg-card);
    border: 1px solid var(--border-subtle);
    border-radius: 8px;
    padding: 0.85rem;
    display: flex;
    flex-direction: column;
    gap: 0.65rem;
  }

  .box-title {
    font-size: 0.68rem;
    font-weight: 800;
    color: var(--text-secondary);
    letter-spacing: 0.5px;
  }

  .field-group {
    display: flex;
    flex-direction: column;
    gap: 0.35rem;
  }

  .field-group label {
    font-size: 0.76rem;
    font-weight: 600;
    color: var(--text-primary);
  }

  .flex-label {
    display: flex;
    justify-content: space-between;
    align-items: center;
  }

  .val-badge {
    font-size: 0.76rem;
    font-weight: 700;
    color: #38bdf8;
  }

  .input-with-presets {
    display: flex;
    align-items: center;
    gap: 0.45rem;
  }

  .input-with-presets input {
    width: 100px;
    padding: 0.35rem 0.5rem;
    font-size: 0.85rem;
    font-weight: 700;
    background: var(--bg-surface);
    border: 1px solid var(--border-medium);
    border-radius: 6px;
    color: var(--text-primary);
  }

  .unit-tag {
    font-size: 0.75rem;
    color: var(--text-secondary);
  }

  .preset-buttons {
    display: flex;
    flex-wrap: wrap;
    gap: 0.35rem;
  }

  .preset-chip {
    font-size: 0.68rem;
    padding: 0.25rem 0.5rem;
    border-radius: 4px;
    background: var(--bg-subtle);
    border: 1px solid var(--border-subtle);
    color: var(--text-secondary);
    cursor: pointer;
    transition: all 0.15s ease;
  }

  .preset-chip:hover {
    border-color: var(--border-highlight);
    color: var(--text-primary);
  }

  .preset-chip.active {
    background: rgba(56, 189, 248, 0.15);
    border-color: #38bdf8;
    color: #38bdf8;
    font-weight: 700;
  }

  .field-hint {
    font-size: 0.68rem;
    color: var(--text-secondary);
    margin: 0;
    line-height: 1.35;
  }

  input[type="range"] {
    width: 100%;
    accent-color: #0284c7;
    cursor: pointer;
  }

  .profile-selector {
    display: grid;
    grid-template-columns: 1fr 1fr 1fr;
    gap: 0.35rem;
    margin-top: 0.25rem;
  }

  .style-btn {
    font-size: 0.72rem;
    padding: 0.4rem 0.2rem;
    border-radius: 6px;
    background: var(--bg-subtle);
    border: 1px solid var(--border-subtle);
    color: var(--text-secondary);
    cursor: pointer;
    font-weight: 600;
    transition: all 0.15s ease;
  }

  .style-btn:hover {
    border-color: var(--border-highlight);
    color: var(--text-primary);
  }

  .style-btn.active {
    background: #0284c7;
    border-color: #0284c7;
    color: #ffffff;
    font-weight: 700;
  }

  /* Columna Derecha */
  .results-column {
    display: flex;
    flex-direction: column;
    gap: 1rem;
  }

  .results-card {
    background: var(--bg-card);
    border: 1px solid var(--border-medium);
    border-radius: 8px;
    padding: 0.85rem;
  }

  .results-header {
    display: flex;
    justify-content: space-between;
    align-items: center;
    margin-bottom: 0.75rem;
  }

  .calc-badge {
    font-size: 0.66rem;
    font-weight: 800;
    background: #6366f1;
    color: #fff;
    padding: 0.2rem 0.5rem;
    border-radius: 4px;
    letter-spacing: 0.5px;
  }

  .status-indicator {
    font-size: 0.75rem;
    font-weight: 700;
  }

  .status-indicator.good { color: #22c55e; }
  .status-indicator.warn { color: #f59e0b; }

  .gains-grid {
    display: grid;
    grid-template-columns: 1fr 1fr;
    gap: 0.55rem;
  }

  .gain-box {
    background: var(--bg-subtle);
    border: 1px solid var(--border-subtle);
    border-radius: 6px;
    padding: 0.5rem 0.65rem;
    display: flex;
    flex-direction: column;
  }

  .gain-box.primary {
    border-color: rgba(56, 189, 248, 0.4);
    background: rgba(56, 189, 248, 0.05);
  }

  .g-name {
    font-size: 0.66rem;
    font-weight: 700;
    color: var(--text-secondary);
  }

  .g-val {
    font-size: 1.15rem;
    font-weight: 800;
    color: #38bdf8;
    margin: 0.15rem 0;
  }

  .gain-box.primary .g-val {
    color: #0284c7;
    font-size: 1.25rem;
  }

  .g-sub {
    font-size: 0.65rem;
    color: var(--text-secondary);
  }

  /* Sim Card */
  .sim-card {
    background: var(--bg-card);
    border: 1px solid var(--border-subtle);
    border-radius: 8px;
    padding: 0.85rem;
    display: flex;
    flex-direction: column;
    gap: 0.5rem;
  }

  .sim-header {
    display: flex;
    justify-content: space-between;
    align-items: center;
    flex-wrap: wrap;
    gap: 0.45rem;
  }

  .sim-title {
    font-size: 0.75rem;
    font-weight: 700;
    color: var(--text-primary);
  }

  .metrics-row {
    display: flex;
    gap: 0.75rem;
    font-size: 0.72rem;
    color: var(--text-secondary);
  }

  .metrics-row strong {
    color: var(--text-primary);
  }

  .canvas-box {
    width: 100%;
    background: #090d16;
    border-radius: 6px;
    overflow: hidden;
  }

  canvas {
    width: 100%;
    height: auto;
    display: block;
  }

  .sim-desc {
    font-size: 0.68rem;
    color: var(--text-secondary);
    margin: 0;
    line-height: 1.35;
  }

  .actions-row {
    display: flex;
    justify-content: flex-end;
    gap: 0.75rem;
    margin-top: 0.5rem;
  }

  .btn-cancel {
    background: var(--bg-subtle);
    border: 1px solid var(--border-subtle);
    color: var(--text-secondary);
    font-size: 0.78rem;
    padding: 0.45rem 0.95rem;
  }

  .btn-apply {
    background: #059669;
    color: #ffffff;
    font-size: 0.82rem;
    font-weight: 700;
    padding: 0.5rem 1.15rem;
    box-shadow: 0 4px 12px rgba(5, 150, 105, 0.3);
  }

  .btn-apply:hover {
    background: #047857;
  }
</style>
