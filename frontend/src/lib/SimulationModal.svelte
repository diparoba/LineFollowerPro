<script>
  import { createEventDispatcher, onMount, onDestroy } from 'svelte';

  export let isOpen = false;
  export let category = 'IM_16'; // 'IM_16' o 'CODEX_8'
  export let isConnected = false;
  export let realTelemetryRaw = [];
  export let currentKp = 0.24;
  export let currentKd = 4.2;
  export let currentBaseSpeed = 180;
  export let currentMaxSpeed = 255;
  export let currentBrakeSpeed = 130;

  const dispatch = createEventDispatcher();

  // Modo de fuente de datos: 'hardware' (sensores reales conectados) o 'virtual' (pista simulada)
  let dataSource = 'hardware'; // por defecto hardware si está conectado

  // Variables locales para sintonización en el simulador
  let simKp = currentKp;
  let simKd = currentKd;
  let simBaseSpeed = currentBaseSpeed;
  let simMaxSpeed = currentMaxSpeed;
  let simBrakeSpeed = currentBrakeSpeed;

  $: if (isOpen) {
    simKp = currentKp;
    simKd = currentKd;
    simBaseSpeed = currentBaseSpeed;
    simMaxSpeed = currentMaxSpeed;
    simBrakeSpeed = currentBrakeSpeed;
    if (isConnected && realTelemetryRaw && realTelemetryRaw.length > 0) {
      dataSource = 'hardware';
    } else {
      dataSource = 'virtual';
    }
  }

  $: numSensors = category === 'CODEX_8' ? 8 : 16;
  $: setpoint = category === 'CODEX_8' ? 3500 : 7500;
  $: maxPos = category === 'CODEX_8' ? 7000 : 15000;

  let isSimRunning = true;
  let trackMode = 'chicanes'; // 'straight', 'chicanes', 'curve90', 'manual'
  let manualOffset = 0; // -100 a +100

  // Telemetría procesada en la simulación
  let simPosition = setpoint;
  let simError = 0;
  let simLastError = 0;
  let simPTerm = 0;
  let simDTerm = 0;
  let simCorrection = 0;
  let simLeftMotor = 0;
  let simRightMotor = 0;
  let simCurrentBase = simBaseSpeed;
  let simRawSensors = [];

  let simTime = 0;
  let animationFrameId = null;

  function close() {
    dispatch('close');
  }

  function applyToRobot() {
    dispatch('applySettings', {
      kp: simKp,
      kd: simKd,
      baseSpeed: simBaseSpeed,
      maxSpeed: simMaxSpeed,
      brakeSpeed: simBrakeSpeed
    });
    close();
  }

  function toggleSim() {
    isSimRunning = !isSimRunning;
  }

  function resetSim() {
    simTime = 0;
    simPosition = setpoint;
    simError = 0;
    simLastError = 0;
  }

  // Generador de posición de línea virtual según modo de pista
  function getVirtualLinePosition(time) {
    if (trackMode === 'manual') {
      return setpoint + (manualOffset / 100) * (setpoint * 0.95);
    } else if (trackMode === 'straight') {
      return setpoint + Math.sin(time * 2.5) * (setpoint * 0.12) + (Math.sin(time * 8.0) * (setpoint * 0.04));
    } else if (trackMode === 'chicanes') {
      return setpoint + Math.sin(time * 3.0) * (setpoint * 0.65) + Math.sin(time * 1.2) * (setpoint * 0.25);
    } else if (trackMode === 'curve90') {
      const mod = time % 6.0;
      if (mod < 2.0) return setpoint;
      if (mod < 4.0) return setpoint + (setpoint * 0.88);
      return setpoint - (setpoint * 0.88);
    }
    return setpoint;
  }

  // Simulación de reflectancia gaussiana offline
  function simulateSensorReadings(linePos) {
    const raw = [];
    const sigma = 800;

    for (let i = 0; i < numSensors; i++) {
      const sensorPos = i * 1000;
      const dist = Math.abs(sensorPos - linePos);
      const response = 950 * Math.exp(-(dist * dist) / (2 * sigma * sigma));
      raw.push(Math.round(Math.max(80, Math.min(1023, response))));
    }
    return raw;
  }

  // Bucle de física y control a 60 FPS
  function simulationLoop() {
    if (isOpen && isSimRunning) {
      simTime += 0.016;

      if (dataSource === 'hardware' && isConnected && realTelemetryRaw && realTelemetryRaw.length >= numSensors) {
        // --- MODO 1: SENSORES REALES EN VIVO (HARDWARE CONECTADO) ---
        simRawSensors = realTelemetryRaw.slice(0, numSensors);

        // Algoritmo Sub-Sensor de Centro de Masa con lecturas físicas reales
        let weightedSum = 0;
        let sum = 0;
        for (let i = 0; i < numSensors; i++) {
          const val = simRawSensors[i] || 0;
          // Normalización dinámica
          const norm = Math.max(0, Math.min(1000, ((val - 100) * 1000) / 850));
          weightedSum += norm * (i * 1000);
          sum += norm;
        }

        if (sum > 100) {
          simPosition = Math.round(weightedSum / sum);
        }
      } else {
        // --- MODO 2: PISTA VIRTUAL SINTÉTICA (OFFLINE) ---
        const actualLine = getVirtualLinePosition(simTime);
        simRawSensors = simulateSensorReadings(actualLine);

        let weightedSum = 0;
        let sum = 0;
        for (let i = 0; i < numSensors; i++) {
          const norm = Math.max(0, Math.min(1000, ((simRawSensors[i] - 120) * 1000) / 850));
          weightedSum += norm * (i * 1000);
          sum += norm;
        }

        if (sum > 80) {
          simPosition = Math.round(weightedSum / sum);
        }
      }

      // --- ALGORITMO PD IDÉNTICO AL FIRMWARE ---
      simError = simPosition - setpoint;
      simPTerm = simError * simKp;
      simDTerm = (simError - simLastError) * simKd;
      simCorrection = Math.round(simPTerm + simDTerm);
      simLastError = simError;

      // Reducción dinámica de velocidad en curvas cerradas
      const absErr = Math.abs(simError);
      simCurrentBase = simBaseSpeed;
      const curveThreshold = Math.round((maxPos * 2) / 15);
      const curveRange = maxPos - setpoint - curveThreshold;

      if (absErr > curveThreshold && curveRange > 0) {
        const speedReduction = Math.round(((absErr - curveThreshold) * (simBaseSpeed / 2)) / curveRange);
        simCurrentBase = Math.max(60, simBaseSpeed - speedReduction);
      }

      // Cálculo diferencial de los motores virtuales
      let left = simCurrentBase - simCorrection;
      let right = simCurrentBase + simCorrection;

      left = Math.min(simMaxSpeed, Math.max(-simBrakeSpeed, left));
      right = Math.min(simMaxSpeed, Math.max(-simBrakeSpeed, right));

      simLeftMotor = left;
      simRightMotor = right;
    }

    if (isOpen) {
      animationFrameId = requestAnimationFrame(simulationLoop);
    }
  }

  onMount(() => {
    if (isOpen) animationFrameId = requestAnimationFrame(simulationLoop);
  });

  $: if (isOpen) {
    if (!animationFrameId) animationFrameId = requestAnimationFrame(simulationLoop);
  } else {
    if (animationFrameId) {
      cancelAnimationFrame(animationFrameId);
      animationFrameId = null;
    }
  }

  onDestroy(() => {
    if (animationFrameId) cancelAnimationFrame(animationFrameId);
  });
</script>

{#if isOpen}
  <div 
    class="modal-backdrop" 
    on:click|self={close}
    on:keydown={(e) => { if (e.key === 'Escape') close(); }}
    role="dialog"
    aria-modal="true"
    tabindex="-1"
  >
    <div class="modal-container precision-card">
      <!-- Encabezado Modal -->
      <div class="modal-header">
        <div class="header-brand">
          <div class="sim-icon">⚡</div>
          <div>
            <h2>Simulador de Arranque & Precalibración Dinámica</h2>
            <p class="subtitle">
              Mueve el robot con la mano sobre la línea para ver la reacción del PD sin activar los motores físicos
            </p>
          </div>
        </div>
        <div class="header-actions">
          <button class="precision-chip sim-status-chip {isSimRunning ? 'running' : 'paused'}" on:click={toggleSim}>
            <span class="status-dot"></span>
            {isSimRunning ? 'EN VIVO' : 'PAUSADO'}
          </button>
          <button class="close-btn" on:click={close}>✕</button>
        </div>
      </div>

      <!-- Barra de Selección de Fuente de Datos -->
      <div class="source-selector-bar">
        <span class="selector-label">Origen de Datos:</span>
        <div class="pill-group">
          <button 
            class="source-pill {dataSource === 'hardware' ? 'active' : ''}" 
            on:click={() => dataSource = 'hardware'}
            disabled={!isConnected}
          >
            🔴 Sensores Reales en Vivo {isConnected ? '(Robot Conectado)' : '(No Conectado)'}
          </button>

          <button 
            class="source-pill {dataSource === 'virtual' ? 'active' : ''}" 
            on:click={() => dataSource = 'virtual'}
          >
            💻 Pista Virtual (Simulación Offline)
          </button>
        </div>

        {#if dataSource === 'virtual'}
          <div class="track-pill-group">
            <button class="track-pill {trackMode === 'straight' ? 'active' : ''}" on:click={() => trackMode = 'straight'}>Recta</button>
            <button class="track-pill {trackMode === 'chicanes' ? 'active' : ''}" on:click={() => trackMode = 'chicanes'}>Chicanas</button>
            <button class="track-pill {trackMode === 'curve90' ? 'active' : ''}" on:click={() => trackMode = 'curve90'}>90°</button>
            <button class="track-pill {trackMode === 'manual' ? 'active' : ''}" on:click={() => trackMode = 'manual'}>Manual</button>
          </div>
        {/if}

        <button class="reset-btn precision-btn" on:click={resetSim}>↺ Reset</button>
      </div>

      {#if dataSource === 'hardware'}
        <div class="hardware-hint-banner">
          <span class="hint-icon">💡</span>
          <span>
            <strong>Modo Banco de Trabajo:</strong> El robot está enviando telemetría analógica real por USB. Desplaza el robot lentamente sobre una cinta o línea negra con la mano. Los motores físicos permanecen apagados, mientras que la pantalla calcula y muestra cómo reaccionaría el algoritmo PD en pista.
          </span>
        </div>
      {:else if trackMode === 'manual'}
        <div class="manual-control-row">
          <label for="manualSlider">Desplazamiento de Línea Virtual:</label>
          <input id="manualSlider" type="range" min="-100" max="100" bind:value={manualOffset} class="manual-slider" />
          <span class="precision-mono offset-val">{manualOffset > 0 ? `+${manualOffset}% (Izq)` : `${manualOffset}% (Der)`}</span>
        </div>
      {/if}

      <!-- Grid Principal -->
      <div class="sim-grid">
        <!-- Columna Izquierda: Sensores y Tracción -->
        <div class="sim-left">
          <!-- Vista de Pista y Posición Relativa -->
          <div class="virtual-track-box">
            <div class="track-header">
              <span>Posición Estimada por Centro de Masa</span>
              <span class="precision-mono">Pos: {simPosition} | Setpoint: {setpoint}</span>
            </div>
            <div class="track-stage">
              <div class="center-guideline"></div>
              <div 
                class="simulated-line" 
                style="left: {(1 - (simPosition / maxPos)) * 100}%"
              >
                <div class="line-glow"></div>
              </div>
              <div class="simulated-car">
                <div class="car-body">
                  <div class="car-wheel left-wheel {simLeftMotor < 0 ? 'rev' : ''}"></div>
                  <div class="car-chassis">
                    <span class="car-badge precision-mono">{category === 'CODEX_8' ? 'C8' : 'IM16'}</span>
                  </div>
                  <div class="car-wheel right-wheel {simRightMotor < 0 ? 'rev' : ''}"></div>
                </div>
                <div class="sensor-bar-head"></div>
              </div>
            </div>
          </div>

          <!-- Barra de Sensores (Física o Virtual) -->
          <div class="sim-sensor-box">
            <div class="sim-box-title">
              <span>
                {dataSource === 'hardware' ? 'Lecturas Físicas de Sensores (Hardware A0..)' : 'Lecturas de Sensores Simuladas'}
              </span>
              <span class="precision-mono">Error: {simError > 0 ? `+${simError} (Izq)` : `${simError} (Der)`}</span>
            </div>
            <div class="sim-sensor-grid" style="grid-template-columns: repeat({numSensors}, 1fr)">
              {#each Array.from({length: numSensors}, (_, k) => (numSensors - 1) - k) as i}
                <div class="sim-sensor-col">
                  <div class="sim-bar-val precision-mono">{simRawSensors[i] || 0}</div>
                  <div class="sim-bar-track">
                    <div 
                      class="sim-bar-fill {(simRawSensors[i] || 0) > 400 ? 'active' : ''}" 
                      style="height: {Math.min(100, ((simRawSensors[i] || 0) / 1023) * 100)}%"
                    ></div>
                  </div>
                  <div class="sim-sensor-name precision-mono">S{i}</div>
                </div>
              {/each}
            </div>
          </div>

          <!-- Tracción Estimada -->
          <div class="sim-motor-box">
            <div class="motor-col">
              <div class="motor-info">
                <span>Motor Izquierdo Estimado</span>
                <strong class="precision-mono {simLeftMotor < 0 ? 'rev' : ''}">{simLeftMotor} PWM</strong>
              </div>
              <div class="motor-meter">
                <div class="meter-fill {simLeftMotor < 0 ? 'rev' : 'fwd'}" style="width: {Math.min(100, (Math.abs(simLeftMotor) / 255) * 100)}%"></div>
              </div>
            </div>

            <div class="motor-col">
              <div class="motor-info">
                <span>Motor Derecho Estimado</span>
                <strong class="precision-mono {simRightMotor < 0 ? 'rev' : ''}">{simRightMotor} PWM</strong>
              </div>
              <div class="motor-meter">
                <div class="meter-fill {simRightMotor < 0 ? 'rev' : 'fwd'}" style="width: {Math.min(100, (Math.abs(simRightMotor) / 255) * 100)}%"></div>
              </div>
            </div>
          </div>
        </div>

        <!-- Columna Derecha: Análisis PD y Ajuste Rápido -->
        <div class="sim-right">
          <div class="math-breakdown-card">
            <h4>Análisis del Algoritmo PD en Tiempo Real</h4>
            <div class="math-metrics-grid">
              <div class="metric-item">
                <span class="m-label">Error Instantáneo (e)</span>
                <span class="m-val precision-mono {simError > 0 ? 'pos' : 'neg'}">{simError}</span>
              </div>
              <div class="metric-item">
                <span class="m-label">Término Proporcional (P)</span>
                <span class="m-val precision-mono">{simPTerm.toFixed(1)}</span>
              </div>
              <div class="metric-item">
                <span class="m-label">Término Derivativo (D)</span>
                <span class="m-val precision-mono">{simDTerm.toFixed(1)}</span>
              </div>
              <div class="metric-item highlight">
                <span class="m-label">Corrección Total (PD)</span>
                <span class="m-val precision-mono">{simCorrection}</span>
              </div>
              <div class="metric-item">
                <span class="m-label">Velocidad Base Dinámica</span>
                <span class="m-val precision-mono">{simCurrentBase} PWM</span>
              </div>
              <div class="metric-item">
                <span class="m-label">Diferencial (Izq - Der)</span>
                <span class="m-val precision-mono">{simLeftMotor - simRightMotor} PWM</span>
              </div>
            </div>
          </div>

          <div class="sim-tuner-card">
            <h4>Precalibración de Ganancias</h4>

            <div class="sim-slider-group">
              <div class="s-label">
                <label for="simKp">Proporcional (Kp)</label>
                <span class="precision-mono s-val">{simKp.toFixed(4)}</span>
              </div>
              <input id="simKp" type="range" min="0.05" max="1.0" step="0.005" bind:value={simKp} />
            </div>

            <div class="sim-slider-group">
              <div class="s-label">
                <label for="simKd">Derivativo (Kd)</label>
                <span class="precision-mono s-val">{simKd.toFixed(2)}</span>
              </div>
              <input id="simKd" type="range" min="0.5" max="15.0" step="0.1" bind:value={simKd} />
            </div>

            <div class="sim-slider-group">
              <div class="s-label">
                <label for="simBase">Velocidad Base</label>
                <span class="precision-mono s-val">{simBaseSpeed} PWM</span>
              </div>
              <input id="simBase" type="range" min="50" max="255" step="5" bind:value={simBaseSpeed} />
            </div>

            <div class="sim-slider-group">
              <div class="s-label">
                <label for="simBrake">Freno Activo</label>
                <span class="precision-mono s-val">{simBrakeSpeed} PWM</span>
              </div>
              <input id="simBrake" type="range" min="0" max="255" step="5" bind:value={simBrakeSpeed} />
            </div>

            <button class="precision-btn btn-apply" on:click={applyToRobot}>
              ✓ Transferir Ganancias al Panel Principal
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
    right: 0;
    bottom: 0;
    background: rgba(0, 0, 0, 0.75);
    backdrop-filter: blur(6px);
    z-index: 1000;
    display: flex;
    align-items: center;
    justify-content: center;
    padding: 1rem;
    animation: fadeIn 0.15s ease-out;
  }

  @keyframes fadeIn {
    from { opacity: 0; }
    to { opacity: 1; }
  }

  .modal-container {
    width: 100%;
    max-width: 980px;
    max-height: 92vh;
    background: var(--bg-card);
    border: 1px solid var(--border-highlight);
    border-radius: var(--radius-card);
    overflow-y: auto;
    padding: 1rem;
    display: flex;
    flex-direction: column;
    gap: 0.85rem;
  }

  .modal-header {
    display: flex;
    justify-content: space-between;
    align-items: center;
    border-bottom: 1px solid var(--border-subtle);
    padding-bottom: 0.6rem;
  }

  .header-brand {
    display: flex;
    align-items: center;
    gap: 0.65rem;
  }

  .sim-icon {
    font-size: 1.4rem;
    background: rgba(2, 132, 199, 0.15);
    border: 1px solid rgba(2, 132, 199, 0.4);
    color: var(--accent-cyan);
    width: 36px;
    height: 36px;
    display: flex;
    align-items: center;
    justify-content: center;
    border-radius: var(--radius-btn);
  }

  .modal-header h2 {
    margin: 0;
    font-size: 1.05rem;
    color: var(--text-heading);
    font-weight: 700;
  }

  .subtitle {
    margin: 0.1rem 0 0 0;
    font-size: 0.72rem;
    color: var(--text-secondary);
  }

  .header-actions {
    display: flex;
    align-items: center;
    gap: 0.5rem;
  }

  .sim-status-chip {
    cursor: pointer;
    border: 1px solid transparent;
  }

  .sim-status-chip.running {
    background: var(--chip-green-bg);
    border-color: var(--chip-green-border);
    color: var(--chip-green-text);
  }

  .sim-status-chip.paused {
    background: var(--bg-subtle);
    border-color: var(--border-subtle);
    color: var(--text-muted);
  }

  .status-dot {
    width: 5px;
    height: 5px;
    border-radius: 50%;
    background: currentColor;
  }

  .close-btn {
    background: var(--bg-subtle);
    border: 1px solid var(--border-subtle);
    color: var(--text-secondary);
    border-radius: var(--radius-btn);
    width: 28px;
    height: 28px;
    cursor: pointer;
    font-size: 0.85rem;
    display: flex;
    align-items: center;
    justify-content: center;
  }

  .source-selector-bar {
    display: flex;
    align-items: center;
    gap: 0.65rem;
    flex-wrap: wrap;
    background: var(--track-bg);
    padding: 0.5rem 0.75rem;
    border-radius: var(--radius-btn);
    border: 1px solid var(--border-subtle);
  }

  .selector-label {
    font-size: 0.75rem;
    font-weight: 700;
    color: var(--text-secondary);
  }

  .pill-group, .track-pill-group {
    display: flex;
    gap: 0.35rem;
  }

  .source-pill, .track-pill {
    font-family: var(--font-geo);
    background: var(--pill-bg);
    border: 1px solid var(--pill-border);
    color: var(--pill-text);
    padding: 0.3rem 0.65rem;
    border-radius: var(--radius-btn);
    font-size: 0.74rem;
    font-weight: 600;
    cursor: pointer;
    transition: all 0.15s ease;
  }

  .source-pill.active, .track-pill.active {
    background: var(--pill-active-bg) !important;
    color: var(--pill-active-text) !important;
    border-color: var(--pill-active-border) !important;
    font-weight: 700;
  }

  .reset-btn {
    margin-left: auto;
    background: var(--bg-card);
    border: 1px solid var(--border-subtle);
    color: var(--text-secondary);
    padding: 0.3rem 0.6rem;
    font-size: 0.72rem;
  }

  .hardware-hint-banner {
    background: rgba(2, 132, 199, 0.1);
    border-left: 3px solid var(--accent-cyan);
    border-radius: var(--radius-btn);
    padding: 0.45rem 0.75rem;
    font-size: 0.75rem;
    color: var(--text-secondary);
    display: flex;
    align-items: center;
    gap: 0.5rem;
  }

  .manual-control-row {
    display: flex;
    align-items: center;
    gap: 0.75rem;
    background: var(--track-bg);
    padding: 0.45rem 0.75rem;
    border-radius: var(--radius-btn);
    border: 1px solid var(--border-highlight);
    font-size: 0.75rem;
  }

  .manual-slider {
    flex: 1;
    accent-color: var(--accent-cyan);
    height: 5px;
  }

  .offset-val {
    color: var(--accent-cyan);
    font-weight: 700;
  }

  .sim-grid {
    display: grid;
    grid-template-columns: 1.15fr 1fr;
    gap: 0.85rem;
  }

  @media (max-width: 820px) {
    .sim-grid {
      grid-template-columns: 1fr;
    }
  }

  .sim-left, .sim-right {
    display: flex;
    flex-direction: column;
    gap: 0.75rem;
  }

  .virtual-track-box {
    background: var(--track-bg);
    border: 1px solid var(--border-subtle);
    border-radius: var(--radius-btn);
    padding: 0.75rem;
  }

  .track-header {
    display: flex;
    justify-content: space-between;
    font-size: 0.72rem;
    color: var(--text-secondary);
    margin-bottom: 0.5rem;
  }

  .track-stage {
    position: relative;
    height: 100px;
    background: #090c12;
    border-radius: var(--radius-btn);
    overflow: hidden;
    display: flex;
    align-items: center;
    justify-content: center;
  }

  .center-guideline {
    position: absolute;
    left: 50%;
    top: 0;
    bottom: 0;
    width: 2px;
    border-left: 1px dashed rgba(217, 119, 6, 0.6);
  }

  .simulated-line {
    position: absolute;
    top: 0;
    bottom: 0;
    width: 12px;
    background: #00f2fe;
    transform: translateX(-50%);
    transition: left 0.03s linear;
    border-radius: 6px;
    box-shadow: 0 0 12px #00f2fe;
  }

  .simulated-car {
    position: absolute;
    left: 50%;
    top: 50%;
    transform: translate(-50%, -50%);
    display: flex;
    flex-direction: column;
    align-items: center;
  }

  .car-body {
    display: flex;
    align-items: center;
    gap: 3px;
  }

  .car-wheel {
    width: 6px;
    height: 20px;
    background: #334155;
    border-radius: 2px;
  }

  .car-wheel.rev {
    background: #e11d48;
  }

  .car-chassis {
    width: 42px;
    height: 32px;
    background: #1e293b;
    border: 1px solid var(--accent-cyan);
    border-radius: 5px;
    display: flex;
    align-items: center;
    justify-content: center;
  }

  .car-badge {
    font-size: 0.6rem;
    font-weight: 700;
    color: var(--accent-cyan);
  }

  .sensor-bar-head {
    width: 54px;
    height: 3px;
    background: var(--accent-amber);
    margin-top: 3px;
    border-radius: 2px;
  }

  .sim-sensor-box {
    background: var(--track-bg);
    border: 1px solid var(--border-subtle);
    border-radius: var(--radius-btn);
    padding: 0.65rem;
  }

  .sim-box-title {
    display: flex;
    justify-content: space-between;
    font-size: 0.72rem;
    color: var(--text-secondary);
    margin-bottom: 0.45rem;
    font-weight: 600;
  }

  .sim-sensor-grid {
    display: grid;
    gap: 3px;
  }

  .sim-sensor-col {
    display: flex;
    flex-direction: column;
    align-items: center;
    gap: 3px;
  }

  .sim-bar-val {
    font-size: 0.58rem;
    color: var(--text-secondary);
  }

  .sim-bar-track {
    width: 100%;
    max-width: 14px;
    height: 48px;
    background: var(--bar-empty);
    border-radius: 2px;
    display: flex;
    align-items: flex-end;
    padding: 1px;
  }

  .sim-bar-fill {
    width: 100%;
    background: var(--border-input);
    border-radius: 1px;
    transition: height 0.04s ease;
  }

  .sim-bar-fill.active {
    background: #0284c7;
  }

  .sim-sensor-name {
    font-size: 0.58rem;
    color: var(--text-muted);
  }

  .sim-motor-box {
    display: grid;
    grid-template-columns: 1fr 1fr;
    gap: 0.5rem;
  }

  .motor-col {
    background: var(--track-bg);
    border: 1px solid var(--border-subtle);
    border-radius: var(--radius-btn);
    padding: 0.55rem;
  }

  .motor-info {
    display: flex;
    justify-content: space-between;
    font-size: 0.72rem;
    color: var(--text-secondary);
    margin-bottom: 0.35rem;
  }

  .motor-info strong {
    color: var(--accent-cyan);
  }

  .motor-info strong.rev {
    color: var(--accent-rose);
  }

  .motor-meter {
    height: 6px;
    background: var(--bar-empty);
    border-radius: 3px;
    overflow: hidden;
  }

  .meter-fill {
    height: 100%;
    border-radius: 3px;
    transition: width 0.04s ease;
  }

  .meter-fill.fwd {
    background: #0284c7;
  }

  .meter-fill.rev {
    background: #e11d48;
  }

  .math-breakdown-card, .sim-tuner-card {
    background: var(--track-bg);
    border: 1px solid var(--border-subtle);
    border-radius: var(--radius-btn);
    padding: 0.75rem;
  }

  h4 {
    margin: 0 0 0.6rem 0;
    font-size: 0.85rem;
    color: var(--text-heading);
    font-weight: 700;
  }

  .math-metrics-grid {
    display: grid;
    grid-template-columns: 1fr 1fr;
    gap: 0.45rem;
  }

  .metric-item {
    background: var(--bg-card);
    border: 1px solid var(--border-subtle);
    border-radius: var(--radius-btn);
    padding: 0.45rem;
    display: flex;
    flex-direction: column;
    gap: 0.15rem;
  }

  .metric-item.highlight {
    border-color: var(--accent-cyan);
    background: rgba(2, 132, 199, 0.08);
  }

  .m-label {
    font-size: 0.65rem;
    color: var(--text-secondary);
  }

  .m-val {
    font-size: 0.85rem;
    font-weight: 700;
    color: var(--text-primary);
  }

  .m-val.pos {
    color: var(--accent-cyan);
  }

  .m-val.neg {
    color: var(--accent-rose);
  }

  .sim-slider-group {
    margin-bottom: 0.55rem;
  }

  .s-label {
    display: flex;
    justify-content: space-between;
    font-size: 0.72rem;
    color: var(--text-secondary);
    margin-bottom: 0.2rem;
  }

  .s-val {
    color: var(--accent-cyan);
    font-weight: 700;
  }

  input[type="range"] {
    width: 100%;
    accent-color: var(--accent-cyan);
    height: 5px;
  }

  .btn-apply {
    width: 100%;
    margin-top: 0.4rem;
    background: #0284c7;
    color: #ffffff;
    font-weight: 700;
    box-shadow: 0 2px 8px rgba(2, 132, 199, 0.35);
  }

  .btn-apply:hover {
    background: #0369a1;
  }
</style>
