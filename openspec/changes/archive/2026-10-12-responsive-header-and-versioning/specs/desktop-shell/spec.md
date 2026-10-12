# Spec Delta: desktop-shell

## MODIFIED Requirements

### Requirement: Fluid Full-Width Dashboard with 3-Column Ultrawide Layout
The system SHALL display a fully responsive, 100% fluid desktop interface without fixed maximum width constraints, adapting its dashboard layout to the available display dimensions using an ergonomic grid and ensuring the top header and serial control bar never compress vertically or clip essential buttons on compact or scaled displays.

#### Scenario: Rendering on wide and ultrawide displays
- **WHEN** the application window width is $\ge 1500\text{px}$ (such as 2K, 4K, or 21:9 Ultrawide monitors)
- **THEN** system arranges the dashboard into 3 distinct columns: Column 1 for Telemetry (Sensors and Motors), Column 2 for Tuning (PD gains and speeds), and Column 3 for Fleet & Profiles (SQLite management).

#### Scenario: Rendering on laptop and standard displays
- **WHEN** the application window width is between $1040\text{px}$ and $1499\text{px}$ or viewed on scaled laptop screens ($\le 800\text{px}$ vertical viewport)
- **THEN** system smoothly reorganizes into 2 columns with Sensors/Motors on the left and Tuning/Profiles on the right, keeping the top category header and serial control bar uncompressed with `flex-shrink: 0`, and allowing vertical scroll in the workspace.

#### Scenario: Rendering on compact displays
- **WHEN** the application window width is $< 1040\text{px}$
- **THEN** system stacks all sections into a single scrollable vertical column without visual overlap or clipping.

#### Scenario: Non-collapsible serial control bar and prominent actions
- **WHEN** the dashboard renders on any screen resolution or browser zoom factor
- **THEN** the COM selector, baud rate select, and connection button remain fully visible with comfortable click/touch padding (minimum 36px control height) and never shrink into an illegible sliver.
