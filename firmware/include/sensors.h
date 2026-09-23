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
    uint32_t bifurcation_cooldown = 0;

    void init() {
#if defined(ROBOT_CODEX_8)
        for (uint8_t i = 0; i < NUM_SENSORS; i++) {
            if (SENSOR_PINS[i] <= A5) {
                pinMode(SENSOR_PINS[i], INPUT);
            }
        }
#else
        pinMode(PIN_LON, OUTPUT);
        digitalWrite(PIN_LON, HIGH); // Encender LEDs infrarrojos

        pinMode(PIN_S0, OUTPUT);
        pinMode(PIN_S1, OUTPUT);
        pinMode(PIN_S2, OUTPUT);
        pinMode(PIN_S3, OUTPUT);
        pinMode(PIN_OM, INPUT);
#endif

        // Acelerar ADC: Prescaler 16 (1 MHz ADC clock -> ~16us por conversion)
        // Bit2=1, Bit1=0, Bit0=0 en ADCSRA
        ADCSRA = (ADCSRA & 0xF8) | 0x04;

        // Valores por defecto de calibración preventiva
        for (uint8_t i = 0; i < NUM_SENSORS; i++) {
            calib_black[i] = 850;
            calib_white[i] = 150;
            threshold[i] = 500;
        }
    }

#if defined(ROBOT_CODEX_8)
    // Lectura directa de los 8 sensores analógicos Codex (A0 - A7)
    void readAll() {
        for (uint8_t i = 0; i < NUM_SENSORS; i++) {
            raw[i] = analogRead(SENSOR_PINS[i]);
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
    int32_t getPosition(uint8_t fork_mode, bool dark_line) {
        readAll();

        uint32_t weighted_sum = 0;
        uint32_t sum = 0;
        uint8_t active_count = 0;

        // Normalizar cada sensor entre 0 y 1000
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
                val = ((int32_t)(raw[i] - min_v) * 1000) / (max_v - min_v);
            } else {
                val = (raw[i] > threshold[i]) ? 1000 : 0;
            }

            if (val < 0) val = 0;
            if (val > 1000) val = 1000;

            norm[i] = (uint16_t)val;
            digital[i] = (norm[i] > 400);
            if (digital[i]) active_count++;
        }

#if defined(ROBOT_CODEX_8)
        // Detección de bifurcación para 8 sensores (S7=Izquierda, S0=Derecha)
        bool left_edge = digital[6] || digital[7];
        bool right_edge = digital[0] || digital[1];
        bool center_active = digital[3] || digital[4];
#else
        // Detección de bifurcación para 16 sensores (S15=Izquierda, S0=Derecha)
        bool left_edge = digital[14] || digital[15];
        bool right_edge = digital[0] || digital[1];
        bool center_active = digital[6] || digital[7] || digital[8] || digital[9];
#endif

        bifurcation_detected = false;
        if ((left_edge && (center_active || right_edge)) || (right_edge && center_active)) {
            if (millis() > bifurcation_cooldown) {
                bifurcation_detected = true;
                bifurcation_cooldown = millis() + 250; // Evita rebotes por 250ms
            }
        }

        // Rango de sensores según bifurcación
        uint8_t start_idx = 0;
        uint8_t end_idx = NUM_SENSORS - 1;

#if defined(ROBOT_CODEX_8)
        if (fork_mode == FORK_LEFT && bifurcation_detected) {
            start_idx = 4; // Seguir rama izquierda (sensores 4 a 7)
        } else if (fork_mode == FORK_RIGHT && bifurcation_detected) {
            end_idx = 3;   // Seguir rama derecha (sensores 0 a 3)
        }
#else
        if (fork_mode == FORK_LEFT && bifurcation_detected) {
            start_idx = 8; // Seguir rama izquierda (sensores 8 a 15)
        } else if (fork_mode == FORK_RIGHT && bifurcation_detected) {
            end_idx = 7;   // Seguir rama derecha (sensores 0 a 7)
        }
#endif

        for (uint8_t i = start_idx; i <= end_idx; i++) {
            uint32_t weight = (uint32_t)i * 1000;
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
