# Design: Web Follower Flashing, ESP32 DevKit V1 BLE, and Universal Speed Scaling

## Context

El sistema cuenta con un backend ASP.NET Core (`LineFollower.Api`) que se ejecuta localmente en el puerto 5010 y una interfaz frontend SPA en Svelte servida en el puerto 5011 (o empaquetada con Tauri). El flasheo de ESP32 se realiza directamente en el navegador mediante Web Serial API (`esptool-js`), mientras que el flasheo de Arduino Nano (ATmega328P) para los seguidores de línea requiere `avrdude` con el protocolo STK500v1.

## Goals / Non-Goals

**Goals:**
- Permitir que los usuarios de la versión web flasheen los seguidores de línea (Arduino Nano `IM_16` y `CODEX_8`) aprovechando el proceso local de `avrdude`.
- Proporcionar un firmware BLE 4.2 dedicado para ESP32 DevKit V1 que habilite control directo desde dispositivos iOS (iPhone/iPad) y Android sin emparejamiento manual.
- Implementar retroalimentación visual en el ESP32 DevKit V1 mediante su LED azul integrado en GPIO 2 (`LED_BUILTIN`).
- Estandarizar la escala de velocidad de control remoto en todos los microcontroladores para que `'0'` sea 0% y `'9'` sea 100% de PWM.

**Non-Goals:**
- Portar el algoritmo seguidor de línea de ATmega328P a ESP32 (los seguidores de línea continúan ejecutándose sobre Arduino Nano).
- Implementar un emulador STK500v1 en JavaScript puro dentro del navegador, ya que el backend local ya dispone de `avrdude.exe` nativo y garantiza la conmutación de temporización DTR sin jitter de navegador.

## Decisions

### 1. Flasheo de Arduino Nano mediante endpoint REST y retransmisión WebSocket
- **Decisión:** Crear el endpoint `POST /api/firmware/flash` en `backend/Program.cs`. El endpoint:
  1. Desconecta temporalmente el puerto COM en `SerialService` si está en uso por la telemetría.
  2. Ejecuta `avrdude.exe` con `-p atmega328p -c arduino -P <port> -b <baud> -D -U flash:w:<hex>:i`.
  3. Redirige la salida `stderr`/`stdout` a eventos WebSocket (`serialService.OnLogReceived`) para que el terminal web muestre el progreso en vivo.
  4. Si falla a 115200 baudios y `autoFallback` está activo, reintenta automáticamente a 57600 baudios (Old Bootloader).
- **Alternativa descartada:** Implementar STK500v1 en JavaScript en el navegador con Web Serial. Se descartó porque los navegadores presentan discrepancias de latencia en la línea DTR con ciertos chips clones CH340, lo que provoca fallos aleatorios de sincronización que `avrdude` maneja de forma robusta.

### 2. Separación de entornos de compilación para ESP32 DevKit V1 (SPP vs BLE)
- **Decisión:** Mantener dos firmwares independientes en PlatformIO: `esp32_bt_car` (Bluetooth Classic SPP) y `esp32_ble_car` (BLE 4.2 NUS).
- **Razón:** El stack de Bluetooth de ESP32 (Bluedroid) consume mucha memoria RAM si se intentan activar ambos protocolos simultáneamente. Mantener binarios separados garantiza ligereza, arranque instantáneo y estabilidad térmica.

### 3. Retroalimentación en GPIO 2 para DevKit V1 BLE
- **Decisión:** En el ESP32 DevKit V1 clásico, el LED azul integrado en la placa está conectado a GPIO 2. El nuevo firmware BLE mantendrá el LED encendido tenue o parpadeando durante la publicidad BLE, y emitirá destellos activos al procesar cada comando (`F`, `B`, `L`, `R`, `S`, etc.).

### 4. Fórmula matemática exacta para velocidades 0 a 9
- **Decisión:** Reemplazar el `switch` estático de velocidades por cálculo proporcional exacto:
  ```cpp
  if (cmd >= '0' && cmd <= '9') {
      currentSpeed = (cmd - '0') * 255 / 9;
  } else if (cmd == 'q' || cmd == 'Q') {
      currentSpeed = 255;
  }
  ```
- **Razón:** Garantiza que `0` = 0 PWM, `9` = 255 PWM (100%), y los valores intermedios sean exactamente proporcionales sin saltos abruptos.

## Risks / Trade-offs

- **[Bloqueo de puerto COM en Windows]** → Si la telemetría está conectada, `avrdude` fallará con "Access denied". *Mitigación:* El backend desconecta automáticamente el puerto COM antes de ejecutar `avrdude` y lo restaura tras finalizar.
- **[Ruta de `avrdude.exe`]** → En entornos donde no está instalado en el PATH del sistema, el backend debe localizarlo. *Mitigación:* Se empaqueta `avrdude.exe` y `avrdude.conf` en `backend/resources/avrdude` y se busca en rutas relativas de respaldo.
