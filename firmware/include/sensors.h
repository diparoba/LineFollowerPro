#pragma once
#include <Arduino.h>
#include "config.h"

class SensorArray {
public:
    uint16_t raw[NUM_SENSORS];
    uint16_t norm[NUM_SENSORS];       // Normalizado 0..1000
    bool digital[NUM_SENSORS];         // true = sobre la línea
    uint16_t calib_black[NUM_SENSORS];
    uint16_t calib_white[NUM_SENSORS];
    uint16_t threshold[NUM_SENSORS];

    int32_t last_position = SENSOR_SETPOINT;
    bool line_lost = false;
    bool bifurcation_detected = false;
    uint32_t bifurcation_active_until = 0; // Mantiene la decisión de rama durante toda la maniobra
    uint32_t bifurcation_cooldown = 0;     // Evita re-disparos tras salir de la bifurcación

    // ---- Parámetros del Modo Piso Reflectante ----
    static const int16_t  REFLECT_NOISE_GATE = 250;        // <= 250/1000 se aplasta a 0 (brillo del fondo)
    static const int16_t  REFLECT_BLACK_SAT  = 760;        // >= 760/1000 vale 1000 (cinta negra iluminada)
    static const uint16_t NORMAL_DIGITAL_THRESHOLD  = 400; // Umbral digital original
    static const uint16_t REFLECT_DIGITAL_THRESHOLD = 350; // Umbral digital en piso reflectante

    // ---- Parámetros de Bifurcación ----
    static const uint32_t FORK_HOLD_MS     = 160;  // Duración sostenida de la elección de rama
    static const uint32_t FORK_COOLDOWN_MS = 400;  // Bloqueo de re-disparo desde el inicio de la maniobra
#if defined(ROBOT_CODEX_8)
    static const int8_t  REFLECT_WINDOW_RADIUS = 3; // Ventana espacial +-3 sensores
    static const int8_t  FORK_BRANCH_RADIUS    = 2; // Ancho de ventana alrededor del borde seguido
    static const uint8_t FORK_MIN_SPAN         = 4; // Ancho anormal de línea (en sensores)
    static const int8_t  FORK_RIGHT_REACH      = 2; // La mancha llega al menos a S2 por la derecha...
    static const int8_t  FORK_LEFT_REACH       = 5; // ...y al menos a S5 por la izquierda
#else
    static const int8_t  REFLECT_WINDOW_RADIUS = 4; // Ventana espacial +-4 sensores
    static const int8_t  FORK_BRANCH_RADIUS    = 3;
    static const uint8_t FORK_MIN_SPAN         = 7;
    static const int8_t  FORK_RIGHT_REACH      = 5; // Hasta S5 por la derecha...
    static const int8_t  FORK_LEFT_REACH       = 10; // ...y hasta S10 por la izquierda
#endif

    void init() {
#if defined(ROBOT_CODEX_8)
        for (uint8_t i = 0; i < NUM_SENSORS; i++) {
            if (SENSOR_PINS[i] <= A5) {
                pinMode(SENSOR_PINS[i], INPUT);
            }
        }
        // Prescaler 64: Reloj ADC a 250 kHz (16 MHz / 64) -> ~32us por conversión
        // Bits ADPS2=1, ADPS1=1, ADPS0=0 (0x06). Óptimo para fototransistores sin ghosting.
        ADCSRA = (ADCSRA & 0xF8) | 0x06;
#else
        pinMode(PIN_LON, OUTPUT);
        digitalWrite(PIN_LON, HIGH); // Encender LEDs infrarrojos

        pinMode(PIN_S0, OUTPUT);
        pinMode(PIN_S1, OUTPUT);
        pinMode(PIN_S2, OUTPUT);
        pinMode(PIN_S3, OUTPUT);
        pinMode(PIN_OM, INPUT);

        // Acelerar ADC: Prescaler 16 para multiplexor rápido de 16 canales
        ADCSRA = (ADCSRA & 0xF8) | 0x04;
#endif

        // Valores por defecto de calibración preventiva
        for (uint8_t i = 0; i < NUM_SENSORS; i++) {
            calib_black[i] = 850;
            calib_white[i] = 150;
            threshold[i] = 500;
        }
    }

#if defined(ROBOT_CODEX_8)
    // Lectura directa de los 8 sensores analógicos Codex (A0 - A7) sin librerías externas
    void readAll() {
        for (uint8_t i = 0; i < NUM_SENSORS; i++) {
            raw[i] = analogRead(SENSOR_PINS[i]);
            delayMicroseconds(3); // Breve asentamiento del capacitor S&H del conversor ADC
        }
    }
#else
    // Lectura de un canal individual con cambio directo en PORTC para 16 canales
    inline uint16_t readChannel(uint8_t channel) {
        PORTC = (PORTC & ~0x1E) | ((channel & 0x0F) << 1);
        delayMicroseconds(3); // Tiempo de estabilización analógica del multiplexor
        return analogRead(PIN_OM);
    }

    // Lee los 16 sensores a máxima velocidad
    void readAll() {
        for (uint8_t i = 0; i < NUM_SENSORS; i++) {
            raw[i] = readChannel(i);
        }
    }
#endif

    // Muestra una superficie y promedia N lecturas para calibrar
    void sampleSurface(uint16_t *destArray, uint8_t samples = 60) {
        uint32_t accumulator[NUM_SENSORS] = {0};

        for (uint8_t s = 0; s < samples; s++) {
            readAll();
            for (uint8_t i = 0; i < NUM_SENSORS; i++) {
                accumulator[i] += raw[i];
            }
            delay(15);
        }

        for (uint8_t i = 0; i < NUM_SENSORS; i++) {
            destArray[i] = accumulator[i] / samples;
        }
    }

    // Calcula umbrales y rangos tras calibrar negro y blanco
    void computeThresholds(bool dark_line_on_white) {
        for (uint8_t i = 0; i < NUM_SENSORS; i++) {
            threshold[i] = (calib_black[i] + calib_white[i]) / 2;
        }
    }

    // Normaliza lecturas y calcula la posición de la línea
    //  - fork_mode: FORK_STRAIGHT / FORK_LEFT / FORK_RIGHT
    //  - dark_line: true = línea negra sobre fondo blanco
    //  - reflective_mode: true = piso brillante (filtro antirreflejos); false = algoritmo original intacto
    int32_t getPosition(uint8_t fork_mode, bool dark_line, bool reflective_mode = false) {
        readAll();

        uint32_t weighted_sum = 0;
        uint32_t sum = 0;
        uint8_t active_count = 0;
        const uint16_t digital_threshold = reflective_mode ? REFLECT_DIGITAL_THRESHOLD : NORMAL_DIGITAL_THRESHOLD;

        // 1. Normalizar cada sensor entre 0 y 1000
        for (uint8_t i = 0; i < NUM_SENSORS; i++) {
            int32_t val;
            int32_t min_v = calib_white[i];
            int32_t max_v = calib_black[i];

            if (!dark_line) {
                // Línea blanca sobre fondo negro: invertimos min y max
                min_v = calib_black[i];
                max_v = calib_white[i];
            }

            if (max_v > min_v) {
                val = (((int32_t)raw[i] - min_v) * 1000L) / (max_v - min_v);
            } else {
                val = (raw[i] > threshold[i]) ? 1000 : 0;
            }

            if (val < 0) val = 0;
            if (val > 1000) val = 1000;

            // Modo Piso Reflectante: Noise Gate (borra brillos del fondo) + Techo (recupera cinta negra iluminada)
            if (reflective_mode) {
                if (val <= REFLECT_NOISE_GATE) {
                    val = 0;
                } else if (val >= REFLECT_BLACK_SAT) {
                    val = 1000;
                } else {
                    val = ((val - REFLECT_NOISE_GATE) * 1000L) / (REFLECT_BLACK_SAT - REFLECT_NOISE_GATE);
                }
            }

            norm[i] = (uint16_t)val;
            digital[i] = (norm[i] > digital_threshold);
            if (digital[i]) active_count++;
        }

        // 2. Detección robusta de bifurcación (Y, T o cruce)
        // first_active = sensor activo más a la DERECHA (S0), last_active = más a la IZQUIERDA (S7 / S15)
        int8_t first_active = -1;
        int8_t last_active = -1;
        uint8_t segments = 0;
        bool in_segment = false;

        for (uint8_t i = 0; i < NUM_SENSORS; i++) {
            if (digital[i]) {
                if (first_active == -1) first_active = (int8_t)i;
                last_active = (int8_t)i;
                if (!in_segment) {
                    segments++; // Cada grupo separado por blanco es una línea distinta (2 = "Y" abierta)
                    in_segment = true;
                }
            } else {
                in_segment = false;
            }
        }

        uint8_t width_span = (first_active != -1) ? (uint8_t)(last_active - first_active + 1) : 0;

        // Línea ancha que se extiende a AMBOS lados del centro (T, cruce o "Y" abriéndose).
        // Una curva de 90° solo se extiende hacia un lado, por eso no dispara la bifurcación
        // y conserva el comportamiento original en esquinas.
        bool wide_both_sides = (first_active != -1) &&
                               (width_span >= FORK_MIN_SPAN) &&
                               (active_count >= 3) &&
                               (first_active <= FORK_RIGHT_REACH) &&
                               (last_active >= FORK_LEFT_REACH);

        bool fork_condition = (segments >= 2) || wide_both_sides;

        uint32_t now_ms = millis();
        if (fork_condition && (int32_t)(now_ms - bifurcation_cooldown) >= 0) {
            bifurcation_active_until = now_ms + FORK_HOLD_MS;   // Sostener la elección durante toda la maniobra
            bifurcation_cooldown = now_ms + FORK_COOLDOWN_MS;   // No re-disparar al salir de la bifurcación
        }

        // CLAVE: permanece TRUE durante los 160 ms completos (antes solo duraba 1 ciclo del loop)
        bifurcation_detected = (int32_t)(now_ms - bifurcation_active_until) < 0;

        // 3. Ventana de sensores a evaluar
        int8_t start_idx = 0;
        int8_t end_idx = NUM_SENSORS - 1;

        if (bifurcation_detected && first_active != -1) {
            if (fork_mode == FORK_LEFT) {
                // Rama IZQUIERDA (sensores altos): engancharse al borde last_active
                start_idx = last_active - FORK_BRANCH_RADIUS;
                if (start_idx < 0) start_idx = 0;
            } else if (fork_mode == FORK_RIGHT) {
                // Rama DERECHA (sensores bajos): engancharse al borde first_active
                end_idx = first_active + FORK_BRANCH_RADIUS;
                if (end_idx > NUM_SENSORS - 1) end_idx = NUM_SENSORS - 1;
            } else {
                // FORK_STRAIGHT: ignorar ramas laterales, ventana estrecha centrada en el arreglo
                const int8_t center_idx = NUM_SENSORS / 2;
                start_idx = center_idx - FORK_BRANCH_RADIUS;
                end_idx = center_idx + FORK_BRANCH_RADIUS - 1;
            }
        } else if (reflective_mode && !line_lost) {
            // Ventana espacial móvil antirreflejos: solo sensores cercanos a la última posición
            int8_t last_sensor = (int8_t)((last_position + 500) / 1000);
            start_idx = last_sensor - REFLECT_WINDOW_RADIUS;
            end_idx = last_sensor + REFLECT_WINDOW_RADIUS;
            if (start_idx < 0) start_idx = 0;
            if (end_idx > NUM_SENSORS - 1) end_idx = NUM_SENSORS - 1;
        }

        // 4. Centroide ponderado
        for (int8_t i = start_idx; i <= end_idx; i++) {
            uint32_t weight = (uint32_t)i * 1000UL;
            weighted_sum += (uint32_t)norm[i] * weight;
            sum += norm[i];
        }

        // Si la suma es muy baja, se ha perdido la línea
        uint32_t min_line_sum = (NUM_SENSORS == 8) ? 150 : 250;
        if (sum < min_line_sum) {
            line_lost = true;
            // Retorna último extremo conocido para recuperar
            return (last_position < SENSOR_SETPOINT) ? 0 : SENSOR_MAX_POS;
        }

        line_lost = false;
        int32_t current_pos = (int32_t)(weighted_sum / sum);
        last_position = current_pos;
        return current_pos;
    }
};
