# Design: Soporte Hardware D8 STBY y Telemetría Directa 8 Canales

## Context

Véase `proposal.md` y `specs/codex8-hardware-and-telemetry/spec.md`. El proyecto cuenta con firmware en C++ sobre plataforma Arduino Nano (ATmega328P). La nueva placa física introdujo el pin digital 8 como control `STBY` del puente H (TB6612FNG) y utiliza 8 canales directos en A0..A7.

## Goals / Non-Goals

**Goals:**
- Activar de forma segura el puente H mediante `PIN_STBY` (D8) en `HIGH`.
- Proveer lecturas analógicas puras y estables en los 8 canales A0 a A7 sin dependencias de librerías externas.
- Garantizar que la telemetría serial fluya de forma inmediata a 25 Hz al encender el microcontrolador.
- Permitir compilar sin errores ni colisiones de pines tanto en PlatformIO como en el IDE oficial de Arduino.
- Recompilar y distribuir el nuevo binario `.hex` optimizado para la suite de escritorio.

**Non-Goals:**
- Modificar el protocolo de comandos de la suite de escritorio (se mantiene la estructura `$TEL`, `$PID`, etc.).
- Alterar la lógica del algoritmo de control PD ni la detección de bifurcaciones ya probadas.

## Decisions

### 1. Manejo del Pin STBY en D8
- **Decisión**: Declarar `#define PIN_STBY 8` en `config.h`. En `motors.init()`, configurar `pinMode(PIN_STBY, OUTPUT); digitalWrite(PIN_STBY, HIGH);`.
- **Alternativas consideradas**:
  - *Puente físico a VCC*: Requiere modificar la placa físicamente con soldador. La solución por software es inmediata y permite apagar los motores por completo si fuera necesario.

### 2. Lectura Analógica Directa en A0..A7 sin Librerías
- **Decisión**:
  - Pines A0 a A5 se configuran explícitamente como `pinMode(pin, INPUT)` en `sensors.init()`.
  - Pines A6 y A7 (canales analógicos puros de ATmega328P sin registros digitales) se leen directamente con `analogRead(A6)` y `analogRead(A7)`.
  - Ajustar el registro `ADCSRA` con prescaler 64 (reloj ADC a 250 kHz, ~32 µs por conversión) o incluir un retardo de estabilización de 3 µs al conmutar canales para asegurar que el condensador Sample-and-Hold del ADC se cargue correctamente con fototransistores de alta impedancia (10k-47k).
- **Alternativas consideradas**:
  - *Uso de librería QTRSensors*: El usuario solicitó explícitamente evitar librerías externas para mantener el código ligero, predecible y bajo control total.

### 3. Telemetría Continua Inmediata
- **Decisión**: Inicializar `telemetry_active = true` en `protocol.h`.
- **Razón**: Permite inspeccionar en vivo los valores de los 8 sensores en cualquier terminal o monitor serial convencional sin requerir el apretón de manos `$CMD,STREAM_ON`. Se preserva el intervalo de emisión a 40 ms (25 Hz) para evitar sobrecarga del procesador.

### 4. Selector Seguro en `config.h` para Arduino IDE
- **Decisión**: Configurar la arquitectura de 8 canales por defecto si no hay ninguna bandera definida en el compilador:
  ```cpp
  #if !defined(ROBOT_IM_16) && !defined(ROBOT_CODEX_8)
  #define ROBOT_CODEX_8 // Por defecto para evitar configurar A0..A4 como salidas digitales
  #endif
  ```
- **Razón**: Si el usuario abre el proyecto directamente en Arduino IDE, compilará como 8 canales directos de forma segura, protegiendo los sensores de colisiones lógicas.

## Risks / Trade-offs

- [Consumo serial continuo en carrera] → A 115200 baudios y 25 Hz (tramas de ~65 bytes cada 40 ms), la transmisión serial consume menos del 1.5% del ancho de banda y del tiempo de CPU, garantizando que el bucle de control a 1 kHz no sufra demoras perceptibles.
- [Corriente de standby en reposo] → Mantener STBY en HIGH consume una corriente insignificante (< 1 mA) en el chip TB6612FNG mientras los PWMs están en 0.
