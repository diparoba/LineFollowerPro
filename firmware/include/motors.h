#pragma once
#include <Arduino.h>
#include "config.h"

class MotorDriver {
public:
    int16_t current_left = 0;
    int16_t current_right = 0;

    void init() {
#if defined(PIN_STBY)
        pinMode(PIN_STBY, OUTPUT);
        digitalWrite(PIN_STBY, HIGH); // Habilita el puente H TB6612FNG
#endif

        pinMode(PIN_PWM_IZQ, OUTPUT);
        pinMode(PIN_IZQ_IN1, OUTPUT);
        pinMode(PIN_IZQ_IN2, OUTPUT);

        pinMode(PIN_PWM_DER, OUTPUT);
        pinMode(PIN_DER_IN1, OUTPUT);
        pinMode(PIN_DER_IN2, OUTPUT);

        stop();

        // Configurar Timer 2 a 31.37 kHz (ultrasónico, silencioso)
        // Timer 2 controla los pines 3 (OC2B) y 11 (OC2A)
        // Divisor = 1 (bits CS22:0 = 001)
        TCCR2B = (TCCR2B & 0b11111000) | 0b00000001;
    }

    // Control individual de motores con soporte de reversa y frenado activo (-255 a 255)
    // Motor Izquierdo: Pines 11 (PWM), 10 (IN1), 9 (IN2)
    // Motor Derecho:   Pines 3 (PWM), 4 (IN1), 5 (IN2)
    void setMotors(int16_t left, int16_t right) {
        // Limitar valores a [-255, 255]
        left = constrain(left, -255, 255);
        right = constrain(right, -255, 255);

        current_left = left;
        current_right = right;

        // --- MOTOR IZQUIERDO (Pines 11, 10, 9) ---
        if (left > 0) {
            // Avance hacia adelante
            digitalWrite(PIN_IZQ_IN1, LOW);
            digitalWrite(PIN_IZQ_IN2, HIGH);
            analogWrite(PIN_PWM_IZQ, left);
        } else if (left < 0) {
            // Retroceso / Freno activo
            digitalWrite(PIN_IZQ_IN1, HIGH);
            digitalWrite(PIN_IZQ_IN2, LOW);
            analogWrite(PIN_PWM_IZQ, -left);
        } else {
            digitalWrite(PIN_IZQ_IN1, LOW);
            digitalWrite(PIN_IZQ_IN2, LOW);
            analogWrite(PIN_PWM_IZQ, 0);
        }

        // --- MOTOR DERECHO (Pines 3, 4, 5) ---
        if (right > 0) {
            // Avance hacia adelante
            digitalWrite(PIN_DER_IN1, HIGH);
            digitalWrite(PIN_DER_IN2, LOW);
            analogWrite(PIN_PWM_DER, right);
        } else if (right < 0) {
            // Retroceso / Freno activo
            digitalWrite(PIN_DER_IN1, LOW);
            digitalWrite(PIN_DER_IN2, HIGH);
            analogWrite(PIN_PWM_DER, -right);
        } else {
            digitalWrite(PIN_DER_IN1, LOW);
            digitalWrite(PIN_DER_IN2, LOW);
            analogWrite(PIN_PWM_DER, 0);
        }
    }

    // Frena en seco ambos motores
    void stop() {
        digitalWrite(PIN_IZQ_IN1, LOW);
        digitalWrite(PIN_IZQ_IN2, LOW);
        analogWrite(PIN_PWM_IZQ, 0);

        digitalWrite(PIN_DER_IN1, LOW);
        digitalWrite(PIN_DER_IN2, LOW);
        analogWrite(PIN_PWM_DER, 0);

        current_left = 0;
        current_right = 0;
    }
};
