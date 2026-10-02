<script>
  import { onMount, onDestroy } from 'svelte';
  import { flashFirmware, installDriver, subscribeFlashLogs, getPorts } from './tauriBridge.js';

  export let isOpen = false;
  export let onClose = () => {};
  export let currentPort = '';
  export let currentCategory = 'IM_16';

  let activeTab = 'flasher'; // 'flasher' | 'pinout' | 'drivers'
  let pinoutModel = currentCategory; // 'IM_16' | 'CODEX_8'

  // Flasher state
  let availablePorts = [];
  let selectedPort = currentPort || '';
  let selectedRobot = currentCategory === 'CODEX_8' ? 'CODEX_8' : 'IM_16';
  let selectedBaud = 115200;
  let autoFallback = true;
  let isFlashing = false;
  let flashStatus = { type: '', message: '' };
  let flashLogs = [];
  let unsubscribeLogs = null;
  let terminalContainer;

  $: if (currentPort && !selectedPort) {
    selectedPort = currentPort;
  }
  $: if (currentCategory) {
    selectedRobot = currentCategory;
    pinoutModel = currentCategory;
  }

  onMount(async () => {
    await refreshPorts();
    unsubscribeLogs = subscribeFlashLogs((logMsg) => {
      flashLogs = [...flashLogs, logMsg];
      if (terminalContainer) {
        setTimeout(() => {
          terminalContainer.scrollTop = terminalContainer.scrollHeight;
        }, 50);
      }
    });
  });

  onDestroy(() => {
    if (unsubscribeLogs) unsubscribeLogs();
  });

  async function refreshPorts() {
    try {
      const info = await getPorts();
      availablePorts = info.ports || [];
      if (!selectedPort && availablePorts.length > 0) {
        selectedPort = availablePorts[0];
      }
    } catch (e) {
      console.error(e);
    }
  }

  async function handleFlash() {
    if (!selectedPort) {
      flashStatus = { type: 'error', message: 'Selecciona un puerto COM antes de flashear.' };
      return;
    }

    isFlashing = true;
    flashStatus = { type: 'info', message: 'Iniciando proceso de flasheo con avrdude...' };
    flashLogs = [
      `[${new Date().toLocaleTimeString()}] Iniciando flasheo para ${selectedRobot} en ${selectedPort}...`
    ];

    try {
      const res = await flashFirmware(selectedPort, selectedRobot, selectedBaud, autoFallback);
      if (res.success) {
        flashStatus = { type: 'success', message: res.message };
      } else {
        flashStatus = { type: 'error', message: res.message };
      }
    } catch (e) {
      flashStatus = { type: 'error', message: e.toString() };
      flashLogs = [...flashLogs, `[ERROR] ${e.toString()}`];
    } finally {
      isFlashing = false;
    }
  }

  let driverStatus = null;

  async function handleInstallDriver(driverType) {
    try {
      driverStatus = { type: 'info', message: 'Iniciando instalador local con permisos de Administrador...' };
      await installDriver(driverType);
      driverStatus = { type: 'success', message: 'Instalador iniciado en Windows. Sigue los pasos del asistente en pantalla.' };
      setTimeout(() => { driverStatus = null; }, 5000);
    } catch (e) {
      driverStatus = { type: 'error', message: `Error ejecutando instalador: ${e.toString()}` };
    }
  }

  function clearLogs() {
    flashLogs = [];
  }
</script>

{#if isOpen}
  <!-- Backdrop -->
  <div 
    class="drawer-backdrop" 
    on:click={onClose}
    on:keydown={(e) => e.key === 'Escape' && onClose()}
    role="button"
    tabindex="0"
    aria-label="Cerrar panel lateral"
  ></div>

  <!-- Slide-out Drawer -->
  <aside class="drawer-container">
    <div class="drawer-header">
      <div class="header-title">
        <span class="icon">🛠️</span>
        <div>
          <h2>Hardware & Flasher Suite</h2>
          <span class="subtitle">Herramientas de Microcontrolador y Cableado</span>
        </div>
      </div>
      <button class="close-btn" on:click={onClose} title="Cerrar">✕</button>
    </div>

    <!-- Tab Bar -->
    <nav class="drawer-tabs">
      <button 
        class="tab-btn" 
        class:active={activeTab === 'flasher'} 
        on:click={() => activeTab = 'flasher'}
      >
        <span>🚀</span> Flasheador
      </button>
      <button 
        class="tab-btn" 
        class:active={activeTab === 'pinout'} 
        on:click={() => activeTab = 'pinout'}
      >
        <span>🔌</span> Pines & Cableado
      </button>
      <button 
        class="tab-btn" 
        class:active={activeTab === 'drivers'} 
        on:click={() => activeTab = 'drivers'}
      >
        <span>🛠️</span> Drivers USB
      </button>
    </nav>

    <!-- Tab Content -->
    <div class="drawer-content">
      {#if activeTab === 'flasher'}
        <div class="tab-pane">
          <div class="section-card">
            <h3>Selección de Firmware</h3>
            <div class="robot-selector">
              <label class="radio-card" class:selected={selectedRobot === 'IM_16'}>
                <input type="radio" bind:group={selectedRobot} value="IM_16" disabled={isFlashing} />
                <div class="radio-content">
                  <strong>16L Ingeniero Maker</strong>
                  <span>Multiplexor CD4051 (A0..A5)</span>
                </div>
              </label>

              <label class="radio-card" class:selected={selectedRobot === 'CODEX_8'}>
                <input type="radio" bind:group={selectedRobot} value="CODEX_8" disabled={isFlashing} />
                <div class="radio-content">
                  <strong>8L Codex</strong>
                  <span>Analógico Directo (A0..A7)</span>
                </div>
              </label>
            </div>
          </div>

          <div class="section-card">
            <div class="flex-between">
              <h3>Puerto COM & Velocidad</h3>
              <button class="btn-sm btn-ghost" on:click={refreshPorts} disabled={isFlashing}>
                🔄 Actualizar Puertos
              </button>
            </div>

            <div class="form-row">
              <div class="form-group flex-1">
                <label for="com-port-select">Puerto COM</label>
                <select id="com-port-select" bind:value={selectedPort} disabled={isFlashing} class="select-input">
                  {#if availablePorts.length === 0}
                    <option value="">No hay puertos detectados</option>
                  {:else}
                    {#each availablePorts as p}
                      <option value={p}>{p}</option>
                    {/each}
                  {/if}
                </select>
              </div>

              <div class="form-group flex-1">
                <label for="baud-rate-select">Velocidad / Bootloader</label>
                <select id="baud-rate-select" bind:value={selectedBaud} disabled={isFlashing} class="select-input">
                  <option value={115200}>115200 (New Bootloader / Optiboot)</option>
                  <option value={9600}>9600 (Estándar Alternativo)</option>
                  <option value={57600}>57600 (Old Bootloader Manual)</option>
                </select>
              </div>
            </div>

            <label class="checkbox-label">
              <input type="checkbox" bind:checked={autoFallback} disabled={isFlashing} />
              <span>
                <strong>Auto-Fallback inteligente:</strong> Si falla a 115200, reintentar automáticamente a 57600 baudios (Old Bootloader).
              </span>
            </label>
          </div>

          <div class="action-box">
            <button 
              class="btn-primary-flash" 
              disabled={isFlashing || !selectedPort} 
              on:click={handleFlash}
            >
              {#if isFlashing}
                <span class="spinner"></span> Flasheando ATmega328P...
              {:else}
                🚀 Subir Firmware a {selectedPort || 'Robot'}
              {/if}
            </button>

            {#if flashStatus.message}
              <div class="status-banner {flashStatus.type}">
                {#if flashStatus.type === 'success'}✅{:else if flashStatus.type === 'error'}❌{:else}ℹ️{/if}
                {flashStatus.message}
              </div>
            {/if}
          </div>

          <!-- Terminal de logs -->
          <div class="terminal-box">
            <div class="terminal-header">
              <span>Terminal de Flasheo (avrdude)</span>
              <button class="btn-text" on:click={clearLogs}>Limpiar</button>
            </div>
            <div class="terminal-body" bind:this={terminalContainer}>
              {#if flashLogs.length === 0}
                <span class="terminal-placeholder">Esperando orden de flasheo...</span>
              {:else}
                {#each flashLogs as line}
                  <div class="terminal-line">{line}</div>
                {/each}
              {/if}
            </div>
          </div>
        </div>

      {:else if activeTab === 'pinout'}
        <div class="tab-pane">
          <div class="pinout-selector">
            <button 
              class="pill-btn" 
              class:active={pinoutModel === 'IM_16'} 
              on:click={() => pinoutModel = 'IM_16'}
            >
              16 Sensores (Ingeniero Maker)
            </button>
            <button 
              class="pill-btn" 
              class:active={pinoutModel === 'CODEX_8'} 
              on:click={() => pinoutModel = 'CODEX_8'}
            >
              8 Sensores (Codex Directo)
            </button>
          </div>

          <!-- Microcontroller Pin Diagram -->
          <div class="chip-diagram-card">
            <h4>Diagrama de Pines - Arduino Nano</h4>
            <div class="chip-container">
              <div class="chip-body">
                <span class="chip-notch"></span>
                <span class="chip-label">ATmega328P</span>
                <div class="chip-pins-left">
                  <div class="pin" title="TXD"><span class="pin-id">D1</span><span class="pin-name">TX</span></div>
                  <div class="pin" title="RXD"><span class="pin-id">D0</span><span class="pin-name">RX</span></div>
                  <div class="pin"><span class="pin-id">RST</span><span class="pin-name">Reset</span></div>
                  <div class="pin gnd"><span class="pin-id">GND</span><span class="pin-name">Tierra</span></div>
                  <div class="pin button-pin"><span class="pin-id">D2</span><span class="pin-name">Pulsador Calibración</span></div>
                  <div class="pin motor-pin"><span class="pin-id">D3</span><span class="pin-name">PWM Motor Der (31kHz)</span></div>
                  <div class="pin motor-pin"><span class="pin-id">D4</span><span class="pin-name">DIR Motor Der (IN1)</span></div>
                  <div class="pin motor-pin"><span class="pin-id">D5</span><span class="pin-name">DIR Motor Der (IN2)</span></div>
                  <div class="pin"><span class="pin-id">D6</span><span class="pin-name">Libre</span></div>
                  <div class="pin"><span class="pin-id">D7</span><span class="pin-name">Libre</span></div>
                  <div class="pin"><span class="pin-id">D8</span><span class="pin-name">Libre</span></div>
                  <div class="pin motor-pin"><span class="pin-id">D9</span><span class="pin-name">DIR Motor Izq (IN2)</span></div>
                  <div class="pin motor-pin"><span class="pin-id">D10</span><span class="pin-name">DIR Motor Izq (IN1)</span></div>
                  <div class="pin motor-pin"><span class="pin-id">D11</span><span class="pin-name">PWM Motor Izq (31kHz)</span></div>
                  <div class="pin"><span class="pin-id">D12</span><span class="pin-name">Libre</span></div>
                </div>

                <div class="chip-pins-right">
                  <div class="pin vcc"><span class="pin-name">VIN (7.4V - 12V)</span><span class="pin-id">VIN</span></div>
                  <div class="pin gnd"><span class="pin-name">GND (Tierra Común)</span><span class="pin-id">GND</span></div>
                  <div class="pin"><span class="pin-name">RST</span><span class="pin-id">RST</span></div>
                  <div class="pin vcc"><span class="pin-name">+5V (Lógica)</span><span class="pin-id">5V</span></div>
                  
                  {#if pinoutModel === 'IM_16'}
                    <div class="pin not-used"><span class="pin-name">No Usado en 16L</span><span class="pin-id">A7</span></div>
                    <div class="pin not-used"><span class="pin-name">No Usado en 16L</span><span class="pin-id">A6</span></div>
                    <div class="pin sensor-pin"><span class="pin-name">OM (Salida MUX Analógica)</span><span class="pin-id">A5</span></div>
                    <div class="pin sensor-pin"><span class="pin-name">S3 (Control Bit 3 MUX)</span><span class="pin-id">A4</span></div>
                    <div class="pin sensor-pin"><span class="pin-name">S2 (Control Bit 2 MUX)</span><span class="pin-id">A3</span></div>
                    <div class="pin sensor-pin"><span class="pin-name">S1 (Control Bit 1 MUX)</span><span class="pin-id">A2</span></div>
                    <div class="pin sensor-pin"><span class="pin-name">S0 (Control Bit 0 MUX)</span><span class="pin-id">A1</span></div>
                    <div class="pin sensor-pin"><span class="pin-name">LON (Enable IR LEDs)</span><span class="pin-id">A0</span></div>
                  {:else}
                    <div class="pin sensor-pin"><span class="pin-name">Sensor S7 (Analógico)</span><span class="pin-id">A7</span></div>
                    <div class="pin sensor-pin"><span class="pin-name">Sensor S6 (Analógico)</span><span class="pin-id">A6</span></div>
                    <div class="pin sensor-pin"><span class="pin-name">Sensor S5 (Analógico)</span><span class="pin-id">A5</span></div>
                    <div class="pin sensor-pin"><span class="pin-name">Sensor S4 (Analógico)</span><span class="pin-id">A4</span></div>
                    <div class="pin sensor-pin"><span class="pin-name">Sensor S3 (Analógico)</span><span class="pin-id">A3</span></div>
                    <div class="pin sensor-pin"><span class="pin-name">Sensor S2 (Analógico)</span><span class="pin-id">A2</span></div>
                    <div class="pin sensor-pin"><span class="pin-name">Sensor S1 (Analógico)</span><span class="pin-id">A1</span></div>
                    <div class="pin sensor-pin"><span class="pin-name">Sensor S0 (Analógico)</span><span class="pin-id">A0</span></div>
                  {/if}

                  <div class="pin"><span class="pin-name">AREF (Referencia 5V)</span><span class="pin-id">AREF</span></div>
                  <div class="pin"><span class="pin-name">3V3</span><span class="pin-id">3V3</span></div>
                  <div class="pin led-pin"><span class="pin-name">LED de Estado</span><span class="pin-id">D13</span></div>
                </div>
              </div>
            </div>
          </div>

          <!-- Pinout Details Table -->
          <div class="section-card">
            <h4>Tabla de Conexiones Físicas ({pinoutModel === 'IM_16' ? '16L Ingeniero Maker' : '8L Codex'})</h4>
            <div class="table-responsive">
              <table class="pinout-table">
                <thead>
                  <tr>
                    <th>Función</th>
                    <th>Pin Arduino</th>
                    <th>Conectar A</th>
                    <th>Detalles Eléctricos</th>
                  </tr>
                </thead>
                <tbody>
                  <tr>
                    <td><strong>Motor Derecho (PWM)</strong></td>
                    <td><span class="badge badge-purple">D3</span></td>
                    <td>Puente H (PWMA)</td>
                    <td>Timer 2 (PWM ultrasónico a 31.37 kHz)</td>
                  </tr>
                  <tr>
                    <td><strong>Motor Derecho (DIR)</strong></td>
                    <td><span class="badge badge-purple">D4 / D5</span></td>
                    <td>Puente H (AIN1 / AIN2)</td>
                    <td>Sentido de rotación adelante/freno</td>
                  </tr>
                  <tr>
                    <td><strong>Motor Izquierdo (PWM)</strong></td>
                    <td><span class="badge badge-purple">D11</span></td>
                    <td>Puente H (PWMB)</td>
                    <td>Timer 2 (PWM ultrasónico a 31.37 kHz)</td>
                  </tr>
                  <tr>
                    <td><strong>Motor Izquierdo (DIR)</strong></td>
                    <td><span class="badge badge-purple">D10 / D9</span></td>
                    <td>Puente H (BIN1 / BIN2)</td>
                    <td>Sentido de rotación adelante/freno</td>
                  </tr>
                  <tr>
                    <td><strong>Pulsador Calibración</strong></td>
                    <td><span class="badge badge-blue">D2</span></td>
                    <td>Botón a Tierra (GND)</td>
                    <td>Pull-up interno activado</td>
                  </tr>
                  <tr>
                    <td><strong>LED Indicador</strong></td>
                    <td><span class="badge badge-amber">D13</span></td>
                    <td>LED en placa</td>
                    <td>Patrones de parpadeo de calibración</td>
                  </tr>

                  {#if pinoutModel === 'IM_16'}
                    <tr>
                      <td><strong>Multiplexor (Canal Analógico)</strong></td>
                      <td><span class="badge badge-green">A5</span></td>
                      <td>Salida COM del MUX (OM)</td>
                      <td>Lectura analógica única multiplexada (0-1023)</td>
                    </tr>
                    <tr>
                      <td><strong>Líneas de Selección MUX</strong></td>
                      <td><span class="badge badge-green">A1, A2, A3, A4</span></td>
                      <td>Entradas S0, S1, S2, S3 del CD4051</td>
                      <td>Control binario para recorrer los 16 sensores</td>
                    </tr>
                    <tr>
                      <td><strong>Habilitador Emisores IR</strong></td>
                      <td><span class="badge badge-green">A0</span></td>
                      <td>Transistor / Pin LON de la regleta</td>
                      <td>HIGH = Enciende los LEDs infrarrojos</td>
                    </tr>
                  {:else}
                    <tr>
                      <td><strong>Sensores Directos 0 a 5</strong></td>
                      <td><span class="badge badge-green">A0 a A5</span></td>
                      <td>Pines S0 a S5 de regleta Codex</td>
                      <td>Lecturas analógicas independientes continuas</td>
                    </tr>
                    <tr>
                      <td><strong>Sensores Directos 6 y 7</strong></td>
                      <td><span class="badge badge-green">A6 y A7</span></td>
                      <td>Pines S6 y S7 de regleta Codex</td>
                      <td>Canales analógicos puros de ATmega328P</td>
                    </tr>
                  {/if}
                </tbody>
              </table>
            </div>
          </div>
        </div>

      {:else if activeTab === 'drivers'}
        <div class="tab-pane">
          <div class="section-card">
            <h3>Centro de Instalación de Drivers USB</h3>
            <p class="desc">
              Si tu computadora no reconoce el robot o no aparece ningún puerto COM al conectarlo por USB,
              instala el controlador correspondiente según el chip que tenga tu tarjeta Arduino Nano:
            </p>

            {#if driverStatus}
              <div class="status-banner {driverStatus.type}">
                <span>{driverStatus.message}</span>
              </div>
            {/if}

            <div class="drivers-grid">
              <!-- CH340 Card -->
              <div class="driver-card highlight-driver">
                <div class="driver-icon">🔌</div>
                <div class="driver-info">
                  <h4>WCH CH340 / CH341</h4>
                  <p>El chip más común en clones económicos de Arduino Nano.</p>
                  <span class="driver-tag">⚡ Offline Embebido (UAC)</span>
                </div>
                <button class="btn-driver btn-offline" on:click={() => handleInstallDriver('ch340')}>
                  Instalar CH340 Directo
                </button>
              </div>

              <!-- CP2102 Card -->
              <div class="driver-card">
                <div class="driver-icon">⚡</div>
                <div class="driver-info">
                  <h4>Silicon Labs CP2102 / CP2104</h4>
                  <p>Utilizado en placas profesionales y módulos ESP32 / ESP8266.</p>
                  <span class="driver-tag">Alta estabilidad</span>
                </div>
                <button class="btn-driver" on:click={() => handleInstallDriver('cp2102')}>
                  Instalar CP2102
                </button>
              </div>

              <!-- FTDI Card -->
              <div class="driver-card">
                <div class="driver-icon">💎</div>
                <div class="driver-info">
                  <h4>FTDI FT232R</h4>
                  <p>Utilizado en Arduino original y convertidores USB-Serial industriales.</p>
                  <span class="driver-tag">Original / Pro</span>
                </div>
                <button class="btn-driver" on:click={() => handleInstallDriver('ftdi')}>
                  Instalar FTDI
                </button>
              </div>
            </div>
          </div>

          <div class="section-card">
            <h4>Verificación en Windows</h4>
            <ol class="steps-list">
              <li>Conecta el robot mediante cable USB a la computadora.</li>
              <li>Abre el <strong>Administrador de Dispositivos</strong> (presiona <kbd>Win</kbd> + <kbd>X</kbd>).</li>
              <li>Despliega la sección <strong>Puertos (COM y LPT)</strong>.</li>
              <li>Debe figurar un elemento como <code>USB-SERIAL CH340 (COMx)</code> o <code>Silicon Labs CP210x (COMx)</code> sin ningún icono de advertencia amarillo.</li>
              <li>Regresa a la pestaña <strong>Flasheador</strong> o al selector principal y presiona <strong>🔄 Actualizar Puertos</strong>.</li>
            </ol>
          </div>
        </div>
      {/if}
    </div>
  </aside>
{/if}

<style>
  .drawer-backdrop {
    position: fixed;
    top: 0;
    left: 0;
    width: 100vw;
    height: 100vh;
    background: rgba(0, 0, 0, 0.55);
    backdrop-filter: blur(4px);
    z-index: 1000;
    animation: fadeIn 0.2s ease-out;
  }

  .drawer-container {
    position: fixed;
    top: 0;
    right: 0;
    width: 580px;
    max-width: 95vw;
    height: 100vh;
    background: var(--bg-card, #0f172a);
    border-left: 1px solid var(--border-highlight, #334155);
    box-shadow: -10px 0 30px rgba(0, 0, 0, 0.4);
    z-index: 1001;
    display: flex;
    flex-direction: column;
    animation: slideIn 0.25s cubic-bezier(0.16, 1, 0.3, 1);
  }

  @keyframes fadeIn {
    from { opacity: 0; }
    to { opacity: 1; }
  }

  @keyframes slideIn {
    from { transform: translateX(100%); }
    to { transform: translateX(0); }
  }

  .drawer-header {
    display: flex;
    justify-content: space-between;
    align-items: center;
    padding: 1.25rem 1.5rem;
    border-bottom: 1px solid var(--border-subtle);
    background: var(--bg-card);
  }

  .header-title {
    display: flex;
    align-items: center;
    gap: 0.75rem;
  }

  .header-title .icon {
    font-size: 1.5rem;
  }

  .header-title h2 {
    margin: 0;
    font-size: 1.15rem;
    font-weight: 700;
    color: var(--text-heading);
  }

  .header-title .subtitle {
    font-size: 0.75rem;
    color: var(--text-secondary);
  }

  .close-btn {
    background: transparent;
    border: none;
    font-size: 1.25rem;
    color: var(--text-secondary);
    cursor: pointer;
    width: 32px;
    height: 32px;
    display: flex;
    align-items: center;
    justify-content: center;
    border-radius: 7px;
    transition: background 0.15s, color 0.15s;
  }

  .close-btn:hover {
    background: var(--bg-subtle);
    color: var(--text-primary);
  }

  .drawer-tabs {
    display: flex;
    background: var(--bg-subtle);
    border-bottom: 1px solid var(--border-subtle);
    padding: 0.5rem 1rem 0;
    gap: 0.5rem;
  }

  .tab-btn {
    flex: 1;
    display: flex;
    align-items: center;
    justify-content: center;
    gap: 0.5rem;
    padding: 0.65rem 0.5rem;
    background: transparent;
    border: none;
    border-bottom: 2px solid transparent;
    color: var(--text-secondary);
    font-size: 0.85rem;
    font-weight: 600;
    cursor: pointer;
    transition: all 0.2s;
  }

  .tab-btn:hover {
    color: var(--text-primary);
  }

  .tab-btn.active {
    color: var(--accent-cyan, #0284c7);
    border-bottom-color: var(--accent-cyan, #0284c7);
  }

  .drawer-content {
    flex: 1;
    overflow-y: auto;
    padding: 1.25rem 1.5rem;
  }

  .tab-pane {
    display: flex;
    flex-direction: column;
    gap: 1.25rem;
  }

  .section-card {
    background: var(--bg-card);
    border: 1px solid var(--border-subtle);
    border-radius: 10px;
    padding: 1rem 1.25rem;
  }

  .section-card h3 {
    margin: 0 0 0.75rem 0;
    font-size: 0.95rem;
    font-weight: 600;
    color: var(--text-heading);
  }

  .section-card h4 {
    margin: 0 0 0.5rem 0;
    font-size: 0.85rem;
    font-weight: 600;
    color: var(--text-secondary, #cbd5e1);
  }

  .robot-selector {
    display: grid;
    grid-template-columns: 1fr 1fr;
    gap: 0.75rem;
  }

  .radio-card {
    display: flex;
    align-items: center;
    gap: 0.75rem;
    padding: 0.75rem 1rem;
    background: var(--track-bg);
    border: 1px solid var(--border-subtle);
    border-radius: 8px;
    cursor: pointer;
    transition: border-color 0.2s, background 0.2s;
  }

  .radio-card.selected {
    border-color: var(--accent-cyan, #0284c7);
    background: rgba(2, 132, 199, 0.08);
  }

  .radio-content strong {
    display: block;
    font-size: 0.85rem;
    color: var(--text-primary);
  }

  .radio-content span {
    font-size: 0.7rem;
    color: var(--text-secondary);
  }

  .form-row {
    display: flex;
    gap: 0.75rem;
    margin-bottom: 0.75rem;
  }

  .form-group label {
    display: block;
    font-size: 0.75rem;
    font-weight: 500;
    color: var(--text-secondary);
    margin-bottom: 0.25rem;
  }

  .select-input {
    width: 100%;
    padding: 0.5rem 0.75rem;
    background: var(--bg-input);
    border: 1px solid var(--border-input);
    border-radius: 7px;
    color: var(--text-primary);
    font-family: inherit;
    font-size: 0.8rem;
  }

  .checkbox-label {
    display: flex;
    align-items: flex-start;
    gap: 0.5rem;
    font-size: 0.75rem;
    color: var(--text-secondary, #94a3b8);
    cursor: pointer;
    margin-top: 0.5rem;
  }

  .btn-primary-flash {
    width: 100%;
    padding: 0.85rem;
    background: #0284c7;
    color: #ffffff;
    font-weight: 700;
    font-size: 0.9rem;
    border: none;
    border-radius: 8px;
    cursor: pointer;
    display: flex;
    align-items: center;
    justify-content: center;
    gap: 0.5rem;
    transition: background 0.2s, transform 0.1s;
  }

  .btn-primary-flash:hover:not(:disabled) {
    background: #0369a1;
  }

  .btn-primary-flash:disabled {
    opacity: 0.6;
    cursor: not-allowed;
  }

  .status-banner {
    margin-top: 0.75rem;
    padding: 0.65rem 0.85rem;
    border-radius: 7px;
    font-size: 0.8rem;
    display: flex;
    align-items: center;
    gap: 0.5rem;
  }

  .status-banner.success {
    background: rgba(34, 197, 94, 0.15);
    border: 1px solid #22c55e;
    color: #4ade80;
  }

  .status-banner.error {
    background: rgba(239, 68, 68, 0.15);
    border: 1px solid #ef4444;
    color: #f87171;
  }

  .status-banner.info {
    background: rgba(56, 189, 248, 0.15);
    border: 1px solid #38bdf8;
    color: #7dd3fc;
  }

  .terminal-box {
    background: #020617;
    border: 1px solid #1e293b;
    border-radius: 8px;
    overflow: hidden;
  }

  .terminal-header {
    display: flex;
    justify-content: space-between;
    align-items: center;
    padding: 0.4rem 0.75rem;
    background: #090d16;
    border-bottom: 1px solid #1e293b;
    font-size: 0.75rem;
    color: #64748b;
  }

  .terminal-body {
    padding: 0.75rem;
    height: 180px;
    overflow-y: auto;
    font-family: 'JetBrains Mono', monospace;
    font-size: 0.72rem;
    color: #94a3b8;
    line-height: 1.4;
  }

  .terminal-placeholder {
    color: #475569;
    font-style: italic;
  }

  .terminal-line {
    word-break: break-all;
    white-space: pre-wrap;
  }

  /* Chip Diagram */
  .pinout-selector {
    display: flex;
    gap: 0.5rem;
  }

  .pill-btn {
    flex: 1;
    padding: 0.5rem;
    background: var(--track-bg);
    border: 1px solid var(--border-subtle);
    border-radius: 6px;
    color: var(--text-secondary);
    font-size: 0.8rem;
    font-weight: 600;
    cursor: pointer;
    transition: all 0.15s ease;
  }

  .pill-btn:hover {
    color: var(--text-primary);
    border-color: var(--border-highlight);
  }

  .pill-btn.active {
    background: var(--pill-active-bg, #0284c7);
    color: #ffffff;
    border-color: var(--pill-active-border, #0284c7);
  }

  .chip-diagram-card {
    background: var(--bg-card);
    border: 1px solid var(--border-subtle);
    border-radius: 10px;
    padding: 1rem;
  }

  .chip-container {
    display: flex;
    justify-content: center;
    margin: 0.5rem 0;
  }

  .chip-body {
    position: relative;
    width: 320px;
    background: #111827;
    border: 2px solid #374151;
    border-radius: 8px;
    padding: 1rem 0.5rem;
    display: flex;
    justify-content: space-between;
    box-shadow: inset 0 2px 8px rgba(0,0,0,0.5);
  }

  .chip-notch {
    position: absolute;
    top: -2px;
    left: 50%;
    transform: translateX(-50%);
    width: 24px;
    height: 8px;
    background: var(--bg-main, #090d16);
    border-bottom: 2px solid #374151;
    border-radius: 0 0 12px 12px;
  }

  .chip-label {
    position: absolute;
    top: 50%;
    left: 50%;
    transform: translate(-50%, -50%) rotate(-90deg);
    font-family: 'JetBrains Mono', monospace;
    font-size: 0.75rem;
    color: #4b5563;
    letter-spacing: 2px;
  }

  .chip-pins-left, .chip-pins-right {
    display: flex;
    flex-direction: column;
    gap: 0.3rem;
  }

  .pin {
    display: flex;
    align-items: center;
    gap: 0.4rem;
    font-family: 'JetBrains Mono', monospace;
    font-size: 0.65rem;
    color: #cbd5e1;
    padding: 0.15rem 0.3rem;
    border-radius: 4px;
    background: rgba(255, 255, 255, 0.03);
  }

  .pin-id {
    font-weight: 700;
    color: #93c5fd;
    min-width: 24px;
  }

  .pin.motor-pin {
    background: rgba(168, 85, 247, 0.15);
    border-left: 2px solid #a855f7;
  }

  .pin.sensor-pin {
    background: rgba(34, 197, 94, 0.15);
    border-right: 2px solid #22c55e;
  }

  .pin.button-pin {
    background: rgba(56, 189, 248, 0.15);
    border-left: 2px solid #38bdf8;
  }

  .pin.led-pin {
    background: rgba(251, 191, 36, 0.15);
    border-right: 2px solid #fbbf24;
  }

  .pin.vcc {
    color: #f87171;
  }

  .pin.gnd {
    color: #9ca3af;
  }

  .pin.not-used {
    opacity: 0.35;
  }

  /* Table */
  .table-responsive {
    overflow-x: auto;
  }

  .pinout-table {
    width: 100%;
    border-collapse: collapse;
    font-size: 0.75rem;
  }

  .pinout-table th, .pinout-table td {
    padding: 0.5rem 0.65rem;
    text-align: left;
    border-bottom: 1px solid var(--border-subtle, #1e293b);
  }

  .pinout-table th {
    color: var(--text-secondary, #94a3b8);
    font-weight: 600;
  }

  .badge {
    display: inline-block;
    padding: 0.15rem 0.4rem;
    border-radius: 4px;
    font-family: 'JetBrains Mono', monospace;
    font-weight: 600;
    font-size: 0.7rem;
  }

  .badge-purple { background: rgba(168, 85, 247, 0.2); color: #d8b4fe; }
  .badge-green { background: rgba(34, 197, 94, 0.2); color: #86efac; }
  .badge-blue { background: rgba(56, 189, 248, 0.2); color: #7dd3fc; }
  .badge-amber { background: rgba(251, 191, 36, 0.2); color: #fde68a; }

  /* Drivers */
  .drivers-grid {
    display: flex;
    flex-direction: column;
    gap: 0.75rem;
    margin-top: 0.75rem;
  }

  .driver-card {
    display: flex;
    align-items: center;
    gap: 1rem;
    padding: 0.85rem 1rem;
    background: var(--track-bg);
    border: 1px solid var(--border-subtle);
    border-radius: 8px;
  }

  .driver-icon {
    font-size: 1.5rem;
  }

  .driver-info {
    flex: 1;
  }

  .driver-info h4 {
    margin: 0;
    font-size: 0.85rem;
    color: var(--text-heading);
  }

  .driver-info p {
    margin: 0.15rem 0 0.25rem 0;
    font-size: 0.72rem;
    color: var(--text-secondary);
  }

  .driver-tag {
    font-size: 0.65rem;
    padding: 0.1rem 0.35rem;
    background: rgba(2, 132, 199, 0.2);
    color: #38bdf8;
    border-radius: 4px;
    font-weight: 600;
  }

  .btn-driver {
    padding: 0.5rem 0.85rem;
    background: #0284c7;
    color: #ffffff;
    font-weight: 600;
    font-size: 0.78rem;
    border: none;
    border-radius: 6px;
    cursor: pointer;
    white-space: nowrap;
    transition: background 0.15s;
  }

  .btn-driver:hover {
    background: #0369a1;
  }

  .desc {
    font-size: 0.8rem;
    color: var(--text-secondary);
    line-height: 1.5;
    margin: 0 0 0.5rem 0;
  }

  .steps-list {
    margin: 0.5rem 0 0 1.25rem;
    padding: 0;
    font-size: 0.78rem;
    color: var(--text-secondary);
    line-height: 1.6;
  }

  kbd {
    background: var(--bg-subtle);
    border: 1px solid var(--border-subtle);
    border-radius: 3px;
    padding: 0.1rem 0.3rem;
    font-family: inherit;
    font-size: 0.7rem;
    color: var(--text-primary);
  }

  .flex-between {
    display: flex;
    justify-content: space-between;
    align-items: center;
  }

  .flex-1 { flex: 1; }

  .btn-sm {
    padding: 0.3rem 0.6rem;
    font-size: 0.75rem;
    border-radius: 5px;
    cursor: pointer;
  }

  .btn-ghost {
    background: transparent;
    border: 1px solid var(--border-subtle);
    color: var(--text-secondary);
    transition: all 0.15s ease;
  }

  .btn-ghost:hover {
    background: var(--bg-subtle);
    color: var(--text-primary);
    border-color: var(--border-highlight);
  }

  .btn-text {
    background: transparent;
    border: none;
    color: #38bdf8;
    font-size: 0.7rem;
    cursor: pointer;
  }

  .spinner {
    display: inline-block;
    width: 14px;
    height: 14px;
    border: 2px solid rgba(255, 255, 255, 0.3);
    border-top-color: #ffffff;
    border-radius: 50%;
    animation: spin 0.8s linear infinite;
  }

  @keyframes spin {
    to { transform: rotate(360deg); }
  }
</style>
