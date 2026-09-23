<script>
  export let leftMotor = 0;
  export let rightMotor = 0;

  function getMotorPercent(speed) {
    return Math.min(100, Math.max(0, (Math.abs(speed) / 255) * 100));
  }

  $: diff = leftMotor - rightMotor;
</script>

<div class="precision-card motor-card">
  <div class="card-header">
    <div class="title-wrap">
      <span class="motor-badge precision-chip">PWM 31.37 kHz</span>
      <h3>Tracción & Motores</h3>
    </div>
    <span class="precision-chip diff-badge {diff > 20 ? 'turn-right' : diff < -20 ? 'turn-left' : 'straight'}">
      {diff > 20 ? 'GIRO DERECHA' : diff < -20 ? 'GIRO IZQUIERDA' : 'RECTO'}
    </span>
  </div>

  <div class="motors-grid">
    <!-- Motor Izquierdo (B: Pin 11) -->
    <div class="motor-unit">
      <div class="motor-label">
        <span class="name">Motor Izq (Pin 11)</span>
        <span class="speed precision-mono {leftMotor < 0 ? 'rev' : ''}">{leftMotor} PWM</span>
      </div>
      <div class="motor-bar-wrapper">
        <div 
          class="motor-bar left-bar {leftMotor < 0 ? 'reverse' : 'forward'}" 
          style="width: {getMotorPercent(leftMotor)}%"
        ></div>
      </div>
      <div class="direction-indicator">
        {#if leftMotor > 0}
          <span class="dir-fwd precision-chip">▲ AVANCE</span>
        {:else if leftMotor < 0}
          <span class="dir-rev precision-chip">▼ FRENO / REV</span>
        {:else}
          <span class="dir-stop precision-chip">● DETENIDO</span>
        {/if}
      </div>
    </div>

    <!-- Motor Derecho (A: Pin 3) -->
    <div class="motor-unit">
      <div class="motor-label">
        <span class="name">Motor Der (Pin 3)</span>
        <span class="speed precision-mono {rightMotor < 0 ? 'rev' : ''}">{rightMotor} PWM</span>
      </div>
      <div class="motor-bar-wrapper">
        <div 
          class="motor-bar right-bar {rightMotor < 0 ? 'reverse' : 'forward'}" 
          style="width: {getMotorPercent(rightMotor)}%"
        ></div>
      </div>
      <div class="direction-indicator">
        {#if rightMotor > 0}
          <span class="dir-fwd precision-chip">▲ AVANCE</span>
        {:else if rightMotor < 0}
          <span class="dir-rev precision-chip">▼ FRENO / REV</span>
        {:else}
          <span class="dir-stop precision-chip">● DETENIDO</span>
        {/if}
      </div>
    </div>
  </div>
</div>

<style>
  .motor-card {
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

  .motor-badge {
    background: rgba(124, 58, 237, 0.15);
    border: 1px solid rgba(124, 58, 237, 0.4);
    color: var(--accent-purple);
  }

  .card-header h3 {
    margin: 0;
    font-size: 0.95rem;
    color: var(--text-heading);
    font-weight: 700;
  }

  .diff-badge {
    border: 1px solid transparent;
  }

  .turn-right {
    background: var(--chip-blue-bg);
    border-color: var(--chip-blue-border);
    color: var(--chip-blue-text);
  }

  .turn-left {
    background: rgba(124, 58, 237, 0.15);
    border-color: rgba(124, 58, 237, 0.4);
    color: var(--accent-purple);
  }

  .straight {
    background: var(--bg-subtle);
    border-color: var(--border-subtle);
    color: var(--text-muted);
  }

  .motors-grid {
    display: grid;
    grid-template-columns: 1fr 1fr;
    gap: 0.75rem;
  }

  .motor-unit {
    background: var(--track-bg);
    border-radius: var(--radius-btn);
    padding: 0.65rem;
    border: 1px solid var(--border-subtle);
  }

  .motor-label {
    display: flex;
    justify-content: space-between;
    align-items: center;
    font-size: 0.75rem;
    margin-bottom: 0.4rem;
  }

  .name {
    color: var(--text-secondary);
    font-weight: 600;
  }

  .speed {
    color: var(--accent-cyan);
    font-weight: 700;
    font-size: 0.85rem;
  }

  .speed.rev {
    color: var(--accent-rose);
  }

  .motor-bar-wrapper {
    height: 7px;
    background: var(--bar-empty);
    border-radius: 4px;
    overflow: hidden;
    margin-bottom: 0.45rem;
  }

  .motor-bar {
    height: 100%;
    border-radius: 4px;
    transition: width 0.05s ease-out;
  }

  .motor-bar.forward {
    background: #0284c7;
  }

  .motor-bar.reverse {
    background: #e11d48;
  }

  .direction-indicator {
    display: flex;
  }

  .dir-fwd {
    background: var(--chip-blue-bg);
    color: var(--chip-blue-text);
    border: 1px solid var(--chip-blue-border);
  }

  .dir-rev {
    background: var(--chip-red-bg);
    color: var(--chip-red-text);
    border: 1px solid var(--chip-red-border);
  }

  .dir-stop {
    background: var(--bg-card);
    color: var(--text-muted);
    border: 1px solid var(--border-subtle);
  }
</style>
