# Design: Configuración de Polaridad y Corrección de Dirección de Motores ESP32

## Context

El firmware actual ([`firmware/src/esp32_bt_car.cpp`](file:///c:/PVIProy/Seguidor_Nano_16/firmware/src/esp32_bt_car.cpp)) controla el puente H dual TB6612FNG mediante las señales directas `AIN1=HIGH, AIN2=LOW` para `speedA > 0` y `BIN1=HIGH, BIN2=LOW` para `speedB > 0`. Debido a la orientación mecánica de los motorreductores en el chasis físico, este patrón de giro produce retroceso en lugar de avance, y en consecuencia invierte la cinemática de giro sobre el eje (`L` y `R`) y los movimientos diagonales.

## Goals / Non-Goals

**Goals:**
- Implementar banderas de inversión `#define INVERT_MOTOR_A true` e `#define INVERT_MOTOR_B true` en `firmware/src/esp32_bt_car.cpp`.
- Ajustar `setMotors()` para que `speed > 0` active la rotación de avance físico y `speed < 0` active la rotación de retroceso físico, preservando `0` como frenado/parada (`LOW, LOW`).
- Recompilar el binario con PlatformIO (`pio run -e esp32_bt_car`) y sincronizar `firmware_esp32_bt.bin` en los recursos de Tauri (`frontend/src-tauri/resources/firmwares/esp32/`) y la carpeta de distribución (`release_desktop_v1.0.0/resources/firmwares/esp32/`).
- Mantener intacto el token de 32 bytes `##BT_CAR_CUSTOM_NAME_TOKEN##` para compatibilidad total con la inyección dinámica de nombre por parte del backend en Rust.

**Non-Goals:**
- Modificar el cableado físico o la asignación de pines GPIO (PWMA: 32, AIN1: 25, AIN2: 33, PWMB: 18, BIN1: 19, BIN2: 27, STBY: 26).
- Alterar la lógica del flasheador en Rust o la interfaz gráfica en Svelte (ambos permanecen compatibles al 100%).

## Decisions

### Decisión 1: Banderas de preprocesador `#define` para polaridad
- **Elección:** Definir `INVERT_MOTOR_A true` e `INVERT_MOTOR_B true` a nivel de preprocesador/constantes en C++.
- **Alternativas consideradas:**
  1. *Intercambiar físicamente los números de pin `PIN_AIN1` con `PIN_AIN2`:* Funcionaría pero confundiría la correspondencia con la tabla de cableado físico de la interfaz gráfica y el datasheet del TB6612.
  2. *Banderas `#define`:* Es la práctica estándar en robótica móvil (como en Marlin, Betaflight o Arduino), manteniendo clara la correspondencia física de los pines y permitiendo ajustar la polaridad en una sola línea.

### Decisión 2: Implementación en `setMotors`
La función calculará las salidas lógicas considerando la inversión:
```cpp
bool fwdA = speedA > 0;
bool fwdB = speedB > 0;

// Si INVERT_MOTOR_A es true, el sentido se invierte
bool pinA1_state = INVERT_MOTOR_A ? !fwdA : fwdA;
bool pinA2_state = INVERT_MOTOR_A ? fwdA : !fwdA;
```
Cuando la velocidad es 0, ambos pines van a `LOW` independientemente de la polaridad para garantizar parada pasiva.

## Risks / Trade-offs

- **[Riesgo] Alteración del tamaño del binario o pérdida del token:** → *Mitigación:* Se compila bajo la misma configuración de PlatformIO garantizando que el array `BT_DEVICE_NAME[32]` con `__attribute__((used))` persista en el binario `.bin` resultante.
- **[Riesgo] Desincronización de binarios en producción:** → *Mitigación:* Tarea explícita de copia del binario compilado hacia `frontend/src-tauri/resources/firmwares/esp32/` y `release_desktop_v1.0.0/resources/firmwares/esp32/` antes de recompilar el ejecutable Tauri.
