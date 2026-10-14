# Spec Delta: desktop-shell

## ADDED Requirements

### Requirement: Interactive Web Serial Port Authorization
La interfaz web SHALL proporcionar un control interactivo de usuario para solicitar y conceder permisos de acceso a dispositivos USB (Arduino Nano y microcontroladores ESP32) mediante la Web Serial API del navegador.

#### Scenario: Requesting USB device authorization in web browser
- **WHEN** la aplicación se ejecuta en entorno navegador web y el usuario presiona el botón "🔑 Conceder Permiso USB" o "🌐 Conectar Web Serial"
- **THEN** el sistema invoca `navigator.serial.requestPort()` desplegando el selector de dispositivos nativo del navegador y añade el puerto autorizado a la lista disponible.

#### Scenario: Connecting to Arduino or ESP32 via granted port
- **WHEN** el usuario selecciona un puerto autorizado por el navegador y presiona "Conectar"
- **THEN** el sistema abre la conexión Web Serial en el hilo de la página, activa la recepción de telemetría y permite transmisión de comandos en tiempo real.

---

### Requirement: Reflective Floor UI Selector and SQLite Persistence
El panel de sintonización y la base de datos local SQLite SHALL permitir al usuario conmutar entre pista normal y piso reflectante y almacenar esta preferencia en el perfil del robot.

#### Scenario: Toggling reflective mode in tuning panel
- **WHEN** el usuario alterna el selector de modo de superficie entre "Piso Normal (Mate)" y "Piso Reflectante (Antirreflejos)"
- **THEN** la interfaz actualiza la configuración activa y transmite el nuevo valor mediante la trama `$PID` extendida o comandos de control serial.

#### Scenario: Persisting reflective mode in SQLite profiles
- **WHEN** el usuario guarda un perfil de configuración en `follower.db`
- **THEN** el motor SQLite almacena el valor de `reflective_mode` (0 o 1) en la tabla `profiles` garantizando su restauración al seleccionar el perfil.

---

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
- **WHEN** the dashboard renders on any screen resolution, compact viewport, or browser zoom factor
- **THEN** the category header, COM selector, baud rate select, and connection button maintain automatic intrinsic height (`min-height: max-content`), `flex-shrink: 0`, and visible boundaries, preventing cards from collapsing into thin unreadable strips.
