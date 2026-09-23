#pragma once
#include <Arduino.h>
#include "config.h"
#include "motors.h"

class PDController {
public:
    int32_t error = 0;
    int32_t last_error = 0;
    float p_term = 0;
    float d_term = 0;
    int32_t correction = 0;

    void reset() {
        error = 0;
        last_error = 0;
        p_term = 0;
        d_term = 0;
        correction = 0;
    }

    void update(int32_t position, bool line_lost, const RobotConfig &cfg, MotorDriver &motors) {
        // En caso de pérdida total de línea: giro reactivo según último lado visto
        // Con barra Ingeniero Maker: last_error > 0 es IZQUIERDA (S15), last_error < 0 es DERECHA (S0)
        if (line_lost) {
            if (last_error > 0) {
                // Línea perdida por la izquierda: pivote rápido a la izquierda (izq freno/atrás, der adelante)
                motors.setMotors(-cfg.brake_speed, cfg.base_speed);
            } else {
                // Línea perdida por la derecha: pivote rápido a la derecha (izq adelante, der freno/atrás)
                motors.setMotors(cfg.base_speed, -cfg.brake_speed);
            }
            return;
        }

        // Setpoint en el centro exacto: SENSOR_SETPOINT (7500 en 16L, 3500 en 8L)
        error = position - SENSOR_SETPOINT;

        // Cálculo de términos P y D
        p_term = (float)error * cfg.kp;
        d_term = (float)(error - last_error) * cfg.kd;
        correction = (int32_t)(p_term + d_term);

        last_error = error;

        // Velocidad dinámica: reducir velocidad base en curvas cerradas para evitar derrape
        int32_t abs_err = abs(error);
        int16_t current_base = cfg.base_speed;

        int32_t curve_threshold = (int32_t)((int64_t)SENSOR_MAX_POS * 2 / 15);
        int32_t curve_range = SENSOR_MAX_POS - SENSOR_SETPOINT - curve_threshold;

        if (abs_err > curve_threshold && curve_range > 0) {
            int32_t speed_reduction = ((abs_err - curve_threshold) * (cfg.base_speed / 2)) / curve_range;
            current_base -= speed_reduction;
            if (current_base < 60) current_base = 60;
        }

        // Aplicación del diferencial de giro:
        // Si error > 0 (línea a la izquierda en S15): left disminuye, right aumenta -> gira IZQUIERDA
        // Si error < 0 (línea a la derecha en S0): left aumenta, right disminuye -> gira DERECHA
        int16_t left = current_base - correction;
        int16_t right = current_base + correction;

        // Límites y frenado activo en curva
        if (left > cfg.max_speed) left = cfg.max_speed;
        if (right > cfg.max_speed) right = cfg.max_speed;

        if (left < -cfg.brake_speed) left = -cfg.brake_speed;
        if (right < -cfg.brake_speed) right = -cfg.brake_speed;

        motors.setMotors(left, right);
    }
};
