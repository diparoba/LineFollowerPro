#pragma once
#include <Arduino.h>
#include <EEPROM.h>
#include "config.h"
#include "sensors.h"
#include "motors.h"
#include "controller.h"

class SerialProtocol {
private:
    char rx_buffer[80];
    uint8_t rx_idx = 0;
    bool telemetry_active = true; // Activo por defecto para visualización inmediata en monitor serial

public:
    void init() {
        Serial.begin(115200);
        telemetry_active = true;
    }

    bool isTelemetryActive() const { return telemetry_active; }
    void setTelemetryActive(bool active) { telemetry_active = active; }

    bool saveConfigEEPROM(const RobotConfig &cfg) {
        uint16_t magic = EEPROM_MAGIC;
        EEPROM.put(EEPROM_ADDR, magic);
        EEPROM.put(EEPROM_ADDR + sizeof(magic), cfg);

        // Verificación de lectura inmediata desde la memoria no volátil
        uint16_t verify_magic = 0;
        RobotConfig verify_cfg;
        EEPROM.get(EEPROM_ADDR, verify_magic);
        EEPROM.get(EEPROM_ADDR + sizeof(magic), verify_cfg);

        // Validar igualdad bit a bit
        bool ok = (verify_magic == EEPROM_MAGIC) &&
                  (abs(verify_cfg.kp - cfg.kp) < 0.0001f) &&
                  (abs(verify_cfg.kd - cfg.kd) < 0.0001f) &&
                  (verify_cfg.base_speed == cfg.base_speed) &&
                  (verify_cfg.max_speed == cfg.max_speed) &&
                  (verify_cfg.brake_speed == cfg.brake_speed) &&
                  (verify_cfg.fork_mode == cfg.fork_mode) &&
                  (verify_cfg.line_color == cfg.line_color);

        if (ok) {
            // Confirmación detallada con los valores leídos directamente de la EEPROM
            Serial.print(F("$EEPROM_OK,SAVED,"));
            Serial.print(verify_cfg.kp, 4);
            Serial.print(',');
            Serial.print(verify_cfg.kd, 4);
            Serial.print(',');
            Serial.print(verify_cfg.base_speed);
            Serial.print(',');
            Serial.print(verify_cfg.max_speed);
            Serial.print(',');
            Serial.print(verify_cfg.brake_speed);
            Serial.print(',');
            Serial.print(verify_cfg.fork_mode);
            Serial.print(',');
            Serial.println(verify_cfg.line_color);

            // Destello visual en el LED 13 del Arduino confirmando grabación física
            for (uint8_t k = 0; k < 3; k++) {
                digitalWrite(PIN_LED, HIGH);
                delay(40);
                digitalWrite(PIN_LED, LOW);
                delay(40);
            }
            return true;
        } else {
            Serial.println(F("$EEPROM_ERR,VERIFY_FAILED"));
            return false;
        }
    }

    void readConfigEEPROM() {
        uint16_t magic = 0;
        RobotConfig stored;
        EEPROM.get(EEPROM_ADDR, magic);
        if (magic == EEPROM_MAGIC) {
            EEPROM.get(EEPROM_ADDR + sizeof(magic), stored);
            Serial.print(F("$EEPROM_DATA,"));
            Serial.print(stored.kp, 4);
            Serial.print(',');
            Serial.print(stored.kd, 4);
            Serial.print(',');
            Serial.print(stored.base_speed);
            Serial.print(',');
            Serial.print(stored.max_speed);
            Serial.print(',');
            Serial.print(stored.brake_speed);
            Serial.print(',');
            Serial.print(stored.fork_mode);
            Serial.print(',');
            Serial.println(stored.line_color);
        } else {
            Serial.println(F("$EEPROM_ERR,EMPTY"));
        }
    }

    bool loadConfigEEPROM(RobotConfig &cfg) {
        uint16_t magic = 0;
        EEPROM.get(EEPROM_ADDR, magic);
        if (magic == EEPROM_MAGIC) {
            EEPROM.get(EEPROM_ADDR + sizeof(magic), cfg);
            return true;
        }
        return false;
    }

    // Envía paquete de telemetría hacia el PC / Svelte
    void sendTelemetry(const SensorArray &sensors, int32_t pos, int32_t err, const MotorDriver &motors, uint8_t state) {
        Serial.print(F("$TEL"));
        for (uint8_t i = 0; i < NUM_SENSORS; i++) {
            Serial.print(',');
            Serial.print(sensors.raw[i]);
        }
        Serial.print(',');
        Serial.print(pos);
        Serial.print(',');
        Serial.print(err);
        Serial.print(',');
        Serial.print(motors.current_left);
        Serial.print(',');
        Serial.print(motors.current_right);
        Serial.print(',');
        Serial.print(state);
        Serial.println();
    }

    void sendConfig(const RobotConfig &cfg) {
        Serial.print(F("$CFG,"));
        Serial.print(cfg.kp, 4);
        Serial.print(',');
        Serial.print(cfg.kd, 4);
        Serial.print(',');
        Serial.print(cfg.base_speed);
        Serial.print(',');
        Serial.print(cfg.max_speed);
        Serial.print(',');
        Serial.print(cfg.brake_speed);
        Serial.print(',');
        Serial.print(cfg.fork_mode);
        Serial.print(',');
        Serial.println(cfg.line_color);
    }

    // Procesa caracteres entrantes sin bloquear
    bool processSerial(RobotConfig &cfg, RobotState &state, SensorArray &sensors, MotorDriver &motors) {
        while (Serial.available()) {
            char c = Serial.read();
            if (c == '\n' || c == '\r') {
                if (rx_idx > 0) {
                    rx_buffer[rx_idx] = '\0';
                    parseCommand(rx_buffer, cfg, state, sensors, motors);
                    rx_idx = 0;
                    return true;
                }
            } else if (rx_idx < sizeof(rx_buffer) - 1) {
                rx_buffer[rx_idx++] = c;
            }
        }
        return false;
    }

private:
    void parseCommand(char *cmd, RobotConfig &cfg, RobotState &state, SensorArray &sensors, MotorDriver &motors) {
        // Comando $PID,kp,kd,base,max,brake,fork,color
        if (strncmp(cmd, "$PID,", 5) == 0) {
            char *p = cmd + 5;
            float kp = atof(strtok(p, ","));
            char *t = strtok(NULL, ",");
            float kd = t ? atof(t) : cfg.kd;
            t = strtok(NULL, ",");
            int16_t base = t ? atoi(t) : cfg.base_speed;
            t = strtok(NULL, ",");
            int16_t max_s = t ? atoi(t) : cfg.max_speed;
            t = strtok(NULL, ",");
            int16_t brake = t ? atoi(t) : cfg.brake_speed;
            t = strtok(NULL, ",");
            uint8_t fork_m = t ? (uint8_t)atoi(t) : cfg.fork_mode;
            t = strtok(NULL, ",");
            uint8_t color_l = t ? (uint8_t)atoi(t) : cfg.line_color;

            cfg.kp = kp;
            cfg.kd = kd;
            cfg.base_speed = base;
            cfg.max_speed = max_s;
            cfg.brake_speed = brake;
            cfg.fork_mode = fork_m;
            cfg.line_color = color_l;

            Serial.println(F("$OK,PID_UPDATED"));
            sendConfig(cfg);
        }
        // Comando $CMD,...
        else if (strncmp(cmd, "$CMD,", 5) == 0) {
            char *action = cmd + 5;
            if (strcmp(action, "START") == 0) {
                state = STATE_RUNNING;
                Serial.println(F("$OK,STARTED"));
            } else if (strcmp(action, "STOP") == 0) {
                motors.stop();
                state = STATE_READY;
                Serial.println(F("$OK,STOPPED"));
            } else if (strcmp(action, "STREAM_ON") == 0) {
                telemetry_active = true;
                Serial.println(F("$OK,STREAM_ACTIVE"));
            } else if (strcmp(action, "STREAM_OFF") == 0) {
                telemetry_active = false;
                Serial.println(F("$OK,STREAM_MUTED"));
            } else if (strcmp(action, "CAL_BLACK") == 0) {
                state = STATE_CALIB_BLACK;
            } else if (strcmp(action, "CAL_WHITE") == 0) {
                state = STATE_CALIB_WHITE;
            }
        }
        // Comando $EEPROM,SAVE, $EEPROM,LOAD o $EEPROM,READ
        else if (strncmp(cmd, "$EEPROM,", 8) == 0) {
            char *sub = cmd + 8;
            if (strcmp(sub, "SAVE") == 0) {
                saveConfigEEPROM(cfg);
            } else if (strcmp(sub, "LOAD") == 0) {
                if (loadConfigEEPROM(cfg)) {
                    Serial.println(F("$OK,EEPROM_LOADED"));
                    sendConfig(cfg);
                } else {
                    Serial.println(F("$ERR,NO_EEPROM_DATA"));
                }
            } else if (strcmp(sub, "READ") == 0) {
                readConfigEEPROM();
            }
        }
        // Consulta $GET
        else if (strcmp(cmd, "$GET") == 0) {
            sendConfig(cfg);
        }
    }
};
