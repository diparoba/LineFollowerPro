# Proposal: Fluid Ultrawide Layout, Left Navigation Sidebar & Direct 4-Decimal Gain Inputs

## Why

In high-resolution, multi-monitor, and ultrawide (21:9) setups, the current desktop dashboard is constrained to a fixed `max-width: 1400px`, causing massive empty background letterboxing and preventing telemetry widgets (sensor bars, motor gauges) from expanding to full screen width. Furthermore, the top header is crowded with tool buttons, categories, and serial parameters, while tuning PD gains ($K_p$ and $K_d$) using range sliders is imprecise and clumsy for high-resolution robotics tuning where students need to enter exact 4-decimal values directly (e.g. `0.3514`, `4.2000`).

Modernizing the interface with a dedicated Left Navigation Sidebar, a 100% fluid full-width layout with an intelligent 3-column responsive grid on wide monitors ($\ge 1500\text{px}$), and replacing $K_p$/$K_d$ sliders with clean, dedicated 4-decimal numeric inputs will deliver an ergonomic, professional laboratory instrument experience.

## What Changes

- **Left Navigation Sidebar**:
  - Implement a dedicated vertical sidebar on the left containing suite navigation (Dashboard / Telemetría, Auto-Tuning, Caja Negra, Hardware & Flasher, Simulador).
  - Include lower sidebar utilities: instant Dark/Light theme toggle, live connection status indicator (ONLINE/OFFLINE), and brand/version label.
  - Declutter the top header so it focuses purely on hardware serial connectivity (COM port selector, baud rate, Connect/Disconnect), robot category selection, and race control buttons.

- **Fluid Full-Width Layout**:
  - Remove fixed `max-width: 1400px` from `.dashboard-layout` in `frontend/src/App.svelte` in favor of full 100% width with comfortable responsive padding.
  - Implement a 3-column responsive layout for wide displays ($\ge 1500\text{px}$):
    - **Column 1 (Telemetry)**: Sensor reflection bars (16L/8L) and Dual Motor PWM power gauges.
    - **Column 2 (Tuning)**: PD adjustments, dynamics sliders, RAM/EEPROM triggers, and profile save form.
    - **Column 3 (Fleet & Profiles)**: Dedicated SQLite profiles list with category filtering, instant profile loading, and database backup.
  - Preserve automatic fallback to 2 columns on medium screens (< 1500px) and 1 column on compact displays (< 1040px).

- **Direct 4-Decimal Numeric Inputs for $K_p$ and $K_d$**:
  - Remove range sliders (`<input type="range">`) from Sensibilidad ($K_p$) and Corrección ($K_d$) in `TuningPanel.svelte`.
  - Provide full-width, high-precision numeric inputs (`<input type="number" step="0.0001" min="0">`) with `JetBrains Mono` typography.
  - Preserve sliders for Base Speed, Max Speed, and Active Braking (0 to 255 PWM).

## Capabilities

### Modified Capabilities
- `full-pid-and-unbounded-gains`: Remove range sliders for $K_p$ and $K_d$, requiring dedicated 4-decimal numeric input fields while maintaining speed sliders.
- `desktop-shell`: Add requirements for fluid full-width layout with responsive 3-column organization on wide/ultrawide displays, and dedicated left navigation sidebar.

## Impact

- Affected files: `frontend/src/App.svelte`, `frontend/src/lib/Sidebar.svelte` (new), `frontend/src/lib/TuningPanel.svelte`.
- No breaking changes to serial communication protocols, packet formats, or SQLite schema.
