<script>
  import { onMount, onDestroy } from 'svelte';
  import Sidebar from './lib/Sidebar.svelte';
  import SensorVisualizer from './lib/SensorVisualizer.svelte';
  import SensorVisualizer8 from './lib/SensorVisualizer8.svelte';
  import MotorVisualizer from './lib/MotorVisualizer.svelte';
  import TuningPanel from './lib/TuningPanel.svelte';
  import ProfileManager from './lib/ProfileManager.svelte';
  import ControlBar from './lib/ControlBar.svelte';
  import SimulationModal from './lib/SimulationModal.svelte';
  import HardwareDrawer from './lib/HardwareDrawer.svelte';
  import BlackBoxModal from './lib/BlackBoxModal.svelte';
  import AutoTuningModal from './lib/AutoTuningModal.svelte';
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

  // Estado de Simulación, Hardware Drawer, Caja Negra y Auto-Tuning
  let isSimOpen = false;
  let isHardwareOpen = false;
  let isBlackBoxOpen = false;
  let isAutoTuningOpen = false;

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

  function handleApplyAutoTuning(e) {
    const gains = e.detail;
    config = {
      ...config,
      kp: gains.kp,
      kd: gains.kd,
      baseSpeed: gains.baseSpeed,
      maxSpeed: gains.maxSpeed,
      brakeSpeed: gains.brakeSpeed
    };
    statusMessage = `Auto-Tuning: Ganancias aplicadas (Kp=${gains.kp}, Kd=${gains.kd}, Base=${gains.baseSpeed})`;
    if (isConnected) {
      handleSendPid({ detail: config });
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
            details: `Sensibilidad (Kp)=${config.kp.toFixed(4)}, Corrección (Kd)=${config.kd.toFixed(4)}, Base=${config.baseSpeed}, Freno=${config.brakeSpeed}`
          };
          statusMessage = `EEPROM sincronizada a controles: Kp=${config.kp.toFixed(4)}, Kd=${config.kd.toFixed(4)}, Base=${config.baseSpeed}`;
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

<div class="app-root-shell">
  <!-- Navbar Lateral de la Suite -->
  <Sidebar 
    {theme}
    {isConnected}
    {activeCategory}
    {activeCarName}
    on:openAutoTuning={() => isAutoTuningOpen = true}
    on:openBlackBox={() => isBlackBoxOpen = true}
    on:openHardware={() => isHardwareOpen = true}
    on:openSim={() => isSimOpen = true}
    on:toggleTheme={toggleTheme}
  />

  <!-- Área de Trabajo Principal Fluida -->
  <main class="main-workspace">
    <!-- Barra Superior Compacta: Categoría, Carro y Setpoint -->
    <header class="workspace-header precision-card">
      <div class="header-left">
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
      </div>

      <div class="header-right-badges">
        <span class="precision-chip status-setpoint precision-mono">
          SETPOINT: {activeCategory === 'CODEX_8' ? '3500' : '7500'}
        </span>
      </div>
    </header>

    <!-- Barra de Control Serial & Carrera -->
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

    <!-- Grid Principal de 3 Columnas Adaptable a Cualquier Pantalla -->
    <div class="content-grid">
      <!-- Columna 1: Telemetría Viva (Sensores y Tracción) -->
      <div class="grid-column col-telemetry">
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

      <!-- Columna 2: Sintonización PD & Dinámica -->
      <div class="grid-column col-tuning">
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
      </div>

      <!-- Columna 3: Perfiles & Flota SQLite -->
      <div class="grid-column col-profiles">
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

    <!-- Modal de Asistente de Auto-Sintonización PID Analítica -->
    <AutoTuningModal 
      bind:isOpen={isAutoTuningOpen}
      {activeCategory}
      currentKp={config.kp}
      currentKd={config.kd}
      currentBaseSpeed={config.baseSpeed}
      {isConnected}
      on:close={() => isAutoTuningOpen = false}
      on:applyGains={handleApplyAutoTuning}
    />
  </main>
</div>

<style>
  .app-root-shell {
    display: flex;
    width: 100vw;
    height: 100vh;
    overflow: hidden;
    box-sizing: border-box;
    background: var(--bg-app);
    padding: 0.5rem;
    gap: 0.5rem;
  }

  .main-workspace {
    flex: 1;
    display: flex;
    flex-direction: column;
    height: 100%;
    overflow-y: auto;
    overflow-x: hidden;
    padding: 0.25rem 0.5rem 1rem 0.5rem;
    box-sizing: border-box;
    gap: 0.65rem;
    min-width: 0;
  }

  .workspace-header {
    display: flex;
    justify-content: space-between;
    align-items: center;
    padding: 0.45rem 0.85rem;
    flex-wrap: wrap;
    gap: 0.65rem;
  }

  .header-left {
    display: flex;
    align-items: center;
    gap: 0.75rem;
    flex-wrap: wrap;
  }

  .status-setpoint {
    font-size: 0.7rem;
    font-weight: 700;
    color: var(--accent-cyan);
    background: rgba(0, 242, 254, 0.1);
    border: 1px solid rgba(0, 242, 254, 0.3);
    padding: 0.25rem 0.6rem;
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

  /* Grid Principal Adaptable de 3 Columnas */
  .content-grid {
    display: grid;
    grid-template-columns: 1.15fr 1fr 1fr;
    gap: 0.75rem;
    align-items: start;
    width: 100%;
    box-sizing: border-box;
  }

  .grid-column {
    display: flex;
    flex-direction: column;
    gap: 0.75rem;
    min-width: 0;
  }

  @media (max-width: 1550px) {
    .content-grid {
      grid-template-columns: 1.15fr 1fr;
    }
    .col-profiles {
      grid-column: 2;
    }
  }

  @media (max-width: 1040px) {
    .content-grid {
      grid-template-columns: 1fr;
    }
    .col-profiles {
      grid-column: 1;
    }
  }

  @media (max-width: 820px) {
    .app-root-shell {
      flex-direction: column;
      height: auto;
      min-height: 100vh;
      overflow-y: auto;
    }
    .main-workspace {
      height: auto;
      overflow-y: visible;
    }
  }
</style>
