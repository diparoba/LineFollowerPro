# Design: Telemetría Bajo Demanda & Caja Negra (Black Box)

## Context

Actualmente, el microcontrolador ATmega328P transmite paquetes `$TEL` ininterrumpidamente cada 40 ms a través de `Serial.print(...)` desde que arranca. En situaciones de competencia donde el robot corre con batería en pista sin una laptop conectada por USB, esto desperdicia ciclos de CPU e introduce posibles retrasos en el control de motores.

Véase `proposal.md` para la motivación general.

## Goals / Non-Goals

**Goals:**
- Garantizar silencio total del puerto UART en el encendido hasta recibir una orden explícita de activación.
- Encender/apagar la telemetría de forma transparente al conectar/desconectar en `LineFollowerPro`.
- Registrar carreras automáticamente al pasar a `STATE_RUNNING` y finalizar al frenar.
- Almacenar un búfer de las últimas 5 vueltas con cálculo de métricas matemáticas (RMSE, tiempo exacto, velocidad).
- Ofrecer un visualizador interactivo con Canvas HTML5 ligero y exportación a CSV sin librerías pesadas.

**Non-Goals:**
- Transmisión de video o protocolos pesados no seriales.
- Modificación de la circuitería física de los robots existentes (el protocolo utiliza la UART existente D0/D1).

## Decisions

### 1. Bandera `telemetry_active` en el Firmware
- **Decisión:** Declarar `bool telemetry_active = false;` en `SerialProtocol` dentro de `protocol.h`.
- **Comportamiento:**
  - En `setup()` de Arduino, `telemetry_active` se inicializa en `false`.
  - El bucle `loop()` en `main.cpp` evalúa `comm.isTelemetryActive()` antes de formatear o transmitir la trama `$TEL`.
  - El comando `$CMD,STREAM_ON` conmuta la bandera a `true`.
  - El comando `$CMD,STREAM_OFF` conmuta la bandera a `false`.
- **Alternativas consideradas:**
  - *Detección de voltaje VBUS por hardware*: Requeriría soldar un cable adicional desde el pin USB hasta un pin analógico del Nano. La solución por comando software es 100% no destructiva y compatible con todas las tarjetas.

### 2. Handshake Automático desde la Aplicación
- **Decisión:** En `App.svelte` / `tauriBridge.js`, al completar la conexión serial (por USB o por Bluetooth HC-05), se transmite automáticamente `$CMD,STREAM_ON`. Al presionar desconectar, se envía `$CMD,STREAM_OFF`.
- **Alternativas consideradas:**
  - *Requerir que el usuario presione un botón manual para iniciar la telemetría*: Mayor fricción de uso. El enlace transparente ofrece la mejor ergonomía.

### 3. Máquina de Estados de la Caja Negra (*Data Logger*)
- **Decisión:** En el frontend, un observador reactivo monitorea `telemetry.state`:
  - Transición `STATE_READY` → `STATE_RUNNING`: Inicializa nuevo arreglo de muestras `currentLapSamples = []` y marca `lapStartTime = performance.now()`.
  - Transición `STATE_RUNNING` → `STATE_READY` o `STATE_WAIT`: Marca `lapEndTime`, calcula duración y métricas, empuja la vuelta a `lapHistory` (máximo 5) y activa la visualización.
- **Alternativas consideradas:**
  - *Guardar en disco SQLite cada muestra individualmente*: Demasiadas escrituras I/O a 60 Hz. Guardar la vuelta en memoria y persistir el resumen o exportar a CSV es mucho más ágil.

### 4. Gráficas con Canvas Nativo de Alto Rendimiento
- **Decisión:** Desarrollar un componente de gráfica con Canvas 2D nativo dentro de `BlackBoxModal.svelte`.
- **Razón:** Cero impacto en el peso del ejecutable (evita Chart.js o D3 que pesan varios megabytes) y renderizado a 60 FPS sin sobrecarga del DOM.

## Risks / Trade-offs

- **[Riesgo] El usuario desconecta el cable USB de golpe sin pulsar "Desconectar"**:
  - *Mitigación:* Si la conexión se corta abruptamente, la siguiente vez que el Arduino se encienda nacerá en silencio (`telemetry_active = false`). Adicionalmente, el intento de escritura en un buffer UART lleno en Arduino no bloquea el microcontrolador si se supera el timeout.
