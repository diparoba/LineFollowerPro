# Spec Delta: desktop-shell

## ADDED Requirements

### Requirement: Fluid Full-Width Dashboard with 3-Column Ultrawide Layout
The system SHALL display a fully responsive, 100% fluid desktop interface without fixed maximum width constraints, adapting its dashboard layout to the available display dimensions using an ergonomic 3-column grid on wide monitors.

#### Scenario: Rendering on wide and ultrawide displays
- **WHEN** the application window width is $\ge 1500\text{px}$ (such as 2K, 4K, or 21:9 Ultrawide monitors)
- **THEN** system arranges the dashboard into 3 distinct columns: Column 1 for Telemetry (Sensors and Motors), Column 2 for Tuning (PD gains and speeds), and Column 3 for Fleet & Profiles (SQLite management).

#### Scenario: Rendering on laptop and standard displays
- **WHEN** the application window width is between $1040\text{px}$ and $1499\text{px}$
- **THEN** system smoothly reorganizes into 2 columns with Sensors/Motors on the left and Tuning/Profiles on the right.

#### Scenario: Rendering on compact displays
- **WHEN** the application window width is $< 1040\text{px}$
- **THEN** system stacks all sections into a single scrollable vertical column without visual overlap or clipping.

---

### Requirement: Ergonomic Left Navigation Sidebar
The system SHALL provide a dedicated left vertical navigation sidebar hosting suite-wide tools (Dashboard, Auto-Tuning, Caja Negra, Hardware Flasher, Simulador), theme toggling, and live connection status, freeing top-level horizontal space for serial connectivity and robot controls.

#### Scenario: Navigating tools via left sidebar
- **WHEN** user clicks on any tool button in the left sidebar (such as Auto-Tuning, Caja Negra, or Hardware Flasher)
- **THEN** system activates the corresponding view or modal seamlessly while maintaining live background serial streaming

#### Scenario: Toggling theme and viewing connection status in sidebar
- **WHEN** user views the lower sidebar section
- **THEN** system displays the current theme toggle and real-time serial link status (ONLINE/OFFLINE) with visual indicator
