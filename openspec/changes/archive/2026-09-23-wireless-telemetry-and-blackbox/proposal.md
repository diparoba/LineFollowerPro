# Proposal: Telemetría Bajo Demanda, Enlace Inalámbrico & Caja Negra (Black Box)

## Why

Cuando un robot seguidor de línea de alta velocidad compite en pista, el envío ininterrumpido de tramas seriales a ciegas (`Serial.print` constante a 25/60 Hz) sin una PC conectada consume ciclos críticos de CPU del microcontrolador ATmega328P, provocando micro-retrasos en el bucle de control PID y calentamiento innecesario del procesador y del chip USB. Además, los pilotos carecen de una herramienta de análisis post-carrera para revisar objetivamente las curvas donde el carro osciló, comparar tiempos entre tandas y contrastar vueltas anteriores tras modificar los parámetros $K_p$ y $K_d$.

## What Changes

- **Telemetría Bajo Demanda (*Zero-Print Startup & Active Stream*)**:
  - El firmware arranca con la telemetría estrictamente apagada (`telemetry_active = false`) desde el milisegundo cero en `setup()`.
  - El microcontrolador no emite un solo `Serial.print` a menos que reciba la orden explícita `$CMD,STREAM_ON` desde la aplicación de escritorio (`LineFollowerPro`), sea por cable USB o por módulo inalámbrico Bluetooth HC-05 / ESP32.
  - Al desconectar el puerto serial en la app, se envía automáticamente la orden `$CMD,STREAM_OFF` silenciando el microcontrolador.
- **Módulo Caja Negra (*Black Box / Data Logger*) en la Suite**:
  - Auto-grabación inteligente: se activa automáticamente cuando el robot pasa a estado `STATE_RUNNING` y se detiene cuando el robot finaliza la vuelta o frena (`STATE_READY`).
  - Historial de las últimas 5 vueltas en memoria con tiempo exacto en milisegundos, Error Cuadrático Medio (RMSE), velocidad de motores y conteo de frenadas dinámicas.
  - Visualizador gráfico comparativo con superposición de curvas (ej. Vuelta anterior vs Vuelta récord) y selector temporal.
  - Exportación de telemetría a formato estándar `.csv` para análisis en Excel, Python o MATLAB.
- **Soporte Inalámbrico Bluetooth HC-05**:
  - Integración nativa a través de los puertos COM virtuales seriales de Windows a 115200 y 9600 baudios sin requerir drivers adicionales.

## Capabilities

### New Capabilities
- `blackbox-telemetry`: Protocolo de streaming de telemetría bajo demanda (silencio en arranque con activación/desactivación remota `$CMD,STREAM_ON`/`OFF`), registrador de datos de carrera (Caja Negra) con auto-grabación, historial de las últimas 5 vueltas, comparador gráfico y exportación a CSV.

### Modified Capabilities
- *(Ninguna; `desktop-shell` y `hardware-flasher` permanecen intactos).*

## Impact

- **Firmware C++ (`firmware/include/protocol.h` y `firmware/src/main.cpp`)**:
  - Inclusión de variable booleana `telemetry_active` inicializada en `false`.
  - Soporte de comandos `$CMD,STREAM_ON` y `$CMD,STREAM_OFF`.
  - Condicionamiento del bloque `sendTelemetry` al estado de `telemetry_active`.
- **Frontend Svelte (`frontend/src/`)**:
  - Nuevo componente `BlackBoxModal.svelte` o panel de Caja Negra en la interfaz.
  - Vinculación del inicio/cierre de conexión serial para enviar `$CMD,STREAM_ON` / `$CMD,STREAM_OFF`.
  - Recompilación y actualización de los binarios precompilados `.hex` en los recursos de Tauri.
