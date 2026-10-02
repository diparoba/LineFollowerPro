# Proposal: Soporte de Nueva Placa con D8 STBY y Telemetría Directa de 8 Sensores

## Why

El usuario ha ensamblado una nueva placa PCB para el robot seguidor de línea de 8 canales directos (A0 a A7), en la cual el pin de habilitación (`STBY`) del puente H (TB6612FNG) se conectó al pin digital 8 (`D8`). Además, la regleta de sensores no responde en el monitor serial debido a que:
1. La telemetría se encontraba silenciada por defecto al encender (`telemetry_active = false`), impidiendo ver lecturas en el monitor serial convencional.
2. Si el código se compila fuera de PlatformIO (ej. Arduino IDE), la configuración por defecto asume 16 canales multiplexados, forzando los pines A0..A4 como salidas digitales que entran en corto/conflicto con las señales analógicas de los sensores.
3. El prescaler 16 configurado en el ADC conmuta demasiado rápido para fototransistores de alta impedancia, requiriendo estabilización de muestreo sin librerías externas.

Esta propuesta moderniza el firmware para dar soporte nativo al pin `D8 STBY`, restablece la telemetría continua al arrancar y garantiza lecturas analógicas limpias y 100% nativas en A0..A7.

## What Changes

- **Soporte de Pin STBY en D8**: Definición de `PIN_STBY 8` en el firmware e inicialización a `OUTPUT` / `HIGH` en `motors.init()` para mantener el puente H activo.
- **Telemetría Continua Inmediata**: Eliminación del silencio obligado en el arranque; `telemetry_active` pasará a ser `true` por defecto para emitir `$TEL` en tiempo real a 25 Hz nada más encender el microcontrolador.
- **Lectura Pura y Robusta de 8 Sensores (A0..A7)**:
  - Sin uso de librerías externas de terceros (100% código C++ nativo para ATmega328P).
  - Prescaler ADC equilibrado (64 o 128) con pequeño tiempo de estabilización entre canales para evitar crosstalk y lecturas congeladas en fototransistores.
- **Selector Amigable en `config.h`**: Inclusión de selector explícito de modo (`#define USE_CODEX_8`) para compilar con éxito tanto en Arduino IDE como en PlatformIO sin riesgo de configurar A0..A4 como salidas digitales.
- **Regeneración de Binarios Embebidos**: Recompilación de `firmware_codex8.hex` y actualización de los binarios distribuidos en `resources/firmwares/`.

## Capabilities

### New Capabilities
- `codex8-hardware-and-telemetry`: Especifica el soporte de hardware para el pin STBY en D8, la lectura directa de 8 sensores analógicos sin librerías y la transmisión inmediata de telemetría por puerto serial.

## Impact

- **Firmware C++ (`firmware/`)**:
  - `config.h`: Definición de `PIN_STBY 8`, selector explícito de arquitectura de sensores.
  - `motors.h`: Inicialización de `PIN_STBY` en `init()` y control en parada.
  - `sensors.h`: Muestreo ADC estabilizado para 8 canales directos.
  - `protocol.h`: Inicialización de `telemetry_active = true` al inicio.
- **Binarios de Firmware**:
  - `firmware/.pio/build/nano_codex8/firmware.hex`
  - `release_desktop_v1.0.0/resources/firmwares/firmware_codex8.hex`
- **Frontend / Desktop**:
  - Sigue recibiendo las tramas `$TEL` estándar; al recibir 8 canales se conecta y grafica de inmediato en el visualizador sin necesidad de enviar comando de inicio previo.
