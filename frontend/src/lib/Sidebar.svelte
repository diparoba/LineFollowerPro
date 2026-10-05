<script>
  import { createEventDispatcher } from 'svelte';

  export let currentView = 'dashboard';
  export let theme = 'dark';
  export let isConnected = false;
  export let activeCategory = 'IM_16';
  export let activeCarName = 'Carro 1';

  const dispatch = createEventDispatcher();

  function selectNav(view) {
    if (view === 'dashboard') {
      currentView = 'dashboard';
      dispatch('navigate', 'dashboard');
    } else if (view === 'autotune') {
      dispatch('openAutoTuning');
    } else if (view === 'blackbox') {
      dispatch('openBlackBox');
    } else if (view === 'hardware') {
      dispatch('openHardware');
    } else if (view === 'sim') {
      dispatch('openSim');
    }
  }

  function handleToggleTheme() {
    dispatch('toggleTheme');
  }
</script>

<aside class="sidebar-container precision-card">
  <!-- Brand Header -->
  <div class="sidebar-brand">
    <div class="brand-top">
      <span class="brand-icon">🏎️</span>
      <div class="brand-text">
        <h1 class="brand-title">LineFollower</h1>
        <div class="brand-subrow">
          <span class="brand-badge">PRO</span>
          <span class="precision-chip version-chip">v1.0.0</span>
        </div>
      </div>
    </div>
    <p class="brand-tagline">Telemetry & Tuning Suite</p>
  </div>

  <!-- Suite Navigation Menu -->
  <nav class="sidebar-nav">
    <div class="nav-section-label">HERRAMIENTAS</div>

    <button 
      class="nav-item {currentView === 'dashboard' ? 'active' : ''}" 
      on:click={() => selectNav('dashboard')}
      title="Vista Principal de Telemetría, Sintonización y Perfiles"
    >
      <span class="nav-icon">📊</span>
      <span class="nav-label">Dashboard</span>
    </button>

    <button 
      class="nav-item" 
      on:click={() => selectNav('autotune')}
      title="Asistente de Auto-Sintonización Analítica PID"
    >
      <span class="nav-icon">🎯</span>
      <span class="nav-label">Auto-Tuning</span>
      <span class="nav-badge pulse-indigo">PID</span>
    </button>

    <button 
      class="nav-item" 
      on:click={() => selectNav('blackbox')}
      title="Caja Negra, Historial de Vueltas y Exportación CSV"
    >
      <span class="nav-icon">🔴</span>
      <span class="nav-label">Caja Negra</span>
      <span class="nav-badge pulse-red">LOG</span>
    </button>

    <button 
      class="nav-item" 
      on:click={() => selectNav('hardware')}
      title="Flasheador de Firmware ESP32 / Arduino Nano y Esquema de Pines"
    >
      <span class="nav-icon">🛠️</span>
      <span class="nav-label">Hardware & Flasher</span>
    </button>

    <button 
      class="nav-item" 
      on:click={() => selectNav('sim')}
      title="Simulador de Pista 2D con Física Dinámica"
    >
      <span class="nav-icon">🧪</span>
      <span class="nav-label">Simulador 2D</span>
    </button>
  </nav>

  <!-- Sidebar Footer: Info, Status & Theme -->
  <div class="sidebar-footer">
    <!-- Active Robot Card -->
    <div class="robot-info-card">
      <div class="robot-label">ROBOT SELECCIONADO</div>
      <div class="robot-val-row">
        <span class="robot-name precision-mono">{activeCarName}</span>
        <span class="precision-chip cat-chip {activeCategory === 'CODEX_8' ? 'codex' : 'im'}">
          {activeCategory === 'CODEX_8' ? '8L' : '16L'}
        </span>
      </div>
    </div>

    <!-- Live Connection Status -->
    <div class="connection-status-pill {isConnected ? 'status-online' : 'status-offline'}">
      <span class="live-dot {isConnected ? 'active' : ''}"></span>
      <span class="status-text precision-mono">{isConnected ? 'SERIAL ONLINE' : 'DESCONECTADO'}</span>
    </div>

    <!-- Theme Switcher Button -->
    <button class="theme-toggle-btn precision-btn" on:click={handleToggleTheme} title="Alternar Modo Oscuro / Modo Claro">
      <span class="theme-icon">{theme === 'dark' ? '☀️' : '🌙'}</span>
      <span class="theme-label">{theme === 'dark' ? 'Modo Claro' : 'Modo Oscuro'}</span>
    </button>
  </div>
</aside>

<style>
  .sidebar-container {
    width: 230px;
    min-width: 230px;
    background: var(--bg-card);
    border-right: 1px solid var(--border-subtle);
    display: flex;
    flex-direction: column;
    justify-content: space-between;
    padding: 0.9rem 0.75rem;
    box-sizing: border-box;
    height: 100%;
    border-radius: var(--radius-card);
  }

  .sidebar-brand {
    padding-bottom: 0.85rem;
    border-bottom: 1px solid var(--border-subtle);
    margin-bottom: 0.85rem;
  }

  .brand-top {
    display: flex;
    align-items: center;
    gap: 0.55rem;
  }

  .brand-icon {
    font-size: 1.6rem;
    filter: drop-shadow(0 2px 4px rgba(0, 0, 0, 0.2));
  }

  .brand-title {
    margin: 0;
    font-size: 1.05rem;
    font-weight: 700;
    letter-spacing: -0.02em;
    background: linear-gradient(90deg, var(--text-heading), var(--accent-cyan));
    -webkit-background-clip: text;
    -webkit-text-fill-color: transparent;
    line-height: 1.2;
  }

  .brand-subrow {
    display: flex;
    align-items: center;
    gap: 0.35rem;
    margin-top: 0.1rem;
  }

  .brand-badge {
    font-size: 0.65rem;
    font-weight: 800;
    letter-spacing: 0.08em;
    color: var(--accent-cyan);
  }

  .version-chip {
    font-size: 0.6rem;
    padding: 0.05rem 0.3rem;
    background: var(--chip-blue-bg);
    border: 1px solid var(--chip-blue-border);
    color: var(--chip-blue-text);
  }

  .brand-tagline {
    margin: 0.35rem 0 0 0;
    font-size: 0.68rem;
    color: var(--text-muted);
    font-weight: 500;
  }

  .sidebar-nav {
    display: flex;
    flex-direction: column;
    gap: 0.35rem;
    flex: 1;
  }

  .nav-section-label {
    font-size: 0.65rem;
    font-weight: 700;
    letter-spacing: 0.06em;
    color: var(--text-muted);
    margin-bottom: 0.25rem;
    padding-left: 0.4rem;
  }

  .nav-item {
    font-family: var(--font-geo);
    display: flex;
    align-items: center;
    gap: 0.65rem;
    background: transparent;
    border: 1px solid transparent;
    color: var(--text-secondary);
    padding: 0.55rem 0.65rem;
    border-radius: var(--radius-btn);
    font-size: 0.82rem;
    font-weight: 600;
    cursor: pointer;
    transition: all 0.15s ease;
    text-align: left;
    width: 100%;
    box-sizing: border-box;
  }

  .nav-item:hover {
    background: var(--bg-hover);
    color: var(--text-heading);
    border-color: var(--border-subtle);
  }

  .nav-item.active {
    background: var(--pill-bg);
    border-color: var(--border-highlight);
    color: var(--text-heading);
    box-shadow: 0 2px 6px rgba(0, 0, 0, 0.2);
  }

  .nav-icon {
    font-size: 1rem;
    display: flex;
    align-items: center;
    justify-content: center;
    width: 20px;
  }

  .nav-label {
    flex: 1;
  }

  .nav-badge {
    font-size: 0.58rem;
    font-weight: 700;
    padding: 0.1rem 0.35rem;
    border-radius: var(--radius-chip);
  }

  .pulse-indigo {
    background: rgba(99, 102, 241, 0.2);
    border: 1px solid rgba(99, 102, 241, 0.4);
    color: #818cf8;
  }

  .pulse-red {
    background: rgba(239, 68, 68, 0.2);
    border: 1px solid rgba(239, 68, 68, 0.4);
    color: #ef4444;
  }

  .sidebar-footer {
    display: flex;
    flex-direction: column;
    gap: 0.55rem;
    padding-top: 0.85rem;
    border-top: 1px solid var(--border-subtle);
  }

  .robot-info-card {
    background: var(--track-bg);
    border: 1px solid var(--border-subtle);
    border-radius: var(--radius-btn);
    padding: 0.45rem 0.55rem;
  }

  .robot-label {
    font-size: 0.6rem;
    font-weight: 700;
    color: var(--text-muted);
    letter-spacing: 0.05em;
  }

  .robot-val-row {
    display: flex;
    justify-content: space-between;
    align-items: center;
    margin-top: 0.2rem;
  }

  .robot-name {
    font-size: 0.78rem;
    font-weight: 700;
    color: var(--text-primary);
  }

  .cat-chip {
    font-size: 0.62rem;
    padding: 0.1rem 0.35rem;
  }

  .cat-chip.codex {
    background: rgba(217, 119, 6, 0.15);
    border: 1px solid rgba(217, 119, 6, 0.4);
    color: #f59e0b;
  }

  .cat-chip.im {
    background: var(--chip-blue-bg);
    border: 1px solid var(--chip-blue-border);
    color: var(--chip-blue-text);
  }

  .connection-status-pill {
    display: flex;
    align-items: center;
    gap: 0.45rem;
    padding: 0.4rem 0.55rem;
    border-radius: var(--radius-btn);
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
    width: 7px;
    height: 7px;
    border-radius: 50%;
    background: #ef4444;
  }

  .live-dot.active {
    background: #10b981;
    box-shadow: 0 0 8px #10b981;
  }

  .status-text {
    font-size: 0.72rem;
    font-weight: 700;
  }

  .theme-toggle-btn {
    display: flex;
    align-items: center;
    justify-content: center;
    gap: 0.45rem;
    width: 100%;
    padding: 0.45rem;
    font-size: 0.76rem;
    font-weight: 600;
    background: var(--bg-card);
    border: 1px solid var(--border-subtle);
    color: var(--text-primary);
    cursor: pointer;
    border-radius: var(--radius-btn);
    transition: all 0.15s ease;
  }

  .theme-toggle-btn:hover {
    background: var(--bg-hover);
    border-color: var(--border-highlight);
  }

  @media (max-width: 820px) {
    .sidebar-container {
      width: 100%;
      height: auto;
      border-right: none;
      border-bottom: 1px solid var(--border-subtle);
    }
  }
</style>
