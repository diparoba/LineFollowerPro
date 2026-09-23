# Tasks: Telemetría Bajo Demanda & Caja Negra (Black Box)

## 1. Actualización de Firmware C++ (Zero-Print Startup)

- [x] 1.1 Modificar `firmware/include/protocol.h` para incorporar la variable `telemetry_active` inicializada en `false`, getters/setters y el procesamiento de los comandos `$CMD,STREAM_ON` y `$CMD,STREAM_OFF`.
- [x] 1.2 Actualizar `firmware/src/main.cpp` para transmitir telemetría únicamente cuando `comm.isTelemetryActive()` sea verdadero, y recompilar ambos firmwares (`nano_im16` y `nano_codex8`) con PlatformIO.
- [x] 1.3 Copiar los binarios `.hex` generados a `frontend/src-tauri/resources/firmwares/` y a la carpeta de distribución `release_desktop_v0.2.0-beta/resources/firmwares/`.

## 2. Enlace y Handshake en la Suite de Escritorio

- [x] 2.1 Actualizar `frontend/src/App.svelte` para enviar automáticamente el comando `$CMD,STREAM_ON` inmediatamente después de abrir el puerto serial (USB o Bluetooth HC-05) y `$CMD,STREAM_OFF` antes de cerrarlo.

## 3. Módulo Caja Negra (*Black Box / Data Logger*)

- [x] 3.1 Crear `frontend/src/lib/BlackBoxModal.svelte` con lógica de auto-grabación reactiva ante cambios de estado (`STATE_READY` → `STATE_RUNNING` → `STATE_READY`), cronómetro de alta precisión en milisegundos y cálculo de Error Cuadrático Medio (RMSE).
- [x] 3.2 Implementar el búfer de historial de las últimas 5 vueltas con tarjetas de resumen (tiempo, RMSE, velocidad pico, frenadas en curva).
- [x] 3.3 Desarrollar la gráfica interactiva en Canvas HTML5 para visualizar la curva de error a lo largo del tiempo y permitir superponer dos vueltas para análisis comparativo.
- [x] 3.4 Implementar la función de exportación a archivo `.csv` estructurado con columnas de tiempo, error, posición, PWMs y canales de sensores.
- [x] 3.5 Integrar el botón `🔴 Caja Negra` en el encabezado de `frontend/src/App.svelte` y conectar los eventos de carrera.

## 4. Validación y Compilación de Producción

- [x] 4.1 Compilar y validar el frontend con `pnpm run build` sin errores ni advertencias de linting.
- [x] 4.2 Compilar el ejecutable final de producción con `pnpm tauri build --no-bundle` y actualizar `release_desktop_v0.2.0-beta/LineFollowerPro.exe`.
