# Tasks: Suite Portable Definitiva & Asistente de Auto-Sintonización PID

## 1. Drivers Offline y Embebibilidad Local

- [x] 1.1 Copiar el instalador `drivers/CH341SER.EXE` a `frontend/src-tauri/resources/drivers/` y a la carpeta de distribución `release_desktop_v0.2.0-beta/resources/drivers/`.
- [x] 1.2 Actualizar `frontend/src-tauri/src/flasher.rs` para que la función `install_driver` resuelva la ruta local del archivo `CH341SER.EXE` y lo ejecute con elevación de Administrador en Windows (`Start-Process -FilePath ... -Verb RunAs`).
- [x] 1.3 Modificar `frontend/src/lib/HardwareDrawer.svelte` para reflejar la instalación directa offline del driver CH340 y mostrar confirmación en la interfaz.

## 2. Portabilidad de Persistencia y Respaldo de Base de Datos

- [x] 2.1 Modificar `frontend/src-tauri/src/lib.rs` para anclar la apertura de `follower.db` al directorio real del ejecutable (`current_exe().parent()`) asegurando portabilidad 100% en memorias flash USB.
- [x] 2.2 Implementar el comando nativo Tauri `backup_database` en `frontend/src-tauri/src/db.rs` y registrarlo en `lib.rs` para clonar la base de datos con nombre fechado (`backup_follower_YYYY-MM-DD_HHmmss.db`).
- [x] 2.3 Exponer `backupDatabase` en `frontend/src/lib/tauriBridge.js` e integrar el botón `💾 Crear Backup` en `frontend/src/lib/ProfileManager.svelte`.

## 3. Asistente Analítico de Auto-Sintonización PID

- [x] 3.1 Crear `frontend/src/lib/AutoTuningModal.svelte` con formulario reactivo de parámetros físicos: voltaje de batería ($V_{\text{in}}$), distancia sensor-eje ($L$), ancho entre ruedas ($W$), velocidad base ($V_{\text{base}}$), categoría (16L vs 8L) y estilo de conducción.
- [x] 3.2 Desarrollar el motor matemático de cálculo de ganancias $K_p$, $K_d$, velocidad base, velocidad máxima y frenos con compensación de tensión de batería.
- [x] 3.3 Implementar la gráfica de simulación de respuesta al escalón en Canvas 2D para previsualizar sobreimpulso y estabilidad en tiempo real.
- [x] 3.4 Conectar el botón `Aplicar al Robot` para inyectar las ganancias a los sliders principales y enviarlas por comando serial a la RAM/EEPROM.
- [x] 3.5 Integrar el botón de acceso `🎯 Auto-Tuning PID` en el encabezado de `frontend/src/App.svelte` o barra de control.

## 4. Validación y Empaquetado Portable Definitivo

- [x] 4.1 Compilar y validar el frontend con `pnpm run build` sin errores ni advertencias de linting.
- [x] 4.2 Compilar el ejecutable final optimizado con `pnpm tauri build --no-bundle`, actualizar `release_desktop_v0.2.0-beta/LineFollowerPro.exe` y verificar la suite portable completa con drivers y base de datos.
