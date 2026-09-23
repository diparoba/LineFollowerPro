<script>
  export let raw = Array(16).fill(0);
  export let position = 7500;
  export let error = 0;
  export let state = 0;

  function getBarHeight(val) {
    return Math.min(100, Math.max(5, (val / 1023) * 100));
  }

  $: posPercent = Math.min(100, Math.max(0, (1 - (position / 15000)) * 100));
</script>

<div class="precision-card sensor-card">
  <div class="card-header">
    <div class="title-wrap">
      <span class="sensor-badge precision-chip">IM-16 MUX</span>
      <h3>16 Sensores Infrarrojos (Ingeniero Maker)</h3>
    </div>
    <span class="precision-chip status-chip {state === 5 ? 'chip-running' : state === 1 ? 'chip-calib' : state === 3 ? 'chip-calib' : 'chip-idle'}">
      <span class="status-dot"></span>
      {state === 5 ? 'SEGUIDOR ACTIVO' : state === 1 ? 'CALIB. NEGRO' : state === 3 ? 'CALIB. BLANCO' : 'LISTO'}
    </span>
  </div>

  <div class="position-track-container">
    <div class="track-info">
      <span>Posición: <strong class="precision-mono">{position}</strong> / 15000</span>
      <span>Setpoint: <strong class="precision-mono">7500</strong></span>
      <span>Error: <strong class="precision-mono {error > 0 ? 'err-left' : 'err-right'}">{error > 0 ? `+${error} (Izq)` : `${error} (Der)`}</strong></span>
    </div>
    <div class="track-bar">
      <div class="setpoint-marker" title="Setpoint Central (7500)"></div>
      <div class="line-pointer" style="left: {posPercent}%">
        <div class="pointer-needle"></div>
        <div class="pointer-tag precision-mono">{position}</div>
      </div>
    </div>
  </div>

  <div class="sensor-grid">
    {#each Array.from({length: 16}, (_, k) => 15 - k) as i}
      <div class="sensor-col">
        <div class="val-label precision-mono">{raw[i] !== undefined ? raw[i] : 0}</div>
        <div class="bar-track">
          <div 
            class="bar-fill {raw[i] > 450 ? 'active' : ''} {i === 0 || i === 15 ? 'extreme' : ''}" 
            style="height: {getBarHeight(raw[i] || 0)}%"
          ></div>
        </div>
        <div class="sensor-num precision-mono {i === 7 || i === 8 ? 'center-sensor' : ''}">S{i}</div>
      </div>
    {/each}
  </div>

  <div class="sensor-legend">
    <span class="legend-item"><span class="dot extreme-dot"></span> S0 / S15: Extremos</span>
    <span class="legend-item"><span class="dot center-dot"></span> S7 / S8: Centro (7500)</span>
    <span class="legend-item"><span class="dot mux-dot"></span> Multiplexado (A0..A5)</span>
  </div>
</div>

<style>
  .sensor-card {
    padding: 0.85rem;
  }

  .card-header {
    display: flex;
    justify-content: space-between;
    align-items: center;
    margin-bottom: 0.65rem;
    gap: 0.5rem;
  }

  .title-wrap {
    display: flex;
    align-items: center;
    gap: 0.45rem;
  }

  .sensor-badge {
    background: var(--chip-blue-bg);
    border: 1px solid var(--chip-blue-border);
    color: var(--chip-blue-text);
  }

  .card-header h3 {
    margin: 0;
    font-size: 0.95rem;
    color: var(--text-heading);
    font-weight: 700;
  }

  .status-chip {
    border: 1px solid transparent;
  }

  .chip-running {
    background: var(--chip-green-bg);
    border-color: var(--chip-green-border);
    color: var(--chip-green-text);
  }

  .chip-calib {
    background: rgba(217, 119, 6, 0.15);
    border-color: rgba(217, 119, 6, 0.4);
    color: var(--accent-amber);
  }

  .chip-idle {
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

  .position-track-container {
    background: var(--track-bg);
    border-radius: var(--radius-btn);
    padding: 0.55rem 0.75rem;
    margin-bottom: 0.75rem;
    border: 1px solid var(--border-subtle);
  }

  .track-info {
    display: flex;
    justify-content: space-between;
    font-size: 0.75rem;
    color: var(--text-secondary);
    margin-bottom: 0.45rem;
  }

  .track-info strong {
    color: var(--text-primary);
  }

  .err-left {
    color: var(--accent-cyan) !important;
  }

  .err-right {
    color: var(--accent-rose) !important;
  }

  .track-bar {
    position: relative;
    height: 9px;
    background: var(--bar-empty);
    border-radius: 5px;
  }

  .setpoint-marker {
    position: absolute;
    left: 50%;
    top: -4px;
    bottom: -4px;
    width: 2px;
    background: var(--accent-amber);
    box-shadow: 0 0 6px var(--accent-amber);
    z-index: 1;
  }

  .line-pointer {
    position: absolute;
    top: -8px;
    transform: translateX(-50%);
    transition: left 0.05s ease-out;
    z-index: 2;
    display: flex;
    flex-direction: column;
    align-items: center;
  }

  .pointer-needle {
    width: 0;
    height: 0;
    border-left: 5px solid transparent;
    border-right: 5px solid transparent;
    border-top: 7px solid var(--accent-cyan);
  }

  .pointer-tag {
    font-size: 0.6rem;
    background: var(--bg-card);
    border: 1px solid var(--accent-cyan);
    color: var(--accent-cyan);
    border-radius: 2px;
    padding: 0px 3px;
  }

  .sensor-grid {
    display: grid;
    grid-template-columns: repeat(16, 1fr);
    gap: 4px;
    background: var(--track-bg);
    padding: 0.6rem 0.4rem;
    border-radius: var(--radius-btn);
    border: 1px solid var(--border-subtle);
  }

  .sensor-col {
    display: flex;
    flex-direction: column;
    align-items: center;
    gap: 3px;
  }

  .val-label {
    font-size: 0.58rem;
    color: var(--text-secondary);
    writing-mode: vertical-rl;
    transform: rotate(180deg);
    height: 24px;
  }

  .bar-track {
    width: 100%;
    max-width: 15px;
    height: 65px;
    background: var(--bar-empty);
    border-radius: var(--radius-chip);
    display: flex;
    align-items: flex-end;
    overflow: hidden;
    padding: 1px;
  }

  .bar-fill {
    width: 100%;
    background: var(--border-input);
    border-radius: 1px;
    transition: height 0.04s ease-out;
  }

  .bar-fill.active {
    background: #0284c7;
    box-shadow: 0 0 6px rgba(2, 132, 199, 0.5);
  }

  .bar-fill.active.extreme {
    background: var(--accent-amber);
    box-shadow: 0 0 6px rgba(217, 119, 6, 0.6);
  }

  .sensor-num {
    font-size: 0.62rem;
    font-weight: 700;
    color: var(--text-secondary);
  }

  .sensor-num.center-sensor {
    color: var(--accent-amber);
  }

  .sensor-legend {
    display: flex;
    gap: 1rem;
    justify-content: center;
    margin-top: 0.55rem;
    font-size: 0.7rem;
    color: var(--text-muted);
    flex-wrap: wrap;
  }

  .legend-item {
    display: flex;
    align-items: center;
    gap: 4px;
  }

  .dot {
    width: 6px;
    height: 6px;
    border-radius: 50%;
  }

  .extreme-dot {
    background: var(--accent-amber);
  }

  .center-dot {
    background: var(--accent-cyan);
  }

  .mux-dot {
    background: var(--accent-purple);
  }
</style>
