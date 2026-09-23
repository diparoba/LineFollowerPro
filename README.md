# Seguidor de Línea 16 Sensores - Sistema de Control y Telemetría

Sistema integral para robot seguidor de línea de alto rendimiento con barra de 16 sensores infrarrojos (*Ingeniero Maker*), microcontrolador Arduino Nano (ATmega328P), algoritmo de control PD sub-sensor optimizado, y suite de configuración en tiempo real compuesta por un backend en **C# (.NET 10)** con **SQLite** y un frontend reactivo en **Svelte**.

---

## 1. Conexiones de Hardware

### 1.1. Sensor de 16 Líneas (Ingeniero Maker)
| Pin Sensor | Pin Arduino Nano | Descripción |
|---|---|---|
| **LON** | `A0` (D14) | Control de encendido de emisores infrarrojos |
| **S0** | `A1` (D15) | Selector de canal multiplexor bit 0 |
| **S1** | `A2` (D16) | Selector de canal multiplexor bit 1 |
| **S2** | `A3` (D17) | Selector de canal multiplexor bit 2 |
| **S3** | `A4` (D18) | Selector de canal multiplexor bit 3 |
| **OM** | `A5` (D19) | Salida analógica multiplexada (Lectura de sensores) |
| **VCC** | `5V` | Alimentación positiva |
| **GND** | `GND` | Tierra común |

### 1.2. Usuario e Indicadores
| Componente | Pin Arduino Nano | Descripción |
|---|---|---|
| **Pulsador** | `D2` | Conectado a GND con Pull-Up interno activo |
| **LED Estado** | `D13` | LED integrado en la placa Nano |

### 1.3. Puente H (Driver de Motores)
| Función | Pin Arduino Nano | Timer / Modo |
|---|---|---|
| **PWMA (Izq)** | `D3` | Timer 2 (OC2B) - 31.37 kHz ultrasónico |
| **A_IN1 (Izq)** | `D4` | Dirección 1 Motor Izquierdo |
| **A_IN2 (Izq)** | `D5` | Dirección 2 Motor Izquierdo |
| **PWMB (Der)** | `D11` | Timer 2 (OC2A) - 31.37 kHz ultrasónico |
| **B_IN1 (Der)** | `D10` | Dirección 1 Motor Derecho |
| **B_IN2 (Der)** | `D9` | Dirección 2 Motor Derecho |

---

## 2. Secuencia de Calibración y Operación (Pulsador D2)

El pulsador en el pin `D2` gestiona todo el ciclo de vida del seguidor:

1. **Espera de Calibración Negro:**
   - Al encender el robot, el **LED 13 parpadea lento (1 Hz)**.
   - Coloca el robot sobre la superficie negra / línea.
   - **Pulsa el botón (1er toque):** El LED parpadea rápido (10 Hz) mientras toma 80 muestras.
2. **Espera de Calibración Blanco:**
   - El LED queda **fijo encendido**.
   - Coloca el robot sobre la superficie blanca / fondo.
   - **Pulsa el botón (2do toque):** El LED parpadea rápido (10 Hz). Calcula automáticamente los umbrales dinámicos y la polaridad.
3. **Estado Listo (Ready):**
   - El LED realiza un **doble destello rítmico** (latido cada 1 segundo).
4. **Carrera / Seguimiento Activo:**
   - **Pulsa el botón (3er toque):** ¡El robot arranca! Los motores se activan y el algoritmo PD toma el control.
   - Mientras sensa y corre, el **LED 13 titila continuamente** (5 Hz).
5. **Parada de Emergencia:**
   - Si pulsas el botón en cualquier momento durante la carrera, los motores se detienen inmediatamente y el robot regresa al estado **Listo**.

---

## 3. Características del Algoritmo de Control PD

- **Lectura Ultra-Rápida:** Conmutación directa de registros `PORTC` para `S0..S3` y prescaler del ADC ajustado a 16 (`ADCSRA`), permitiendo leer los 16 sensores en **< 300 µs** (> 1000 Hz de tasa de muestreo).
- **Interpolación Ponderada Sub-Sensor:** Los 16 sensores se normalizan individualmente (0 a 1000) y se calcula el centro de masa con setpoint en 7500 (resolución 0 a 15000).
- **Control PD Sin Término I:** Eliminación del término integral para erradicar el retraso y sobrepaso en curvas cerradas.
- **Manejo de Bifurcaciones (Desvíos):** Los sensores de los extremos (`S0`/`S1` y `S14`/`S15`) detectan la apertura de bifurcaciones. Según el modo seleccionado (`FORK_LEFT`, `FORK_RIGHT`, `FORK_STRAIGHT`), el robot enmascara los sensores opuestos para seguir limpiamente la rama elegida.
- **Recuperación de Pérdida de Línea:** Si la línea desaparece súbitamente de la barra, el robot recuerda el último lado donde estuvo y ejecuta un pivote reactivo para reenganchar la pista.
- **Frecuencia PWM Ultrasónica (31.37 kHz):** Ambos motores operan sin el molesto pitido agudo tradicional y con mejor respuesta a bajas revoluciones.

---

## 4. Cómo Subir el Firmware con PlatformIO

Desde la terminal en la raíz del proyecto:
```powershell
& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" run -d firmware -t upload
```

Para monitor serie en consola:
```powershell
& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" device monitor -b 115200
```

---

## 5. Cómo Iniciar la App Web (C# .NET 10 + Svelte + SQLite)

Puedes iniciar ambos servicios con un solo comando ejecutando el script:
```powershell
.\start_app.ps1
```

O bien iniciarlos manualmente en terminales separadas:

### Terminal 1: Backend C# (.NET 10)
```powershell
& "$env:USERPROFILE\.dotnet\dotnet.exe" run --project backend/LineFollower.Api.csproj --urls=http://localhost:5000
```

### Terminal 2: Frontend Svelte (Vite + pnpm)
```powershell
cd frontend
pnpm run dev
```

Abre tu navegador en: **`http://localhost:5173`**

### Funcionalidades del Dashboard Web:
- **Visualizador de 16 Sensores en Tiempo Real:** Barras analógicas (0-1023), estado activo, centro de referencia y puntero de posición dinámica del robot vía WebSockets.
- **Monitor de Motores:** Nivel PWM y dirección (Adelante, Reversa / Freno activo).
- **Ajuste de Parámetros al Vuelo:** Modifica Kp, Kd, Velocidades y Selección de Bifurcación y envíalos inmediatamente a la RAM o EEPROM del robot.
- **Gestión de Perfiles en SQLite:** Guarda diferentes configuraciones para pistas rápidas, técnicas o con desvíos y cárgalas con un solo clic.
- **Control Remoto:** Botones para calibrar negro, calibrar blanco, arrancar y parada de emergencia por puerto serie.
