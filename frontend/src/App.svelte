<script>
  import { onMount, onDestroy } from 'svelte';
  import SensorVisualizer from './lib/SensorVisualizer.svelte';
  import SensorVisualizer8 from './lib/SensorVisualizer8.svelte';
  import MotorVisualizer from './lib/MotorVisualizer.svelte';
  import TuningPanel from './lib/TuningPanel.svelte';
  import ProfileManager from './lib/ProfileManager.svelte';
  import ControlBar from './lib/ControlBar.svelte';
  import SimulationModal from './lib/SimulationModal.svelte';
  import HardwareDrawer from './lib/HardwareDrawer.svelte';
  import BlackBoxModal from './lib/BlackBoxModal.svelte';
  import {
    getPorts,
    connectSerial,
    disconnectSerial,
    sendPid as bridgeSendPid,
    sendCommand as bridgeSendCommand,
    saveEeprom as bridgeSaveEeprom,
    readEeprom as bridgeReadEeprom,
    getProfiles as bridgeGetProfiles,
    saveProfile as bridgeSaveProfile,
    deleteProfile as bridgeDeleteProfile,
    subscribeTelemetry,
    isTauri
  } from './lib/tauriBridge.js';

  // Tema Oscuro / Claro
  let theme = 'dark';

  // Estado del Carro y Categoría
  let activeCategory = 'IM_16'; // 'IM_16' o 'CODEX_8'
  let activeCarName = 'Carro 1';
  let carFleet = ['Carro 1', 'Carro 2', 'Carro Competencia'];
  let newCarInput = '';
  let showNewCarInput = false;

  // Estado de Simulación, Hardware Drawer y Caja Negra
  let isSimOpen = false;
  let isHardwareOpen = false;
  let isBlackBoxOpen = false;

  // Estado de Conexión Serial
  let ports = [];
  let selectedPort = '';
  let isConnected = false;
  let statusMessage = 'Listo para conectar al robot';

  // Telemetría
  let telemetry = {
    channels: 16,
    raw: Array(16).fill(0),
    position: 7500,
    error: 0,
    leftMotor: 0,
    rightMotor: 0,
    state: 0
  };

  // Parámetros de Sintonización
  let config = {
    kp: 0.24,
    kd: 4.2,
    baseSpeed: 180,
    maxSpeed: 255,
    brakeSpeed: 130,
    forkMode: 0,
    lineColor: 0
  };

  let profiles = [];
  let eepromStatus = null;
  let ws = null;
  let wsReconnectTimer = null;

  function toggleTheme() {
    theme = theme === 'dark' ? 'light' : 'dark';
    document.documentElement.setAttribute('data-theme', theme);
    localStorage.setItem('lf_theme', theme);
  }

  function handleCategoryChange(cat) {
    activeCategory = cat;
    if (cat === 'CODEX_8') {
      telemetry.position = 3500;
      if (config.kp === 0.24) {
        config.kp = 0.35;
        config.kd = 5.0;
      }
    } else {
      telemetry.position = 7500;
      if (config.kp === 0.35) {
        config.kp = 0.24;
        config.kd = 4.2;
      }
    }
  }

  function handleAddCar() {
    if (newCarInput.trim()) {
      const name = newCarInput.trim();
      if (!carFleet.includes(name)) {
        carFleet = [...carFleet, name];
      }
      activeCarName = name;
      newCarInput = '';
      showNewCarInput = false;
      localStorage.setItem('lf_fleet', JSON.stringify(carFleet));
    }
  }

  let unsubscribeTelemetry = null;

  async function fetchPorts() {
    try {
      const data = await getPorts();
      ports = data.ports || [];
      isConnected = data.connected || false;
      if (data.currentPort) selectedPort = data.currentPort;
    } catch (e) {
      statusMessage = isTauri()
        ? 'Error consultando puertos seriales del sistema'
        : 'Asegúrate de que el backend esté activo en el puerto 5000.';
    }
  }

  async function handleConnect(e) {
    const detail = e.detail;
    const port = typeof detail === 'string' ? detail : detail.port;
    const baudRate = (typeof detail === 'object' && detail.baudRate) ? Number(detail.baudRate) : 115200;
    statusMessage = `Conectando a ${port} (${baudRate} baud)...`;
    try {
      const data = await connectSerial(port, baudRate);
      if (data.success) {
        isConnected = true;
        statusMessage = `Conectado a ${port} a ${baudRate} baudios`;
        // Enlace transparente: Activar telemetría bajo demanda ($CMD,STREAM_ON)
        try {
          await new Promise(r => setTimeout(r, 200));
          await bridgeSendCommand('STREAM_ON');
        } catch (errStream) {
          console.warn('No se pudo activar STREAM_ON automático:', errStream);
        }
      } else {
        statusMessage = `Fallo al conectar: ${data.message || 'Error desconocido'}`;
      }
    } catch (e) {
      statusMessage = `Error conectando: ${e.message || e}`;
    }
  }

  async function handleDisconnect() {
    try {
      // Enlace transparente: Mudar a modo silencioso ($CMD,STREAM_OFF) antes de cerrar puerto
      try {
        await bridgeSendCommand('STREAM_OFF');
        await new Promise(r => setTimeout(r, 50));
      } catch (errStream) {
        console.warn('No se pudo enviar STREAM_OFF previo a desconexión:', errStream);
      }
      await disconnectSerial();
      isConnected = false;
      statusMessage = 'Desconectado del puerto serial';
    } catch (e) {
      statusMessage = `Error desconectando: ${e.message || e}`;
    }
  }

  async function handleCommand(e) {
    const command = e.detail;
    try {
      await bridgeSendCommand(command);
      statusMessage = `Comando enviado: ${command}`;
    } catch (e) {
      statusMessage = `Error enviando comando: ${e.message || e}`;
    }
  }

  async function handleSendPid(e) {
    const params = e.detail;
    try {
      await bridgeSendPid(params);
      config = { ...config, ...params };
      statusMessage = 'Parámetros PD actualizados en la RAM del robot con éxito';
    } catch (e) {
      statusMessage = `Error actualizando PID: ${e.message || e}`;
    }
  }

  async function handleSaveEEPROM() {
    eepromStatus = { type: 'loading', message: 'Escribiendo y verificando en memoria EEPROM del robot...', details: '' };
    try {
      const data = await bridgeSaveEeprom();
      if (data.success) {
        eepromStatus = {
          type: 'success',
          message: '¡Confirmado! Datos grabados y verificados en la memoria física del robot.',
          details: data.data || 'Lectura de verificación coincidente al 100%'
        };
        statusMessage = 'EEPROM: Guardado y verificado con éxito (LED 13 destelló 3 veces).';
      } else {
        eepromStatus = {
          type: 'error',
          message: 'Error al grabar en la EEPROM del robot.',
          details: data.message || 'Sin confirmación de verificación'
        };
        statusMessage = `Error EEPROM: ${data.message}`;
      }
    } catch (e) {
      eepromStatus = {
        type: 'error',
        message: 'Error de comunicación.',
        details: e.message || String(e)
      };
      statusMessage = `Error: ${e.message || e}`;
    }
  }

  async function handleReadEEPROM() {
    eepromStatus = { type: 'loading', message: 'Consultando memoria no volátil del robot...', details: '' };
    try {
      const data = await bridgeReadEeprom();
      if (data.success && data.data) {
        // Formato: $EEPROM_DATA,kp,kd,base,max,brake,fork,color
        const clean = data.data.replace('$EEPROM_DATA,', '').trim();
        const parts = clean.split(',');
        if (parts.length >= 7) {
          config = {
            kp: parseFloat(parts[0]),
            kd: parseFloat(parts[1]),
            baseSpeed: parseInt(parts[2]),
            maxSpeed: parseInt(parts[3]),
            brakeSpeed: parseInt(parts[4]),
            forkMode: parseInt(parts[5]),
            lineColor: parseInt(parts[6])
          };
          eepromStatus = {
            type: 'success',
            message: '¡Valores de EEPROM cargados en los ajustes PD con éxito!',
            details: `Kp=${config.kp.toFixed(4)}, Kd=${config.kd.toFixed(2)}, Base=${config.baseSpeed}, Freno=${config.brakeSpeed}`
          };
          statusMessage = `EEPROM sincronizada a controles: Kp=${config.kp}, Kd=${config.kd}, Base=${config.baseSpeed}`;
          return;
        }

        eepromStatus = {
          type: 'success',
          message: 'Datos leídos directamente de la EEPROM física del Arduino:',
          details: data.data
        };
      } else {
        eepromStatus = {
          type: 'error',
          message: 'No se pudo leer la EEPROM del robot.',
          details: data.message || 'Verifica que el robot esté conectado'
        };
      }
    } catch (e) {
      eepromStatus = {
        type: 'error',
        message: 'Error al consultar EEPROM.',
        details: e.message || String(e)
      };
    }
  }

  async function fetchProfiles() {
    try {
      profiles = await bridgeGetProfiles();
    } catch (e) {
      console.error('Error fetching profiles', e);
    }
  }

  async function handleSaveProfile(e) {
    const p = e.detail;
    try {
      await bridgeSaveProfile(p);
      statusMessage = `Perfil "${p.name}" guardado para ${p.carName} en SQLite`;
      fetchProfiles();
    } catch (e) {
      statusMessage = `Error guardando perfil: ${e.message || e}`;
    }
  }

  async function handleDeleteProfile(e) {
    const id = e.detail;
    try {
      await bridgeDeleteProfile(id);
      statusMessage = `Perfil eliminado de SQLite`;
      fetchProfiles();
    } catch (e) {
      statusMessage = `Error eliminando perfil: ${e.message || e}`;
    }
  }

  function handleLoadProfile(e) {
    const p = e.detail;
    config = {
      kp: p.kp,
      kd: p.kd,
      baseSpeed: p.baseSpeed,
      maxSpeed: p.maxSpeed,
      brakeSpeed: p.brakeSpeed,
      forkMode: p.forkMode,
      lineColor: p.lineColor
    };
    if (p.carCategory) activeCategory = p.carCategory;
    if (p.carName) activeCarName = p.carName;

    if (isConnected) {
      handleSendPid({ detail: config });
      statusMessage = `Perfil "${p.name}" (${activeCarName}) cargado y transmitido a RAM`;
    } else {
      statusMessage = `Perfil "${p.name}" (${activeCarName}) cargado en los controles`;
    }
  }

  function handleApplySimSettings(e) {
    const simSettings = e.detail;
    config = {
      ...config,
      kp: simSettings.kp,
      kd: simSettings.kd,
      baseSpeed: simSettings.baseSpeed,
      maxSpeed: simSettings.maxSpeed,
      brakeSpeed: simSettings.brakeSpeed
    };
    if (isConnected) {
      handleSendPid({ detail: config });
    }
    statusMessage = 'Ganancias optimizadas en el Simulador transferidas a los controles.';
  }

  onMount(() => {
    const savedTheme = localStorage.getItem('lf_theme') || 'dark';
    theme = savedTheme;
    document.documentElement.setAttribute('data-theme', theme);

    const savedFleet = localStorage.getItem('lf_fleet');
    if (savedFleet) {
      try {
        carFleet = JSON.parse(savedFleet);
      } catch (e) {}
    }

    fetchPorts();
    fetchProfiles();

    unsubscribeTelemetry = subscribeTelemetry(
      (data) => {
        const rawArr = data.raw || data.Raw || [];
        const channels = rawArr.length > 0 ? rawArr.length : (activeCategory === 'CODEX_8' ? 8 : 16);

        if (rawArr.length === 8 && activeCategory !== 'CODEX_8') {
          activeCategory = 'CODEX_8';
        } else if (rawArr.length === 16 && activeCategory !== 'IM_16') {
          activeCategory = 'IM_16';
        }

        telemetry = {
          channels,
          raw: rawArr,
          position: (data.position !== undefined ? data.position : data.Position) ?? (activeCategory === 'CODEX_8' ? 3500 : 7500),
          error: (data.error !== undefined ? data.error : data.Error) ?? 0,
          leftMotor: (data.leftMotor !== undefined ? data.leftMotor : (data.LeftMotor !== undefined ? data.LeftMotor : data.left_motor)) ?? 0,
          rightMotor: (data.rightMotor !== undefined ? data.rightMotor : (data.RightMotor !== undefined ? data.RightMotor : data.right_motor)) ?? 0,
          state: (data.state !== undefined ? data.state : data.State) ?? 0
        };
      },
      (logMsg) => {
        statusMessage = `[Robot]: ${logMsg}`;
      }
    );
  });

  onDestroy(() => {
    if (unsubscribeTelemetry) unsubscribeTelemetry();
  });
</script>

<main class="dashboard-layout">
  <!-- Barra Superior Compacta -->
  <header class="app-header precision-card">
    <div class="brand">
      <div class="logo-icon">🏎️</div>
      <div class="brand-text">
        <div class="title-row">
          <h1>LineFollower Pro</h1>
          <span class="precision-chip version-chip">v0.2.0-beta</span>
        </div>
        <p class="subtitle">Arduino Nano (16L & 8L) • Tauri v2 Desktop (Rust) • SQLite Local</p>
      </div>
    </div>

    <!-- Pestañas de Categoría -->
    <div class="category-tabs">
      <button 
        class="category-tab {activeCategory === 'IM_16' ? 'active' : ''}" 
        on:click={() => handleCategoryChange('IM_16')}
      >
        <span class="tab-icon">🏎️</span>
        <span class="tab-label">16L Ingeniero Maker</span>
      </button>

      <button 
        class="category-tab {activeCategory === 'CODEX_8' ? 'active' : ''}" 
        on:click={() => handleCategoryChange('CODEX_8')}
      >
        <span class="tab-icon">⚡</span>
        <span class="tab-label">8L Codex Direct</span>
      </button>
    </div>

    <!-- Selector de Carro -->
    <div class="fleet-selector-wrap">
      <label for="carSelect" class="fleet-label">Carro:</label>
      <select id="carSelect" bind:value={activeCarName} class="car-select precision-mono">
        {#each carFleet as car}
          <option value={car}>{car}</option>
        {/each}
      </select>
      
      {#if !showNewCarInput}
        <button class="precision-btn btn-add-car" on:click={() => showNewCarInput = true} title="Añadir nuevo carro">
          +
        </button>
      {:else}
        <div class="new-car-modal-inline">
          <input 
            type="text" 
            placeholder="Nombre..." 
            bind:value={newCarInput} 
            class="new-car-input precision-mono" 
          />
          <button class="precision-btn btn-confirm-car" on:click={handleAddCar}>✓</button>
          <button class="precision-btn btn-cancel-car" on:click={() => showNewCarInput = false}>✕</button>
        </div>
      {/if}
    </div>

    <!-- Controles de Modo y Estado -->
    <div class="header-controls">
      <button 
        class="blackbox-btn precision-btn" 
        on:click={() => isBlackBoxOpen = true} 
        title="Abrir Caja Negra, Historial de 5 Vueltas, Comparador RMSE y Exportación CSV"
      >
        🔴 Caja Negra
      </button>

      <button 
        class="hardware-btn precision-btn" 
        on:click={() => isHardwareOpen = true} 
        title="Abrir Flasheador de Firmware, Esquema de Pines y Drivers"
      >
        🛠️ Hardware & Flasher
      </button>

      <button class="theme-toggle precision-btn" on:click={toggleTheme} title="Alternar Modo Oscuro / Modo Claro">
        {theme === 'dark' ? '☀️ Claro' : '🌙 Oscuro'}
      </button>

      <div class="system-status precision-chip {isConnected ? 'status-online' : 'status-offline'}">
        <span class="live-dot {isConnected ? 'active' : ''}"></span>
        <span>{isConnected ? 'ONLINE' : 'DESCONECTADO'}</span>
      </div>
    </div>
  </header>

  <!-- Barra de Control -->
  <ControlBar 
    {ports} 
    {selectedPort} 
    {isConnected} 
    {statusMessage}
    on:connect={handleConnect}
    on:disconnect={handleDisconnect}
    on:refreshPorts={fetchPorts}
    on:command={handleCommand}
    on:openSim={() => isSimOpen = true}
  />

  <!-- Grid Principal Compacto -->
  <div class="content-grid">
    <!-- Columna Izquierda: Sensores y Tracción -->
    <div class="left-column">
      {#if activeCategory === 'CODEX_8'}
        <SensorVisualizer8 
          raw={telemetry.raw} 
          position={telemetry.position} 
          error={telemetry.error} 
          state={telemetry.state} 
        />
      {:else}
        <SensorVisualizer 
          raw={telemetry.raw} 
          position={telemetry.position} 
          error={telemetry.error} 
          state={telemetry.state} 
        />
      {/if}

      <MotorVisualizer 
        leftMotor={telemetry.leftMotor} 
        rightMotor={telemetry.rightMotor} 
      />
    </div>

    <!-- Columna Derecha: Sintonía y Perfiles -->
    <div class="right-column">
      <TuningPanel 
        kp={config.kp} 
        kd={config.kd} 
        baseSpeed={config.baseSpeed} 
        maxSpeed={config.maxSpeed} 
        brakeSpeed={config.brakeSpeed} 
        forkMode={config.forkMode} 
        lineColor={config.lineColor} 
        {isConnected}
        {eepromStatus}
        {activeCarName}
        {activeCategory}
        on:sendPid={handleSendPid}
        on:saveEEPROM={handleSaveEEPROM}
        on:readEEPROM={handleReadEEPROM}
        on:saveProfile={handleSaveProfile}
      />

      <ProfileManager 
        {profiles}
        {activeCategory}
        on:loadProfile={handleLoadProfile}
        on:deleteProfile={handleDeleteProfile}
      />
    </div>
  </div>

  <!-- Modal del Simulador con Soporte de Sensores Reales en Vivo -->
  <SimulationModal 
    isOpen={isSimOpen}
    category={activeCategory}
    {isConnected}
    realTelemetryRaw={telemetry.raw}
    currentKp={config.kp}
    currentKd={config.kd}
    currentBaseSpeed={config.baseSpeed}
    currentMaxSpeed={config.maxSpeed}
    currentBrakeSpeed={config.brakeSpeed}
    on:close={() => isSimOpen = false}
    on:applySettings={handleApplySimSettings}
  />

  <!-- Panel Lateral de Hardware, Flasher y Pinout -->
  <HardwareDrawer 
    isOpen={isHardwareOpen} 
    onClose={() => isHardwareOpen = false} 
    currentPort={selectedPort} 
    currentCategory={activeCategory} 
  />

  <!-- Modal de Caja Negra & Data Logger -->
  <BlackBoxModal 
    bind:isOpen={isBlackBoxOpen}
    {telemetry}
    {isConnected}
    {activeCarName}
    on:close={() => isBlackBoxOpen = false}
  />
</main>

<style>
  .dashboard-layout {
    max-width: 1400px;
    margin: 0 auto;
    padding: 0.75rem 1rem;
    box-sizing: border-box;
  }

  .app-header {
    display: flex;
    justify-content: space-between;
    align-items: center;
    margin-bottom: 0.75rem;
    padding: 0.55rem 0.85rem;
    flex-wrap: wrap;
    gap: 0.65rem;
  }

  .brand {
    display: flex;
    align-items: center;
    gap: 0.65rem;
  }

  .logo-icon {
    font-size: 1.6rem;
  }

  .title-row {
    display: flex;
    align-items: center;
    gap: 0.5rem;
  }

  .app-header h1 {
    margin: 0;
    font-size: 1.15rem;
    font-weight: 700;
    letter-spacing: -0.02em;
    background: linear-gradient(90deg, var(--text-heading), var(--accent-cyan));
    -webkit-background-clip: text;
    -webkit-text-fill-color: transparent;
  }

  .version-chip {
    background: var(--chip-blue-bg);
    border: 1px solid var(--chip-blue-border);
    color: var(--chip-blue-text);
    font-size: 0.62rem;
  }

  .subtitle {
    margin: 0.1rem 0 0 0;
    font-size: 0.7rem;
    color: var(--text-secondary);
  }

  .category-tabs {
    display: flex;
    background: var(--track-bg);
    padding: 0.2rem;
    border-radius: var(--radius-btn);
    border: 1px solid var(--border-subtle);
    gap: 0.2rem;
  }

  .category-tab {
    font-family: var(--font-geo);
    display: flex;
    align-items: center;
    gap: 0.35rem;
    background: transparent;
    border: none;
    color: var(--text-secondary);
    padding: 0.3rem 0.65rem;
    border-radius: var(--radius-chip);
    font-size: 0.75rem;
    font-weight: 600;
    cursor: pointer;
    transition: all 0.15s ease;
  }

  .category-tab.active {
    background: var(--bg-card);
    color: var(--text-heading);
    box-shadow: 0 1px 4px rgba(0, 0, 0, 0.15);
    border: 1px solid var(--border-highlight);
  }

  .tab-icon {
    font-size: 0.85rem;
  }

  .fleet-selector-wrap {
    display: flex;
    align-items: center;
    gap: 0.35rem;
    background: var(--track-bg);
    padding: 0.25rem 0.5rem;
    border-radius: var(--radius-btn);
    border: 1px solid var(--border-subtle);
  }

  .fleet-label {
    font-size: 0.72rem;
    color: var(--text-secondary);
    font-weight: 600;
  }

  .car-select {
    padding: 0.25rem 0.45rem;
    font-size: 0.75rem;
  }

  .btn-add-car {
    background: var(--bg-card);
    border: 1px solid var(--border-subtle);
    color: var(--accent-cyan);
    padding: 0.25rem 0.45rem;
    font-size: 0.75rem;
  }

  .new-car-modal-inline {
    display: flex;
    align-items: center;
    gap: 0.25rem;
  }

  .new-car-input {
    width: 85px;
    padding: 0.2rem 0.35rem;
    font-size: 0.72rem;
  }

  .btn-confirm-car {
    background: #059669;
    color: #fff;
    padding: 0.2rem 0.4rem;
    font-size: 0.7rem;
  }

  .btn-cancel-car {
    background: var(--bg-subtle);
    color: var(--text-muted);
    padding: 0.2rem 0.4rem;
    font-size: 0.7rem;
  }

  .header-controls {
    display: flex;
    align-items: center;
    gap: 0.55rem;
  }

  .blackbox-btn {
    background: rgba(239, 68, 68, 0.12);
    border: 1px solid rgba(239, 68, 68, 0.35);
    color: #ef4444;
    padding: 0.35rem 0.65rem;
    font-size: 0.75rem;
    font-weight: 700;
  }

  .blackbox-btn:hover {
    background: rgba(239, 68, 68, 0.22);
    border-color: #ef4444;
  }

  .hardware-btn {
    background: var(--track-bg);
    border: 1px solid var(--border-subtle);
    color: var(--text-primary);
    padding: 0.35rem 0.65rem;
    font-size: 0.75rem;
    font-weight: 600;
  }

  .hardware-btn:hover {
    border-color: var(--border-highlight);
    background: var(--bg-hover);
  }

  .theme-toggle {
    background: var(--track-bg);
    border: 1px solid var(--border-subtle);
    color: var(--text-primary);
    padding: 0.35rem 0.65rem;
    font-size: 0.75rem;
  }

  .theme-toggle:hover {
    border-color: var(--border-highlight);
  }

  .system-status {
    border: 1px solid transparent;
  }

  .status-online {
    background: var(--chip-green-bg);
    border-color: var(--chip-green-border);
    color: var(--chip-green-text);
  }

  .status-offline {
    background: var(--track-bg);
    border-color: var(--border-subtle);
    color: var(--text-muted);
  }

  .live-dot {
    width: 6px;
    height: 6px;
    border-radius: 50%;
    background: #ef4444;
  }

  .live-dot.active {
    background: #10b981;
    box-shadow: 0 0 8px #10b981;
  }

  .content-grid {
    display: grid;
    grid-template-columns: 1.1fr 1fr;
    gap: 0.75rem;
  }

  @media (max-width: 1040px) {
    .content-grid {
      grid-template-columns: 1fr;
    }
  }

  .left-column, .right-column {
    display: flex;
    flex-direction: column;
    gap: 0.75rem;
  }
</style>
