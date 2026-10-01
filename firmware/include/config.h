#pragma once
#include <Arduino.h>

// Si no se define bandera en PlatformIO (ej. al compilar directamente en Arduino IDE),
// se selecciona por defecto ROBOT_CODEX_8 para asegurar que A0..A5 se configuren como ENTRADAS
// y evitar colisiones lógicas con los sensores analógicos.
#if !defined(ROBOT_IM_16) && !defined(ROBOT_CODEX_8)
#define ROBOT_CODEX_8
#endif

#if defined(ROBOT_CODEX_8)
// ==========================================
// ASIGNACIÓN DE PINES - SENSOR 8 LÍNEAS (CODEX ANALÓGICO DIRECTO)
// ==========================================
#define NUM_SENSORS        8
#define SENSOR_SETPOINT    3500
#define SENSOR_MAX_POS     7000

// Canales analógicos directos de A0 a A7
// A0-A5 (PC0-PC5) y A6-A7 (entradas analógicas puras ADC6/ADC7 de ATmega328P)
static const uint8_t SENSOR_PINS[NUM_SENSORS] = {A0, A1, A2, A3, A4, A5, A6, A7};

#else
// ==========================================
// ASIGNACIÓN DE PINES - SENSOR 16 LÍNEAS (INGENIERO MAKER MULTIPLEXADO)
// ==========================================
#define NUM_SENSORS        16
#define SENSOR_SETPOINT    7500
#define SENSOR_MAX_POS     15000

#define PIN_LON       A0  // D14 (PC0) - Habilita los LEDs infrarrojos (HIGH = ON)
#define PIN_S0        A1  // D15 (PC1) - Bit 0 multiplexor
#define PIN_S1        A2  // D16 (PC2) - Bit 1 multiplexor
#define PIN_S2        A3  // D17 (PC3) - Bit 2 multiplexor
#define PIN_S3        A4  // D18 (PC4) - Bit 3 multiplexor
#define PIN_OM        A5  // D19 (PC5) - Salida analógica multiplexor
#endif

// ==========================================
// ASIGNACIÓN DE PINES - USUARIO E INTERFAZ
// ==========================================
#define PIN_BOTON     2   // D2 (PD2)  - Pulsador de usuario (Pull-up)
#define PIN_LED       13  // D13 (PB5) - LED indicador integrado

// ==========================================
// ASIGNACIÓN DE PINES - PUENTE H (MOTORES)
// ==========================================
// Standby Driver Puente H TB6612FNG (Pin D8 en nueva PCB)
#define PIN_STBY      8   // D8 (PB0)  - HIGH = Driver Habilitado, LOW = Standby/Apagado

// Motor Derecho (Pines 3, 4, 5)
#define PIN_PWM_DER   3   // D3 (PD3 - OC2B)
#define PIN_DER_IN1   4   // D4 (PD4)
#define PIN_DER_IN2   5   // D5 (PD5)

// Motor Izquierdo (Pines 11, 10, 9)
#define PIN_PWM_IZQ   11  // D11 (PB3 - OC2A)
#define PIN_IZQ_IN1   10  // D10 (PB2)
#define PIN_IZQ_IN2   9   // D9  (PB1)

// ==========================================
// MODOS DE BIFURCACIÓN / CRUCE
// ==========================================
enum BifurcationMode : uint8_t {
    FORK_STRAIGHT = 0, // Seguir trayectoria recta / central
    FORK_LEFT     = 1, // Seguir rama izquierda al detectar bifurcación
    FORK_RIGHT    = 2  // Seguir rama derecha al detectar bifurcación
};

// ==========================================
// ESTADOS DE LA MÁQUINA DE CONTROL
// ==========================================
enum RobotState : uint8_t {
    STATE_WAIT_CALIB_BLACK = 0, // Esperando 1er toque: Calibrar Negro
    STATE_CALIB_BLACK      = 1, // Muestreando superficie negra
    STATE_WAIT_CALIB_WHITE = 2, // Esperando 2do toque: Calibrar Blanco
    STATE_CALIB_WHITE      = 3, // Muestreando superficie blanca
    STATE_READY            = 4, // Calibrado: Esperando 3er toque para arrancar
    STATE_RUNNING          = 5  // En carrera: Seguidor PD activo
};

// ==========================================
// ESTRUCTURA DE CONFIGURACIÓN PD Y CONTROL
// ==========================================
struct RobotConfig {
    float kp;               // Ganancia Proporcional (ej: 0.18 - 0.40)
    float kd;               // Ganancia Derivativa (ej: 2.5 - 6.0)
    int16_t base_speed;     // Velocidad base crucero (0 - 255)
    int16_t max_speed;      // Límite de velocidad máxima (0 - 255)
    int16_t brake_speed;    // Intensidad de frenado activo en curvas (0 - 255)
    uint8_t fork_mode;      // Modo de bifurcación (0: Recto, 1: Izq, 2: Der)
    uint8_t line_color;     // 0 = Línea negra sobre fondo blanco, 1 = Línea blanca sobre fondo negro
};

// ==========================================
// DIRECCIÓN EEPROM
// ==========================================
#define EEPROM_MAGIC 0x4C46  // "LF" (Line Follower)
#define EEPROM_ADDR  0
