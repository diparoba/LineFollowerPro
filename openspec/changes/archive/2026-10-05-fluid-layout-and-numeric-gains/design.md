# Design: Fluid Ultrawide Layout, Left Navigation Sidebar & Direct 4-Decimal Gain Inputs

## Context

The desktop client is built on Svelte + Vite with Tauri v2. Currently, `frontend/src/App.svelte` wraps the entire dashboard in a `.dashboard-layout` container with `max-width: 1400px; margin: 0 auto;`. On monitors wider than 1400px (such as 1080p full-window, 1440p, 4K, and 21:9 Ultrawide displays), this produces large blank borders on both sides and at the bottom.

Furthermore, all suite tool buttons (Auto-Tuning, Caja Negra, Hardware Flasher, Theme Toggle, Online status) are clustered in the header alongside car selection and category tabs, causing horizontal clutter. In addition, `frontend/src/lib/TuningPanel.svelte` renders range sliders (`<input type="range">`) for $K_p$ and $K_d$, which are imprecise for robotics tuning where exact 4-decimal fractions are needed.

## Goals / Non-Goals

**Goals:**
- Introduce a dedicated left navigation sidebar (`frontend/src/lib/Sidebar.svelte`) that houses suite navigation items, theme switcher, and serial status.
- Provide a 100% fluid full-width layout that automatically adapts to any monitor resolution.
- On large and ultrawide screens ($\ge 1500\text{px}$), render a balanced 3-column dashboard:
  - Column 1: Telemetry (Sensors IM-16/CODEX-8 and Motor visualizers).
  - Column 2: PD Tuning (Ganancias $K_p$/$K_d$, Speeds, RAM/EEPROM actions, Profile Save form).
  - Column 3: Profiles & Fleet (SQLite profile list with category filter and database backup).
- On medium screens (< 1500px), cleanly fold into a 2-column layout (Left: Telemetry, Right: Tuning + Profiles).
- On compact screens (< 1040px), cleanly stack into a single column.
- Remove range sliders from $K_p$ and $K_d$ in `TuningPanel.svelte`, replacing them with full-width direct numeric input fields supporting 4-decimal precision (`step="0.0001"`, `min="0"`).
- Maintain range sliders for Base Speed, Max Speed, and Active Braking.

**Non-Goals:**
- Do not alter the backend serial protocol (`$PID,kp,kd,base,max,brake,fork,color`).
- Do not modify the SQLite schema or profile storage model.
- Do not introduce quick increment buttons for gains (per explicit user requirement).

## Decisions

### 1. Left Navigation Sidebar Architecture (`Sidebar.svelte`)
- Structure the application layout as a two-pane flex view:
  - **Left Pane (`Sidebar.svelte`)**: Fixed-width (~220px on desktop) vertical navbar with:
    - Top: Brand logo (`🏎️ LineFollower Pro` + `v1.0.0`).
    - Middle: Navigation items (📊 Dashboard, 🎯 Auto-Tuning, 🔴 Caja Negra, 🛠️ Hardware & Flasher, 🧪 Simulador).
    - Bottom: Theme switcher button (`☀️ Claro` / `🌙 Oscuro`), connection status badge (ONLINE/OFFLINE), and fleet car indicator.
  - **Right Pane (`.main-workspace`)**: Takes `flex: 1`, occupying 100% remaining width. Contains the top hardware bar (ports, baud, connect, category, calibration/start/stop) and the responsive `.content-grid`.
- **Alternative Considered**: Dropdown menus in the top bar. Rejected because vertical sidebars provide superior ergonomics, modern desktop feel, and single-click access for students.

### 2. Adaptive 3-Column Responsive CSS Grid
- **Approach**: Configure `.content-grid` in `App.svelte` using CSS media queries:
  - `@media (min-width: 1500px)`: `grid-template-columns: 1.15fr 1fr 1fr;`
  - `@media (max-width: 1499px)`: `grid-template-columns: 1.1fr 1fr;`
  - `@media (max-width: 1040px)`: `grid-template-columns: 1fr;`
- In the template markup, `<ProfileManager />` renders as the third column child element. When the grid drops to 2 columns, CSS grid or conditional placement ensures `ProfileManager` flows naturally into the right column below `TuningPanel`.

### 3. Direct 4-Decimal Numeric Inputs for $K_p$ and $K_d$
- **Approach**: Replace the `.slider-row` containing `<input type="range">` and `<input type="number">` with a single full-width styled numeric input `<input type="number" step="0.0001" min="0" bind:value={kp} class="gain-input precision-mono">`.
- Apply prominent styling with clear focus rings, dark/light theme contrast, and `JetBrains Mono` font for instant readability.

## Risks / Trade-offs

- [Risk]: On compact screens (< 1000px), a fixed sidebar might reduce workspace width.
  → Mitigation: In narrow media queries, the sidebar collapses into a top compact bar or icon-only strip, preserving content space.
