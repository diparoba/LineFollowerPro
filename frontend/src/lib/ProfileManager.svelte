<script>
  import { createEventDispatcher } from 'svelte';
  import { backupDatabase } from './tauriBridge.js';

  export let profiles = [];
  export let activeCategory = 'ALL';

  const dispatch = createEventDispatcher();

  let filterCategory = 'ALL';
  let backupStatus = '';

  $: if (activeCategory && activeCategory !== 'ALL') {
    filterCategory = activeCategory;
  }

  $: filteredProfiles = profiles.filter(p => {
    if (filterCategory === 'ALL') return true;
    return (p.carCategory || 'IM_16') === filterCategory;
  });

  function selectProfile(p) {
    dispatch('loadProfile', p);
  }

  function deleteProfile(id) {
    if (confirm('¿Deseas eliminar este perfil de SQLite?')) {
      dispatch('deleteProfile', id);
    }
  }

  async function handleBackup() {
    backupStatus = 'Generando copia de seguridad...';
    const now = new Date();
    const pad = (n) => String(n).padStart(2, '0');
    const ts = `${now.getFullYear()}-${pad(now.getMonth()+1)}-${pad(now.getDate())}_${pad(now.getHours())}${pad(now.getMinutes())}${pad(now.getSeconds())}`;
    const filename = `backup_follower_${ts}.db`;

    const res = await backupDatabase(filename);
    if (res.success) {
      backupStatus = `✓ Copia creada en flash: ${res.filename}`;
      setTimeout(() => { backupStatus = ''; }, 6000);
    } else {
      backupStatus = `Error: ${res.message || 'Fallo al respaldar'}`;
    }
  }

  function getForkName(mode) {
    if (mode === 1) return '← Izq';
    if (mode === 2) return 'Der →';
    return 'Recto';
  }
</script>

<div class="precision-card profile-card">
  <div class="card-header">
    <div class="header-left">
      <span class="profile-badge precision-chip">SQLITE DB</span>
      <h3>Perfiles & Flota</h3>
      <button 
        class="btn-backup precision-chip" 
        on:click={handleBackup} 
        title="Crear un punto de restauración de follower.db en la memoria flash"
      >
        💾 Backup DB
      </button>
    </div>
    <div class="filter-pills">
      <button 
        class="filter-pill {filterCategory === 'ALL' ? 'active' : ''}" 
        on:click={() => filterCategory = 'ALL'}
      >Todos ({profiles.length})</button>
      <button 
        class="filter-pill {filterCategory === 'IM_16' ? 'active' : ''}" 
        on:click={() => filterCategory = 'IM_16'}
      >🏎️ 16L</button>
      <button 
        class="filter-pill {filterCategory === 'CODEX_8' ? 'active' : ''}" 
        on:click={() => filterCategory = 'CODEX_8'}
      >⚡ 8L</button>
    </div>
  </div>

  {#if backupStatus}
    <div class="backup-banner precision-mono">
      {backupStatus}
    </div>
  {/if}

  {#if filteredProfiles.length === 0}
    <div class="empty-state">
      No hay perfiles registrados en esta categoría.
    </div>
  {:else}
    <div class="profiles-list">
      {#each filteredProfiles as p}
        <div class="profile-item">
          <div class="profile-info">
            <div class="name-row">
              <span class="category-chip precision-chip {p.carCategory === 'CODEX_8' ? 'codex' : 'im'}">
                {p.carCategory === 'CODEX_8' ? 'CODEX 8' : 'IM 16'}
              </span>
              <span class="car-name-tag precision-mono">🚗 {p.carName || 'Carro 1'}</span>
              <span class="profile-name">{p.name}</span>
            </div>
            <div class="profile-meta precision-mono">
              <span>Kp: <strong>{p.kp.toFixed(4)}</strong></span>
              <span>Kd: <strong>{p.kd.toFixed(2)}</strong></span>
              <span>Base: <strong>{p.baseSpeed}</strong></span>
              <span>Freno: <strong>{p.brakeSpeed}</strong></span>
              <span>Bifurc: <strong>{getForkName(p.forkMode)}</strong></span>
            </div>
          </div>
          <div class="profile-actions">
            <button class="precision-btn btn-load" on:click={() => selectProfile(p)} title="Cargar este perfil a los sliders y al robot">
              ⚡ Cargar
            </button>
            <button class="precision-btn btn-delete" on:click={() => deleteProfile(p.id)} title="Eliminar de SQLite">
              ✕
            </button>
          </div>
        </div>
      {/each}
    </div>
  {/if}
</div>

<style>
  .profile-card {
    padding: 0.85rem;
  }

  .card-header {
    display: flex;
    justify-content: space-between;
    align-items: center;
    margin-bottom: 0.65rem;
    gap: 0.5rem;
    flex-wrap: wrap;
  }

  .header-left {
    display: flex;
    align-items: center;
    gap: 0.45rem;
  }

  .profile-badge {
    background: var(--chip-green-bg);
    border: 1px solid var(--chip-green-border);
    color: var(--chip-green-text);
  }

  .btn-backup {
    background: rgba(2, 132, 199, 0.15);
    border: 1px solid rgba(2, 132, 199, 0.4);
    color: #38bdf8;
    cursor: pointer;
    font-size: 0.68rem;
    font-weight: 700;
    transition: all 0.15s ease;
  }

  .btn-backup:hover {
    background: #0284c7;
    color: #ffffff;
    border-color: #0284c7;
  }

  .backup-banner {
    background: rgba(16, 185, 129, 0.12);
    border: 1px solid rgba(16, 185, 129, 0.35);
    color: #10b981;
    font-size: 0.72rem;
    padding: 0.3rem 0.6rem;
    border-radius: 6px;
    margin-bottom: 0.6rem;
  }

  .card-header h3 {
    margin: 0;
    font-size: 0.95rem;
    color: var(--text-heading);
    font-weight: 700;
  }

  .filter-pills {
    display: flex;
    gap: 0.25rem;
  }

  .filter-pill {
    font-family: var(--font-geo);
    background: var(--pill-bg);
    border: 1px solid var(--pill-border);
    color: var(--pill-text);
    padding: 0.2rem 0.45rem;
    border-radius: var(--radius-chip);
    font-size: 0.72rem;
    font-weight: 600;
    cursor: pointer;
    transition: all 0.15s ease;
  }

  .filter-pill.active {
    background: var(--pill-active-bg) !important;
    border-color: var(--pill-active-border) !important;
    color: var(--pill-active-text) !important;
    font-weight: 700;
  }

  .empty-state {
    text-align: center;
    padding: 1.5rem 0.5rem;
    color: var(--text-muted);
    font-size: 0.8rem;
  }

  .profiles-list {
    display: flex;
    flex-direction: column;
    gap: 0.45rem;
    max-height: 220px;
    overflow-y: auto;
  }

  .profile-item {
    background: var(--track-bg);
    border: 1px solid var(--border-subtle);
    border-radius: var(--radius-btn);
    padding: 0.55rem 0.75rem;
    display: flex;
    justify-content: space-between;
    align-items: center;
    gap: 0.5rem;
  }

  .profile-item:hover {
    border-color: var(--border-highlight);
  }

  .name-row {
    display: flex;
    align-items: center;
    gap: 0.4rem;
    margin-bottom: 0.2rem;
    flex-wrap: wrap;
  }

  .category-chip {
    font-size: 0.62rem;
    padding: 0.1rem 0.35rem;
  }

  .category-chip.im {
    background: var(--chip-blue-bg);
    color: var(--chip-blue-text);
    border: 1px solid var(--chip-blue-border);
  }

  .category-chip.codex {
    background: rgba(124, 58, 237, 0.15);
    color: var(--accent-purple);
    border: 1px solid rgba(124, 58, 237, 0.4);
  }

  .car-name-tag {
    font-size: 0.72rem;
    color: var(--text-primary);
    background: var(--bg-card);
    border: 1px solid var(--border-subtle);
    padding: 0.1rem 0.35rem;
    border-radius: 3px;
    font-weight: 600;
  }

  .profile-name {
    font-weight: 700;
    color: var(--text-heading);
    font-size: 0.82rem;
  }

  .profile-meta {
    display: flex;
    gap: 0.55rem;
    font-size: 0.7rem;
    color: var(--text-secondary);
    flex-wrap: wrap;
  }

  .profile-meta strong {
    color: var(--accent-cyan);
  }

  .profile-actions {
    display: flex;
    gap: 0.35rem;
  }

  .btn-load {
    background: var(--chip-blue-bg);
    border: 1px solid var(--chip-blue-border);
    color: var(--chip-blue-text);
    padding: 0.3rem 0.6rem;
    font-size: 0.75rem;
    font-weight: 700;
  }

  .btn-load:hover {
    background: #0284c7;
    color: #ffffff;
  }

  .btn-delete {
    background: var(--bg-card);
    border: 1px solid var(--border-subtle);
    color: var(--accent-rose);
    padding: 0.3rem 0.5rem;
    font-size: 0.75rem;
  }

  .btn-delete:hover {
    background: rgba(225, 29, 72, 0.15);
    border-color: var(--accent-rose);
  }
</style>
