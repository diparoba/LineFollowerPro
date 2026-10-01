# Tasks: Soporte Hardware D8 STBY y Telemetría Directa de 8 Sensores

## 1. Configuración de Hardware y Pin STBY (D8)

- [x] 1.1 Definir `PIN_STBY 8` y configurar el selector seguro de arquitectura en `firmware/include/config.h` para compilar por defecto en 8 canales directos protegiendo pines A0..A5.
- [x] 1.2 Inicializar `PIN_STBY` en `firmware/include/motors.h` dentro de `motors.init()` como salida en nivel `HIGH` para habilitar el controlador TB6612FNG.

## 2. Lectura Analógica Robusta de 8 Sensores y Telemetría Continua

- [x] 2.1 Optimizar `sensors.init()` y `sensors.readAll()` en `firmware/include/sensors.h` con prescaler equilibrado (64) y retardo de estabilización entre canales para fototransistores sin librerías externas.
- [x] 2.2 Habilitar telemetría serial continua por defecto (`telemetry_active = true`) en `firmware/include/protocol.h` para emitir tramas `$TEL` inmediatamente en el arranque a 25 Hz.

## 3. Recompilación, Empaquetado de Binarios y Validación

- [x] 3.1 Compilar el entorno `nano_codex8` con PlatformIO y verificar ausencia de advertencias y errores.
- [x] 3.2 Copiar el nuevo binario `firmware_codex8.hex` a las carpetas de recursos `frontend/src-tauri/resources/firmwares/` y `release_desktop_v1.0.0/resources/firmwares/`.
- [x] 3.3 Probar la recepción de datos seriales en COM8 verificando la llegada de tramas `$TEL` con los 8 canales analógicos activos.
