# Tasks: Fluid Ultrawide Layout, Left Navigation Sidebar & Direct 4-Decimal Gain Inputs

## 1. Direct 4-Decimal Numeric Inputs for Gains

- [x] 1.1 In `frontend/src/lib/TuningPanel.svelte`, remove the range sliders (`<input type="range">`) for Sensibilidad ($K_p$) and Corrección ($K_d$)
- [x] 1.2 In `frontend/src/lib/TuningPanel.svelte`, update the $K_p$ and $K_d$ numeric inputs to be full-width, clean, and styled with `step="0.0001"`, `min="0"`, and `JetBrains Mono` font
- [x] 1.3 Verify that typing values like `0.3514` and `4.2000` updates the internal config cleanly without slider clipping

## 2. Left Navigation Sidebar

- [x] 2.1 Create `frontend/src/lib/Sidebar.svelte` with suite brand, navigation items (Dashboard, Auto-Tuning, Caja Negra, Hardware Flasher, Simulador), theme switch, and live connection status
- [x] 2.2 In `frontend/src/App.svelte`, wrap layout in a flex container containing `Sidebar.svelte` and `.main-workspace`
- [x] 2.3 Wire up sidebar events to trigger modals (Auto-Tuning, Caja Negra, Hardware Drawer, Simulador) and theme toggling

## 3. Fluid Full-Width Layout & 3-Column Grid

- [x] 3.1 In `frontend/src/App.svelte`, remove the fixed `max-width: 1400px` limitation from `.dashboard-layout` and apply full fluid width with responsive padding
- [x] 3.2 In `frontend/src/App.svelte`, reorganize `.content-grid` to support 3 distinct columns: Telemetry (Col 1), Tuning PD (Col 2), and Profiles & Fleet (Col 3)
- [x] 3.3 Implement responsive CSS media queries: 3 columns for $\ge 1500\text{px}$, 2 columns for $1040\text{px}-1499\text{px}$, and 1 column for $< 1040\text{px}$
- [x] 3.4 Reorganize the top bar in `.main-workspace` to focus strictly on COM port connection, category selection, and race control commands

## 4. End-to-End Verification

- [x] 4.1 Run frontend build check (`npm run build` or Vite build in `frontend/`) to ensure no compilation or syntax errors
- [x] 4.2 Verify visual presentation across small, standard, and ultrawide resolutions with functional sidebar navigation and direct gain editing
