#include <Arduino.h>
#include "config.h"
#include "sensors.h"
#include "motors.h"
#include "controller.h"
#include "protocol.h"

// Instancias principales
SensorArray sensors;
MotorDriver motors;
PDController controller;
SerialProtocol comm;

// Configuración activa
RobotConfig config;
RobotState state = STATE_WAIT_CALIB_BLACK;

// Variables de tiempo y estado
uint32_t last_telemetry_time = 0;
uint32_t last_led_toggle = 0;
bool led_state = false;

// Variables de botón con antirebote
bool last_button_state = HIGH;
uint32_t last_debounce_time = 0;
const uint32_t DEBOUNCE_DELAY = 50;

// Variables auxiliares de posición
int32_t current_pos = SENSOR_SETPOINT;
int32_t current_err = 0;

void setLED(bool on) {
    digitalWrite(PIN_LED, on ? HIGH : LOW);
    led_state = on;
}

void toggleLED() {
    led_state = !led_state;
    digitalWrite(PIN_LED, led_state ? HIGH : LOW);
}

// Patrón de parpadeo del LED 13 según el estado actual
void updateLED() {
    uint32_t now = millis();

    switch (state) {
        case STATE_WAIT_CALIB_BLACK:
            // Parpadeo lento (1 Hz) esperando 1er toque
            if (now - last_led_toggle >= 500) {
                last_led_toggle = now;
                toggleLED();
            }
            break;

        case STATE_CALIB_BLACK:
        case STATE_CALIB_WHITE:
            // Parpadeo ultra-rápido (10 Hz) mientras toma muestras
            if (now - last_led_toggle >= 50) {
                last_led_toggle = now;
                toggleLED();
            }
            break;

        case STATE_WAIT_CALIB_WHITE:
            // LED fijo encendido indicando que el negro ya se guardó
            setLED(true);
            break;

        case STATE_READY:
            // Doble destello (latido) cada 1 segundo
            {
                uint32_t phase = now % 1000;
                bool on = (phase < 80) || (phase >= 160 && phase < 240);
                setLED(on);
            }
            break;

        case STATE_RUNNING:
            // Parpadeo continuo mientras sensa y corre (5 Hz)
            if (now - last_led_toggle >= 100) {
                last_led_toggle = now;
                toggleLED();
            }
            break;
    }
}

// Lectura del pulsador con antirebote (flanco de bajada al presionar a GND)
bool checkButtonPressed() {
    int reading = digitalRead(PIN_BOTON);
    bool pressed = false;

    if (reading != last_button_state) {
        last_debounce_time = millis();
    }

    if ((millis() - last_debounce_time) > DEBOUNCE_DELAY) {
        static int debounced_button = HIGH;
        if (reading != debounced_button) {
            debounced_button = reading;
            if (debounced_button == LOW) {
                pressed = true;
            }
        }
    }

    last_button_state = reading;
    return pressed;
}

void setup() {
    // Configurar pines de interfaz
    pinMode(PIN_BOTON, INPUT_PULLUP);
    pinMode(PIN_LED, OUTPUT);
    setLED(true);

    // Inicializar subsistemas
    comm.init();
    sensors.init();
    motors.init();
    controller.reset();

    // Cargar configuración desde EEPROM o asignar valores por defecto
    if (!comm.loadConfigEEPROM(config)) {
#if defined(ROBOT_CODEX_8)
        config.kp = 0.35f;
        config.kd = 5.0f;
#else
        config.kp = 0.24f;
        config.kd = 4.2f;
#endif
        config.base_speed = 180;
        config.max_speed = 255;
        config.brake_speed = 130;
        config.fork_mode = FORK_STRAIGHT;
        config.line_color = 0; // Línea negra por defecto
        comm.saveConfigEEPROM(config);
    }

#if defined(ROBOT_CODEX_8)
    Serial.println(F("$BOOT,READY_8_CHANNELS_CODEX"));
#else
    Serial.println(F("$BOOT,READY_16_CHANNELS"));
#endif
    comm.sendConfig(config);

    setLED(false);
}

void loop() {
    // 1. Procesar comandos por puerto serial (si hay datos)
    comm.processSerial(config, state, sensors, motors);

    // 2. Gestionar pulsador de usuario
    if (checkButtonPressed()) {
        switch (state) {
            case STATE_WAIT_CALIB_BLACK:
                state = STATE_CALIB_BLACK;
                break;

            case STATE_WAIT_CALIB_WHITE:
                state = STATE_CALIB_WHITE;
                break;

            case STATE_READY:
                controller.reset();
                state = STATE_RUNNING;
                break;

            case STATE_RUNNING:
                // Parada de emergencia
                motors.stop();
                state = STATE_READY;
                break;

            default:
                break;
        }
    }

    // 3. Ejecutar calibraciones si se activaron
    if (state == STATE_CALIB_BLACK) {
        Serial.println(F("$LOG,CALIBRATING_BLACK"));
        sensors.sampleSurface(sensors.calib_black, 80);
        state = STATE_WAIT_CALIB_WHITE;
        Serial.println(F("$OK,BLACK_DONE"));
    } else if (state == STATE_CALIB_WHITE) {
        Serial.println(F("$LOG,CALIBRATING_WHITE"));
        sensors.sampleSurface(sensors.calib_white, 80);
        sensors.computeThresholds(config.line_color == 0);
        state = STATE_READY;
        Serial.println(F("$OK,WHITE_DONE_CALIBRATED"));
    }

    // 4. Control de Seguimiento en Carrera
    if (state == STATE_RUNNING) {
        current_pos = sensors.getPosition(config.fork_mode, config.line_color == 0);
        current_err = current_pos - SENSOR_SETPOINT;
        controller.update(current_pos, sensors.line_lost, config, motors);
    } else {
        // En reposo, seguir leyendo sensores para telemetría visual
        sensors.readAll();
        current_pos = sensors.last_position;
        current_err = current_pos - SENSOR_SETPOINT;
    }

    // 5. Actualizar animación del LED 13
    updateLED();

    // 6. Transmisión de telemetría a la app en PC (cada 40ms = 25 Hz)
    // Solo transmite si la suite de escritorio o enlace Bluetooth ha activado la telemetría bajo demanda ($CMD,STREAM_ON)
    uint32_t now = millis();
    if (comm.isTelemetryActive() && (now - last_telemetry_time >= 40)) {
        last_telemetry_time = now;
        comm.sendTelemetry(sensors, current_pos, current_err, motors, (uint8_t)state);
    }
}
