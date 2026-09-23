# Spec Delta

## Purpose

Define el motor de ejecución offline de controladores USB con elevación de privilegios, la persistencia autónoma de la base de datos para memorias flash USB con función de respaldo en un clic, y el asistente analítico de auto-sintonización PID basado en geometría y voltaje.

## ADDED Requirements

### Requirement: Offline Local Driver Installer with UAC Elevation
The desktop application SHALL bundle the USB driver installer executable (`CH341SER.EXE`) inside its local resources and SHALL launch it directly using Windows Administrator privileges (`runas`) without requiring an internet connection or redirecting to external web browsers.

#### Scenario: User clicks Install CH340 button
- **WHEN** the user opens the Drivers tab in the Hardware Drawer and clicks "Instalar CH340"
- **THEN** the application verifies the local presence of `resources/drivers/CH341SER.EXE` and launches the process with Windows UAC elevation prompt directly on the desktop.

#### Scenario: Offline execution without internet
- **WHEN** the host computer has no active internet connection
- **THEN** the driver installer executes successfully from the local flash/application directory without failing or hanging.

### Requirement: True USB Flash Portability for SQLite Database
The desktop application SHALL anchor the SQLite database (`follower.db`) to the directory of the running executable (`current_exe`) ensuring that all car profiles, tuning configurations, and logs reside on the portable flash drive regardless of the working directory or shortcut used to launch the app.

#### Scenario: Running application from a USB drive
- **WHEN** the application executable is launched from a removable drive or arbitrary folder path
- **THEN** the system resolves and opens `follower.db` directly in the same folder as the executable, preventing file creation in system directories or AppData.

### Requirement: One-Click Database Backup
The desktop application SHALL provide a one-click backup action that creates a timestamped duplicate of the active SQLite database directly in the application directory.

#### Scenario: Creating a database backup
- **WHEN** the user clicks "Crear Backup" in the Profile Manager interface
- **THEN** the system flushes any pending SQLite transactions, clones `follower.db` to `backup_follower_YYYY-MM-DD_HHmmss.db` in the same directory, and notifies the user with a confirmation banner.

### Requirement: Analytical PID Auto-Tuning Assistant
The desktop application SHALL provide an interactive auto-tuning assistant modal that calculates recommended $K_p$, $K_d$, base speed, maximum speed, and brake speed based on physical robot geometry, battery supply voltage, sensor category, and chosen driving profile.

#### Scenario: Calculating gains from geometry and voltage
- **WHEN** the user inputs battery voltage ($V_{\text{in}}$), sensor-to-axle distance ($L$), wheel track width ($W$), desired base speed ($V_{\text{base}}$), and selects a driving profile (Conservador, Equilibrado, Agresivo)
- **THEN** the system mathematically computes recommended values for $K_p$ and $K_d$, scales them to the active sensor category (16L vs 8L), and renders an interactive step response simulation preview curve.

#### Scenario: Applying auto-tuning values to active configuration
- **WHEN** the user clicks "Aplicar al Robot" in the Auto-Tuning modal
- **THEN** the calculated gains are loaded into the main tuning sliders and immediately transmitted to the robot's RAM via `$PID` command.
