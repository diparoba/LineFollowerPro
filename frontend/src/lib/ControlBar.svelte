<script>
  import { createEventDispatcher } from 'svelte';
  import { isTauri, requestWebSerialPort } from './tauriBridge.js';

  export let ports = [];
  export let selectedPort = '';
  export let selectedBaud = 115200;
  export let isConnected = false;
  export let statusMessage = '';

  const dispatch = createEventDispatcher();
  let isWeb = !isTauri();
  let requestingPermission = false;

  async function handleGrantPermission() {
    try {
      requestingPermission = true;
      await requestWebSerialPort();
      handleRefreshPorts();
    } catch (err) {
      if (err.message && !err.message.includes('No se seleccionó')) {
        alert(err.message || 'Error solicitando permiso Web Serial');
      }
    } finally {
      requestingPermission = false;
    }
  }

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
      <select id="comPort" class="com-select precision-mono" bind:value={selectedPort} disabled={isConnected}>
        <option value="">-- Seleccionar Puerto --</option>
        {#each ports as p}
          <option value={p}>{p}</option>
        {/each}
      </select>
      <button class="precision-btn btn-icon" on:click={handleRefreshPorts} title="Actualizar puertos COM" disabled={isConnected}>
        🔄
      </button>
      <select id="baudRate" bind:value={selectedBaud} disabled={isConnected} class="baud-select precision-mono" title="Velocidad en baudios (USB o Bluetooth HC-05)">
        <option value={115200}>115200</option>
        <option value={9600}>9600 (HC-05)</option>
      </select>
    </div>

    {#if isWeb}
      <button 
        class="precision-btn btn-permission" 
        on:click={handleGrantPermission} 
        disabled={isConnected || requestingPermission}
        title="Conceder permiso de acceso al puerto USB/COM en el navegador (Web Serial API para Arduino o ESP32)"
      >
        <span>{requestingPermission ? '⌛ Solicitando...' : '🔑 Conceder Permiso USB'}</span>
      </button>
    {/if}

    <button 
      class="precision-btn {isConnected ? 'btn-disconnect' : 'btn-connect'}" 
      on:click={handleConnect}
    >
      <span>{isConnected ? '🔌 Desconectar' : '🔌 Conectar Serial'}</span>
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
    padding: 0.65rem 0.95rem;
    display: flex;
    justify-content: space-between;
    align-items: center;
    flex-wrap: wrap;
    gap: 0.75rem;
    margin-bottom: 0.75rem;
    flex-shrink: 0;
    min-height: max-content;
    overflow: visible;
  }

  .serial-section {
    display: flex;
    align-items: center;
    gap: 0.65rem;
    flex-wrap: wrap;
  }

  .btn-permission {
    background: #0d9488;
    color: #ffffff;
    font-weight: 700;
    height: 36px;
    padding: 0 0.85rem;
    font-size: 0.82rem;
    border-radius: var(--radius-btn);
    box-shadow: 0 2px 6px rgba(13, 148, 136, 0.3);
    display: inline-flex;
    align-items: center;
    gap: 0.35rem;
    transition: all 0.15s ease;
  }

  .btn-permission:hover:not(:disabled) {
    background: #0f766e;
  }

  .com-select-group {
    display: flex;
    align-items: center;
    gap: 0.45rem;
    font-size: 0.85rem;
    font-weight: 700;
    color: var(--text-secondary);
  }

  .com-select, .baud-select {
    height: 36px;
    font-size: 0.85rem;
    padding: 0.35rem 0.65rem;
    border-radius: var(--radius-btn);
    background: var(--bg-input);
    border: 1px solid var(--border-medium);
    color: var(--text-primary);
    box-sizing: border-box;
  }

  .com-select {
    min-width: 155px;
    font-weight: 600;
  }

  .robot-actions {
    display: flex;
    gap: 0.45rem;
    flex-wrap: wrap;
    align-items: center;
  }

  .btn-icon {
    background: var(--bg-card);
    border: 1px solid var(--border-medium);
    color: var(--text-secondary);
    height: 36px;
    width: 36px;
    padding: 0;
    display: inline-flex;
    align-items: center;
    justify-content: center;
    font-size: 1rem;
    border-radius: var(--radius-btn);
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
    height: 36px;
    padding: 0 1.15rem;
    font-size: 0.85rem;
    box-shadow: 0 2px 8px rgba(2, 132, 199, 0.4);
  }

  .btn-connect:hover {
    background: #0369a1;
  }

  .btn-disconnect {
    background: #e11d48;
    color: #ffffff;
    font-weight: 700;
    height: 36px;
    padding: 0 1.15rem;
    font-size: 0.85rem;
  }

  .btn-sim {
    background: #7c3aed;
    color: #ffffff;
    font-weight: 700;
    height: 36px;
    padding: 0 0.85rem;
    font-size: 0.82rem;
    box-shadow: 0 2px 8px rgba(124, 58, 237, 0.35);
  }

  .btn-sim:hover {
    background: #6d28d9;
  }

  .btn-calib {
    background: var(--bg-card);
    border: 1px solid var(--border-subtle);
    color: var(--text-primary);
    height: 36px;
    padding: 0 0.8rem;
    font-size: 0.82rem;
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
    height: 36px;
    padding: 0 0.95rem;
    font-size: 0.82rem;
    box-shadow: 0 2px 6px rgba(5, 150, 105, 0.3);
  }

  .btn-start:hover:not(:disabled) {
    background: #047857;
  }

  .btn-stop {
    background: #e11d48;
    color: #ffffff;
    font-weight: 700;
    height: 36px;
    padding: 0 0.95rem;
    font-size: 0.82rem;
    box-shadow: 0 2px 6px rgba(225, 29, 72, 0.3);
  }

  .btn-stop:hover:not(:disabled) {
    background: #be123c;
  }

  .status-banner {
    flex-shrink: 0;
    background: var(--bg-card);
    border: 1px solid var(--border-subtle);
    border-left: 3px solid var(--accent-cyan);
    border-radius: var(--radius-btn);
    padding: 0.45rem 0.75rem;
    margin-bottom: 0.75rem;
    display: flex;
    align-items: center;
    gap: 0.5rem;
  }

  .status-label {
    background: var(--chip-blue-bg);
    color: var(--chip-blue-text);
    border: 1px solid var(--chip-blue-border);
    font-size: 0.65rem;
    font-weight: 700;
  }

  .status-text {
    font-size: 0.78rem;
    color: var(--text-secondary);
  }

  @media (max-width: 1200px) {
    .control-bar {
      flex-direction: column;
      align-items: stretch;
      gap: 0.6rem;
    }
    .serial-section, .robot-actions {
      width: 100%;
      justify-content: flex-start;
    }
  }
</style>
