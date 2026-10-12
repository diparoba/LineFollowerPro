#include <Arduino.h>
#include "BluetoothSerial.h"
#include "driver/gpio.h" // Fuerza de salida GPIO

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to enable it
#endif

BluetoothSerial SerialBT;

// Búfer fijo de 32 bytes con token de reemplazo único.
__attribute__((used)) const char BT_DEVICE_NAME[32] = "##BT_CAR_CUSTOM_NAME_TOKEN##\0\0\0";

// --- Asignación de Pines TB6612FNG (ESP32 DevKit V1) ---
// Motor A (Izquierdo)
#define PIN_PWMA 32
#define PIN_AIN1 25
#define PIN_AIN2 33

// Motor B (Derecho)
#define PIN_PWMB 18
#define PIN_BIN1 19
#define PIN_BIN2 27

// Standby
#define PIN_STBY 26

// Inversión de Polaridad de Motores
#define INVERT_MOTOR_A true
#define INVERT_MOTOR_B true

// Configuración LEDC PWM
#define PWM_FREQ 1000
#define PWM_RES  8
#define PWM_CH_A 0
#define PWM_CH_B 1

// Velocidad actual (0 - 255)
uint8_t currentSpeed = 200; // Por defecto ~78%

void setMotors(int speedA, int speedB) {
    // Apagar PWM momentáneamente para evitar picos inductivos durante la conmutación de dirección
    ledcWrite(PWM_CH_A, 0);
    ledcWrite(PWM_CH_B, 0);

    // Control Motor A (Izquierdo)
    if (speedA > 0) {
        if (INVERT_MOTOR_A) {
            digitalWrite(PIN_AIN1, LOW);
            digitalWrite(PIN_AIN2, HIGH);
        } else {
            digitalWrite(PIN_AIN2, LOW);
            digitalWrite(PIN_AIN1, HIGH);
        }
    } else if (speedA < 0) {
        if (INVERT_MOTOR_A) {
            digitalWrite(PIN_AIN2, LOW);
            digitalWrite(PIN_AIN1, HIGH);
        } else {
            digitalWrite(PIN_AIN1, LOW);
            digitalWrite(PIN_AIN2, HIGH);
        }
    } else {
        digitalWrite(PIN_AIN1, LOW);
        digitalWrite(PIN_AIN2, LOW);
    }

    // Control Motor B (Derecho)
    if (speedB > 0) {
        if (INVERT_MOTOR_B) {
            digitalWrite(PIN_BIN1, LOW);
            digitalWrite(PIN_BIN2, HIGH);
        } else {
            digitalWrite(PIN_BIN2, LOW);
            digitalWrite(PIN_BIN1, HIGH);
        }
    } else if (speedB < 0) {
        if (INVERT_MOTOR_B) {
            digitalWrite(PIN_BIN2, LOW);
            digitalWrite(PIN_BIN1, HIGH);
        } else {
            digitalWrite(PIN_BIN1, LOW);
            digitalWrite(PIN_BIN2, HIGH);
        }
    } else {
        digitalWrite(PIN_BIN1, LOW);
        digitalWrite(PIN_BIN2, LOW);
    }

    // Reactivar PWM con el ciclo de trabajo correspondiente
    int dutyA = constrain(abs(speedA), 0, 255);
    int dutyB = constrain(abs(speedB), 0, 255);
    ledcWrite(PWM_CH_A, dutyA);
    ledcWrite(PWM_CH_B, dutyB);
}

void stopCar() {
    setMotors(0, 0);
}

void executeCommand(char cmd) {
    switch (cmd) {
        case 'F': // Adelante
            setMotors(currentSpeed, currentSpeed);
            break;
        case 'B': // Atrás
            setMotors(-currentSpeed, -currentSpeed);
            break;
        case 'L': // Giro sobre su eje Izquierda
            setMotors(-currentSpeed, currentSpeed);
            break;
        case 'R': // Giro sobre su eje Derecha
            setMotors(currentSpeed, -currentSpeed);
            break;
        case 'S': // Detener
            stopCar();
            break;

        // Comandos de velocidad individuales según tu calibración
        case '0':
            currentSpeed = 60;
            break;
        case '1':
            currentSpeed = 70;
            break;
        case '2':
            currentSpeed = 80;
            break;
        case '3':
            currentSpeed = 90;
            break;
        case '4':
            currentSpeed = 100;
            break;
        case '5':
            currentSpeed = 130;
            break;
        case '6':
            currentSpeed = 160;
            break;
        case '7':
            currentSpeed = 190;
            break;
        case '8':
            currentSpeed = 220;
            break;
        case '9':
            currentSpeed = 255;
            break;

        default:
            break;
    }
}

void setup() {
    Serial.begin(115200);

    // Pines de salida para control de dirección y standby
    pinMode(PIN_AIN1, OUTPUT);
    pinMode(PIN_AIN2, OUTPUT);
    pinMode(PIN_BIN1, OUTPUT);
    pinMode(PIN_BIN2, OUTPUT);
    pinMode(PIN_STBY, OUTPUT);

    // Refuerzo de corriente de salida (Drive Strength) en pines clave para la PCB
    gpio_set_drive_capability((gpio_num_t)PIN_AIN1, GPIO_DRIVE_CAP_3);
    gpio_set_drive_capability((gpio_num_t)PIN_AIN2, GPIO_DRIVE_CAP_3); // Refuerza GPIO 33
    gpio_set_drive_capability((gpio_num_t)PIN_BIN1, GPIO_DRIVE_CAP_3); // Refuerza GPIO 19
    gpio_set_drive_capability((gpio_num_t)PIN_BIN2, GPIO_DRIVE_CAP_3);

    // Habilitar driver TB6612 (STBY en HIGH)
    digitalWrite(PIN_STBY, HIGH);

    // Configuración PWM LEDC
    ledcSetup(PWM_CH_A, PWM_FREQ, PWM_RES);
    ledcAttachPin(PIN_PWMA, PWM_CH_A);

    ledcSetup(PWM_CH_B, PWM_FREQ, PWM_RES);
    ledcAttachPin(PIN_PWMB, PWM_CH_B);

    stopCar();

    // Inicializar Bluetooth Classic SPP
    SerialBT.begin(BT_DEVICE_NAME);
    Serial.print(F("[ESP32 Car] Bluetooth Classic iniciado. Nombre: "));
    Serial.println(BT_DEVICE_NAME);
}

void loop() {
    if (SerialBT.available()) {
        char cmd = (char)SerialBT.read();
        executeCommand(cmd);
    }
}