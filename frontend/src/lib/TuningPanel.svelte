<script>
  import { createEventDispatcher } from 'svelte';

  export let kp = 0.24;
  export let kd = 4.2;
  export let baseSpeed = 180;
  export let maxSpeed = 255;
  export let brakeSpeed = 130;
  export let forkMode = 0;
  export let lineColor = 0;
  export let isConnected = false;
  export let eepromStatus = null;
  export let activeCarName = 'Carro 1';
  export let activeCategory = 'IM_16';

  const dispatch = createEventDispatcher();

  let profileName = '';

  $: setpointVal = activeCategory === 'CODEX_8' ? 3500 : 7500;
  $: maxKpSlider = Math.max(1.5, Math.ceil((Number(kp) || 0) * 2) / 2 || 1.5);
  $: maxKdSlider = Math.max(20.0, Math.ceil((Number(kd) || 0) / 5) * 5 || 20.0);

  function handleSendPid() {
    dispatch('sendPid', {
      kp,
      kd,
      baseSpeed,
      maxSpeed,
      brakeSpeed,
      forkMode,
      lineColor
    });
  }

  function handleSaveEEPROM() {
    dispatch('saveEEPROM');
  }

  function handleReadEEPROM() {
    dispatch('readEEPROM');
  }

  function handleSaveProfile() {
    if (!profileName.trim()) {
      alert('Ingresa un nombre para el perfil');
      return;
    }
    dispatch('saveProfile', {
      name: profileName,
      carName: activeCarName || 'Carro 1',
      carCategory: activeCategory || 'IM_16',
      kp,
      kd,
      baseSpeed,
      maxSpeed,
      brakeSpeed,
      forkMode,
      lineColor
    });
    profileName = '';
  }
</script>

<div class="precision-card tuning-card">
  <div class="card-header">
    <div class="header-left">
      <span class="tuning-badge precision-chip">{activeCategory}</span>
      <h3>Ajuste PD & Dinámica</h3>
    </div>
    <div class="header-right">
      <span class="setpoint-tag precision-mono">SP: {setpointVal}</span>
      <span class="precision-chip {isConnected ? 'connected' : 'disconnected'}">
        {isConnected ? 'SERIAL CONECTADO' : 'OFFLINE'}
      </span>
    </div>
  </div>

  <div class="tuning-grid">
    <!-- Ganancia Proporcional (Kp) - Sensibilidad -->
    <div class="input-group">
      <div class="group-label">
        <label for="kp">Sensibilidad (Kp)</label>
        <span class="val-display precision-mono">{(Number(kp) || 0).toFixed(4)}</span>
      </div>
      <div class="slider-row">
        <input id="kp" type="range" min="0" max={maxKpSlider} step="0.0001" bind:value={kp} />
        <input type="number" min="0" step="0.0001" bind:value={kp} class="num-box precision-mono" />
      </div>
    </div>

    <!-- Ganancia Derivativa (Kd) - Corrección -->
    <div class="input-group">
      <div class="group-label">
        <label for="kd">Corrección (Kd)</label>
        <span class="val-display precision-mono">{(Number(kd) || 0).toFixed(4)}</span>
      </div>
      <div class="slider-row">
        <input id="kd" type="range" min="0" max={maxKdSlider} step="0.0001" bind:value={kd} />
        <input type="number" min="0" step="0.0001" bind:value={kd} class="num-box precision-mono" />
      </div>
    </div>

    <!-- Velocidad Base -->
    <div class="input-group">
      <div class="group-label">
        <label for="baseSpeed">Velocidad Base</label>
        <span class="val-display precision-mono">{baseSpeed} PWM</span>
      </div>
      <div class="slider-row">
        <input id="baseSpeed" type="range" min="50" max="255" step="5" bind:value={baseSpeed} />
        <input type="number" min="50" max="255" step="5" bind:value={baseSpeed} class="num-box precision-mono" />
      </div>
    </div>

    <!-- Velocidad Máxima -->
    <div class="input-group">
      <div class="group-label">
        <label for="maxSpeed">Velocidad Máxima</label>
        <span class="val-display precision-mono">{maxSpeed} PWM</span>
      </div>
      <div class="slider-row">
        <input id="maxSpeed" type="range" min="100" max="255" step="5" bind:value={maxSpeed} />
        <input type="number" min="100" max="255" step="5" bind:value={maxSpeed} class="num-box precision-mono" />
      </div>
    </div>

    <!-- Velocidad / Intensidad de Freno -->
    <div class="input-group">
      <div class="group-label">
        <label for="brakeSpeed">Freno Activo Curva</label>
        <span class="val-display precision-mono">{brakeSpeed} PWM</span>
      </div>
      <div class="slider-row">
        <input id="brakeSpeed" type="range" min="0" max="255" step="5" bind:value={brakeSpeed} />
        <input type="number" min="0" max="255" step="5" bind:value={brakeSpeed} class="num-box precision-mono" />
      </div>
    </div>

    <!-- Modo de Bifurcación -->
    <div class="input-group fork-group">
      <div class="group-label">
        <span class="group-title">Bifurcación / Desvío</span>
      </div>
      <div class="pill-selector">
        <button 
          class="pill {forkMode === 0 ? 'selected' : ''}" 
          on:click={() => forkMode = 0}
        >Recto</button>
        <button 
          class="pill {forkMode === 1 ? 'selected' : ''}" 
          on:click={() => forkMode = 1}
        >← Rama Izq</button>
        <button 
          class="pill {forkMode === 2 ? 'selected' : ''}" 
          on:click={() => forkMode = 2}
        >Rama Der →</button>
      </div>
    </div>

    <!-- Polaridad de Pista -->
    <div class="input-group line-color-group">
      <div class="group-label">
        <span class="group-title">Polaridad de Pista</span>
      </div>
      <div class="pill-selector">
        <button 
          class="pill {lineColor === 0 ? 'selected' : ''}" 
          on:click={() => lineColor = 0}
        >⚫ Línea Negra</button>
        <button 
          class="pill {lineColor === 1 ? 'selected' : ''}" 
          on:click={() => lineColor = 1}
        >⚪ Línea Blanca</button>
      </div>
    </div>
  </div>

  <!-- Botones de Acción de Alto Contraste -->
  <div class="action-row">
    <button class="precision-btn btn-primary" on:click={handleSendPid} disabled={!isConnected} title="Aplica los valores inmediatamente a la memoria volátil (RAM) del Arduino">
      🚀 Enviar a RAM
    </button>
    <button class="precision-btn btn-secondary" on:click={handleSaveEEPROM} disabled={!isConnected} title="Graba y valida en la EEPROM física del Arduino">
      💾 Guardar en EEPROM
    </button>
    <button class="precision-btn btn-verify" on:click={handleReadEEPROM} disabled={!isConnected} title="Lee la EEPROM del Arduino y carga los valores en este panel">
      🔍 Leer EEPROM
    </button>
  </div>

  <!-- Indicador Visual de Verificación EEPROM -->
  {#if eepromStatus}
    <div class="eeprom-feedback {eepromStatus.type}">
      <div class="feedback-header">
        <span class="icon">
          {#if eepromStatus.type === 'loading'}⏳{/if}
          {#if eepromStatus.type === 'success'}✅{/if}
          {#if eepromStatus.type === 'error'}❌{/if}
        </span>
        <span class="feedback-msg">{eepromStatus.message}</span>
      </div>
      {#if eepromStatus.details}
        <div class="feedback-details precision-mono">
          <span>{eepromStatus.details}</span>
        </div>
      {/if}
    </div>
  {/if}

  <!-- Guardar en SQLite -->
  <div class="sqlite-save-row">
    <span class="car-badge-mini precision-chip">
      🚗 {activeCarName}
    </span>
    <input 
      type="text" 
      placeholder="Nombre del perfil (ej: Pista Rápida Chicanas)" 
      bind:value={profileName}
      class="text-input" 
    />
    <button class="precision-btn btn-sqlite" on:click={handleSaveProfile}>
      + Guardar Perfil
    </button>
  </div>
</div>

<style>
  .tuning-card {
    padding: 0.85rem;
  }

  .card-header {
    display: flex;
    justify-content: space-between;
    align-items: center;
    margin-bottom: 0.75rem;
    gap: 0.5rem;
  }

  .header-left, .header-right {
    display: flex;
    align-items: center;
    gap: 0.5rem;
  }

  .tuning-badge {
    background: var(--chip-blue-bg);
    border: 1px solid var(--chip-blue-border);
    color: var(--chip-blue-text);
  }

  .setpoint-tag {
    font-size: 0.72rem;
    font-weight: 700;
    color: var(--accent-amber);
    background: rgba(217, 119, 6, 0.12);
    border: 1px solid rgba(217, 119, 6, 0.4);
    padding: 0.15rem 0.4rem;
    border-radius: var(--radius-chip);
  }

  .card-header h3 {
    margin: 0;
    font-size: 0.95rem;
    color: var(--text-heading);
    font-weight: 700;
  }

  .connected {
    background: var(--chip-green-bg);
    border: 1px solid var(--chip-green-border);
    color: var(--chip-green-text);
  }

  .disconnected {
    background: var(--bg-subtle);
    border: 1px solid var(--border-subtle);
    color: var(--text-muted);
  }

  .tuning-grid {
    display: grid;
    grid-template-columns: repeat(auto-fit, minmax(230px, 1fr));
    gap: 0.65rem;
    margin-bottom: 0.85rem;
  }

  .input-group {
    background: var(--track-bg);
    border: 1px solid var(--border-subtle);
    border-radius: var(--radius-btn);
    padding: 0.55rem 0.75rem;
    display: flex;
    flex-direction: column;
    gap: 0.35rem;
  }

  .group-label {
    display: flex;
    justify-content: space-between;
    align-items: center;
    font-size: 0.75rem;
    font-weight: 600;
    color: var(--text-secondary);
  }

  .val-display {
    color: var(--accent-cyan);
    font-weight: 700;
  }

  .slider-row {
    display: flex;
    align-items: center;
    gap: 0.5rem;
  }

  .slider-row input[type="range"] {
    flex: 1;
    accent-color: var(--accent-cyan);
    cursor: pointer;
    height: 6px;
  }

  .num-box {
    width: 65px;
    text-align: right;
    padding: 0.2rem 0.35rem;
    font-size: 0.75rem;
  }

  .pill-selector {
    display: flex;
    gap: 0.35rem;
  }

  .pill {
    flex: 1;
    font-family: var(--font-geo);
    background: var(--pill-bg);
    border: 1px solid var(--pill-border);
    color: var(--pill-text);
    padding: 0.35rem 0.25rem;
    border-radius: var(--radius-btn);
    font-size: 0.74rem;
    font-weight: 500;
    cursor: pointer;
    transition: all 0.15s ease;
  }

  .pill.selected {
    background: var(--pill-active-bg) !important;
    color: var(--pill-active-text) !important;
    border-color: var(--pill-active-border) !important;
    font-weight: 700;
    box-shadow: 0 2px 6px rgba(2, 132, 199, 0.3);
  }

  .action-row {
    display: flex;
    gap: 0.5rem;
    margin-bottom: 0.75rem;
  }

  .btn-primary {
    flex: 1;
    background: #0284c7;
    color: #ffffff;
    box-shadow: 0 2px 8px rgba(2, 132, 199, 0.35);
  }

  .btn-primary:hover:not(:disabled) {
    background: #0369a1;
  }

  .btn-secondary {
    flex: 1;
    background: var(--bg-btn-secondary, #1e293b);
    border: 1px solid var(--border-btn-secondary, #334155);
    color: var(--text-btn-secondary, #f8fafc);
    font-weight: 600;
  }

  .btn-secondary:hover:not(:disabled) {
    background: var(--bg-btn-secondary-hover, #334155);
    border-color: var(--border-highlight);
  }

  .btn-verify {
    flex: 1;
    background: rgba(2, 132, 199, 0.12);
    border: 1px solid #0284c7;
    color: #0284c7;
    font-weight: 700;
  }

  .btn-verify:hover:not(:disabled) {
    background: #0284c7;
    color: #ffffff;
  }

  .eeprom-feedback {
    border-radius: var(--radius-btn);
    padding: 0.55rem 0.85rem;
    margin-bottom: 0.75rem;
    font-size: 0.78rem;
    display: flex;
    flex-direction: column;
    gap: 0.25rem;
  }

  .eeprom-feedback.success {
    background: var(--chip-green-bg);
    border: 1px solid var(--chip-green-border);
    color: var(--chip-green-text);
  }

  .eeprom-feedback.error {
    background: var(--chip-red-bg);
    border: 1px solid var(--chip-red-border);
    color: var(--chip-red-text);
  }

  .eeprom-feedback.loading {
    background: var(--chip-blue-bg);
    border: 1px solid var(--chip-blue-border);
    color: var(--chip-blue-text);
  }

  .feedback-header {
    display: flex;
    align-items: center;
    gap: 0.4rem;
    font-weight: 700;
  }

  .feedback-details {
    font-size: 0.72rem;
    padding-left: 1.25rem;
  }

  .sqlite-save-row {
    display: flex;
    gap: 0.5rem;
    padding-top: 0.75rem;
    border-top: 1px solid var(--border-subtle);
    align-items: center;
  }

  .car-badge-mini {
    background: rgba(124, 58, 237, 0.15);
    border: 1px solid rgba(124, 58, 237, 0.4);
    color: var(--accent-purple);
    font-size: 0.7rem;
    white-space: nowrap;
  }

  .text-input {
    flex: 1;
    font-size: 0.78rem;
  }

  .btn-sqlite {
    background: #059669;
    color: #ffffff;
    font-weight: 700;
    white-space: nowrap;
  }

  .btn-sqlite:hover {
    background: #047857;
  }
</style>
