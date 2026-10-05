#include <Arduino.h>
#include "BluetoothSerial.h"

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to enable it
#endif

BluetoothSerial SerialBT;

// Búfer fijo de 32 bytes con token de reemplazo único.
// El servicio de flasheo de la aplicación reemplazará "##BT_CAR_CUSTOM_NAME_TOKEN##"
// por el nombre configurado por el usuario en la interfaz gráfica antes de flashear.
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

// Inversión de Polaridad de Motores (true: invierte sentido de giro para corregir avance/giro)
#define INVERT_MOTOR_A true
#define INVERT_MOTOR_B true

// Configuración LEDC PWM (Ultrasónico 20 kHz para evitar silbido de motor)
#define PWM_FREQ 20000
#define PWM_RES  8
#define PWM_CH_A 0
#define PWM_CH_B 1

// Velocidad actual (0 - 255)
uint8_t currentSpeed = 200; // Por defecto ~78%

void setMotors(int speedA, int speedB) {
    // Control Motor A (Izquierdo)
    if (speedA > 0) {
        digitalWrite(PIN_AIN1, INVERT_MOTOR_A ? LOW : HIGH);
        digitalWrite(PIN_AIN2, INVERT_MOTOR_A ? HIGH : LOW);
        ledcWrite(PWM_CH_A, speedA > 255 ? 255 : speedA);
    } else if (speedA < 0) {
        digitalWrite(PIN_AIN1, INVERT_MOTOR_A ? HIGH : LOW);
        digitalWrite(PIN_AIN2, INVERT_MOTOR_A ? LOW : HIGH);
        ledcWrite(PWM_CH_A, (-speedA) > 255 ? 255 : -speedA);
    } else {
        digitalWrite(PIN_AIN1, LOW);
        digitalWrite(PIN_AIN2, LOW);
        ledcWrite(PWM_CH_A, 0);
    }

    // Control Motor B (Derecho)
    if (speedB > 0) {
        digitalWrite(PIN_BIN1, INVERT_MOTOR_B ? LOW : HIGH);
        digitalWrite(PIN_BIN2, INVERT_MOTOR_B ? HIGH : LOW);
        ledcWrite(PWM_CH_B, speedB > 255 ? 255 : speedB);
    } else if (speedB < 0) {
        digitalWrite(PIN_BIN1, INVERT_MOTOR_B ? HIGH : LOW);
        digitalWrite(PIN_BIN2, INVERT_MOTOR_B ? LOW : HIGH);
        ledcWrite(PWM_CH_B, (-speedB) > 255 ? 255 : -speedB);
    } else {
        digitalWrite(PIN_BIN1, LOW);
        digitalWrite(PIN_BIN2, LOW);
        ledcWrite(PWM_CH_B, 0);
    }
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
        case 'G': // Adelante + Izquierda
            setMotors(currentSpeed / 3, currentSpeed);
            break;
        case 'I': // Adelante + Derecha
            setMotors(currentSpeed, currentSpeed / 3);
            break;
        case 'H': // Reversa + Izquierda
            setMotors(-currentSpeed / 3, -currentSpeed);
            break;
        case 'J': // Reversa + Derecha
            setMotors(-currentSpeed, -currentSpeed / 3);
            break;
        case 'S': // Detener
        case 'D': // Detener todo
            stopCar();
            break;

        // Comandos de velocidad universales (0 a 9, q = 100%)
        case '0': currentSpeed = 0;   break;
        case '1': currentSpeed = 50;  break;
        case '2': currentSpeed = 75;  break;
        case '3': currentSpeed = 100; break;
        case '4': currentSpeed = 125; break;
        case '5': currentSpeed = 150; break;
        case '6': currentSpeed = 175; break;
        case '7': currentSpeed = 200; break;
        case '8': currentSpeed = 225; break;
        case '9': currentSpeed = 240; break;
        case 'q': currentSpeed = 255; break;

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

    // Habilitar driver TB6612 (STBY en HIGH)
    digitalWrite(PIN_STBY, HIGH);

    // Configuración PWM LEDC
    ledcSetup(PWM_CH_A, PWM_FREQ, PWM_RES);
    ledcAttachPin(PIN_PWMA, PWM_CH_A);

    ledcSetup(PWM_CH_B, PWM_FREQ, PWM_RES);
    ledcAttachPin(PIN_PWMB, PWM_CH_B);

    stopCar();

    // Inicializar Bluetooth Classic SPP con el nombre del dispositivo
    SerialBT.begin(BT_DEVICE_NAME);
    Serial.print(F("[ESP32 Car] Bluetooth Classic iniciado. Nombre: "));
    Serial.println(BT_DEVICE_NAME);
}

void loop() {
    if (SerialBT.available()) {
        char cmd = (char)SerialBT.read();
        executeCommand(cmd);
    }

    if (Serial.available()) {
        char cmd = (char)Serial.read();
        executeCommand(cmd);
    }
}
