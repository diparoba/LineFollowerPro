<script>
  import { createEventDispatcher } from 'svelte';

  export let ports = [];
  export let selectedPort = '';
  export let selectedBaud = 115200;
  export let isConnected = false;
  export let statusMessage = '';

  const dispatch = createEventDispatcher();

  function handleConnect() {
    if (isConnected) {
      dispatch('disconnect');
    } else {
      if (!selectedPort) {
        alert('Selecciona un puerto COM primero');
        return;
      }
      dispatch('connect', { port: selectedPort, baudRate: selectedBaud });
    }
  }

  function handleRefreshPorts() {
    dispatch('refreshPorts');
  }

  function sendCmd(cmd) {
    dispatch('command', cmd);
  }

  function handleOpenSim() {
    dispatch('openSim');
  }
</script>

<div class="precision-card control-bar">
  <div class="serial-section">
    <div class="com-select-group">
      <label for="comPort">COM:</label>
      <select id="comPort" bind:value={selectedPort} disabled={isConnected}>
        <option value="">-- Puerto --</option>
        {#each ports as p}
          <option value={p}>{p}</option>
        {/each}
      </select>
      <button class="precision-btn btn-icon" on:click={handleRefreshPorts} title="Actualizar puertos COM" disabled={isConnected}>
        🔄
      </button>
      <select id="baudRate" bind:value={selectedBaud} disabled={isConnected} class="baud-select" title="Velocidad en baudios (USB o Bluetooth HC-05)">
        <option value={115200}>115200</option>
        <option value={9600}>9600 (HC-05)</option>
      </select>
    </div>

    <button 
      class="precision-btn {isConnected ? 'btn-disconnect' : 'btn-connect'}" 
      on:click={handleConnect}
    >
      {isConnected ? 'Desconectar' : 'Conectar Serial'}
    </button>
  </div>

  <div class="robot-actions">
    <!-- Simular Arranque -->
    <button 
      class="precision-btn btn-sim" 
      on:click={handleOpenSim}
      title="Simular avance con sensores reales o virtuales sin mover motores"
    >
      ⚡ Simular Arranque
    </button>

    <button 
      class="precision-btn btn-calib" 
      on:click={() => sendCmd('CAL_BLACK')} 
      disabled={!isConnected}
      title="Calibrar negro (1er toque)"
    >
      ⚫ Calib. Negro
    </button>

    <button 
      class="precision-btn btn-calib" 
      on:click={() => sendCmd('CAL_WHITE')} 
      disabled={!isConnected}
      title="Calibrar blanco (2do toque)"
    >
      ⚪ Calib. Blanco
    </button>

    <button 
      class="precision-btn btn-start" 
      on:click={() => sendCmd('START')} 
      disabled={!isConnected}
      title="Arrancar robot (3er toque)"
    >
      ▶ Arrancar
    </button>

    <button 
      class="precision-btn btn-stop" 
      on:click={() => sendCmd('STOP')} 
      disabled={!isConnected}
      title="Parada de emergencia inmediata de motores"
    >
      ⏹ Parada
    </button>
  </div>
</div>

{#if statusMessage}
  <div class="status-banner">
    <span class="status-label precision-chip">ESTADO</span>
    <span class="status-text precision-mono">{statusMessage}</span>
  </div>
{/if}

<style>
  .control-bar {
    padding: 0.55rem 0.85rem;
    display: flex;
    justify-content: space-between;
    align-items: center;
    flex-wrap: wrap;
    gap: 0.65rem;
    margin-bottom: 0.75rem;
  }

  .serial-section {
    display: flex;
    align-items: center;
    gap: 0.5rem;
    flex-wrap: wrap;
  }

  .com-select-group {
    display: flex;
    align-items: center;
    gap: 0.35rem;
    font-size: 0.78rem;
    font-weight: 600;
    color: var(--text-secondary);
  }

  .robot-actions {
    display: flex;
    gap: 0.45rem;
    flex-wrap: wrap;
  }

  .btn-icon {
    background: var(--bg-card);
    border: 1px solid var(--border-subtle);
    color: var(--text-secondary);
    padding: 0.35rem 0.5rem;
    transition: all 0.15s ease;
  }

  .btn-icon:hover:not(:disabled) {
    background: var(--bg-hover);
    color: var(--text-primary);
    border-color: var(--border-highlight);
  }

  .btn-connect {
    background: #0284c7;
    color: #ffffff;
    font-weight: 700;
    box-shadow: 0 2px 6px rgba(2, 132, 199, 0.35);
  }

  .btn-connect:hover {
    background: #0369a1;
  }

  .btn-disconnect {
    background: #e11d48;
    color: #ffffff;
    font-weight: 700;
  }

  .btn-sim {
    background: #7c3aed;
    color: #ffffff;
    font-weight: 700;
    box-shadow: 0 2px 8px rgba(124, 58, 237, 0.35);
  }

  .btn-sim:hover {
    background: #6d28d9;
  }

  .btn-calib {
    background: var(--bg-card);
    border: 1px solid var(--border-subtle);
    color: var(--text-primary);
    transition: all 0.15s ease;
  }

  .btn-calib:hover:not(:disabled) {
    border-color: var(--border-highlight);
    background: var(--bg-hover);
  }

  .btn-start {
    background: #059669;
    color: #ffffff;
    font-weight: 700;
    box-shadow: 0 2px 6px rgba(5, 150, 105, 0.3);
  }

  .btn-start:hover:not(:disabled) {
    background: #047857;
  }

  .btn-stop {
    background: #e11d48;
    color: #ffffff;
    font-weight: 700;
    box-shadow: 0 2px 6px rgba(225, 29, 72, 0.3);
  }

  .btn-stop:hover:not(:disabled) {
    background: #be123c;
  }

  .status-banner {
    background: var(--bg-card);
    border: 1px solid var(--border-subtle);
    border-left: 3px solid var(--accent-cyan);
    border-radius: var(--radius-btn);
    padding: 0.35rem 0.65rem;
    margin-bottom: 0.75rem;
    display: flex;
    align-items: center;
    gap: 0.5rem;
  }

  .status-label {
    background: var(--chip-blue-bg);
    color: var(--chip-blue-text);
    border: 1px solid var(--chip-blue-border);
    font-size: 0.62rem;
  }

  .status-text {
    font-size: 0.75rem;
    color: var(--text-secondary);
  }
</style>
