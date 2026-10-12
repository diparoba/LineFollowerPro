#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

// ==============================================================================
// Firmware ESP32 DevKit V1 - Carro Robot Controlado por BLE 4.2
// Compatible con iOS (iPhone/iPad) y Android sin emparejamiento manual
// Incluye indicador visual mediante LED azul integrado en GPIO 2
// ==============================================================================

// Búfer fijo de 32 bytes con token de reemplazo único.
// La herramienta de flasheo web reemplaza "##BT_CAR_CUSTOM_NAME_TOKEN##"
// por el nombre configurado por el usuario antes de grabar la flash.
__attribute__((used)) const char BT_DEVICE_NAME[32] = "##BT_CAR_CUSTOM_NAME_TOKEN##\0\0\0";

// --- LED Integrado (GPIO 2 en ESP32 DevKit V1) ---
#define PIN_LED 2

// --- Asignación de Pines Driver TB6612FNG (ESP32 DevKit V1) ---
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

// Configuración LEDC PWM (1 kHz para óptimo par de arranque y evitar bloqueos)
#define PWM_FREQ 1000
#define PWM_RES  8
#define PWM_CH_A 0
#define PWM_CH_B 1

// Nordic UART Service (NUS) UUIDs universales para BLE RC Cars & Terminales
#define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

BLEServer *pServer = nullptr;
BLECharacteristic *pTxCharacteristic = nullptr;
bool deviceConnected = false;
bool oldDeviceConnected = false;

// Estado y temporizador para feedback visual del LED integrado
unsigned long lastRxActivityTime = 0;
bool rxActivityActive = false;

// Velocidad actual (0 - 255)
uint8_t currentSpeed = 200; // ~78%

void updateLedStatus() {
    unsigned long now = millis();

    // Si hubo recepción de comando, dar un pulso de parpadeo rápido
    if (rxActivityActive) {
        if (now - lastRxActivityTime < 80) {
            digitalWrite(PIN_LED, LOW); // Breve apagado para notar el destello
            return;
        } else {
            rxActivityActive = false;
        }
    }

    if (deviceConnected) {
        // Conectado por BLE: LED azul encendido fijo
        digitalWrite(PIN_LED, HIGH);
    } else {
        // Desconectado / Publicando BLE: Parpadeo cada 500ms
        digitalWrite(PIN_LED, (now % 1000) < 500 ? HIGH : LOW);
    }
}

void triggerLedPulse() {
    rxActivityActive = true;
    lastRxActivityTime = millis();
    digitalWrite(PIN_LED, LOW);
}

void setMotors(int speedA, int speedB) {
    // Control Motor A (Izquierdo)
    if (speedA > 0) {
        if (INVERT_MOTOR_A) {
            digitalWrite(PIN_AIN1, LOW);
            digitalWrite(PIN_AIN2, HIGH);
        } else {
            digitalWrite(PIN_AIN2, LOW);
            digitalWrite(PIN_AIN1, HIGH);
        }
        ledcWrite(PWM_CH_A, speedA > 255 ? 255 : speedA);
    } else if (speedA < 0) {
        if (INVERT_MOTOR_A) {
            digitalWrite(PIN_AIN2, LOW);
            digitalWrite(PIN_AIN1, HIGH);
        } else {
            digitalWrite(PIN_AIN1, LOW);
            digitalWrite(PIN_AIN2, HIGH);
        }
        ledcWrite(PWM_CH_A, (-speedA) > 255 ? 255 : -speedA);
    } else {
        digitalWrite(PIN_AIN1, LOW);
        digitalWrite(PIN_AIN2, LOW);
        ledcWrite(PWM_CH_A, 0);
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
        ledcWrite(PWM_CH_B, speedB > 255 ? 255 : speedB);
    } else if (speedB < 0) {
        if (INVERT_MOTOR_B) {
            digitalWrite(PIN_BIN2, LOW);
            digitalWrite(PIN_BIN1, HIGH);
        } else {
            digitalWrite(PIN_BIN1, LOW);
            digitalWrite(PIN_BIN2, HIGH);
        }
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
    triggerLedPulse();

    switch (cmd) {
        case 'F': // Adelante
            setMotors(currentSpeed, currentSpeed);
            break;
        case 'B': // Atrás
            setMotors(-currentSpeed, -currentSpeed);
            break;
        case 'L': // Giro Izquierda
            setMotors(-currentSpeed, currentSpeed);
            break;
        case 'R': // Giro Derecha
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
        case 'D':
            stopCar();
            break;

        // Comandos de velocidad universales (0 = 0% hasta 9 = 100%)
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            currentSpeed = (uint8_t)((cmd - '0') * 255 / 9);
            break;
        case 'q':
        case 'Q':
            currentSpeed = 255;
            break;

        default:
            break;
    }
}

// Callbacks del servidor BLE
class ServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
        deviceConnected = true;
    }

    void onDisconnect(BLEServer* pServer) {
        deviceConnected = false;
        stopCar();
    }
};

// Callbacks de la característica RX (Recepción de comandos desde App móvil BLE)
class RxCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
        std::string rxValue = pCharacteristic->getValue();
        if (rxValue.length() > 0) {
            for (int i = 0; i < rxValue.length(); i++) {
                executeCommand(rxValue[i]);
            }
        }
    }
};

void setup() {
    Serial.begin(115200);

    // Inicializar LED indicador azul (GPIO 2)
    pinMode(PIN_LED, OUTPUT);
    digitalWrite(PIN_LED, LOW);

    // Pines de salida TB6612FNG
    pinMode(PIN_AIN1, OUTPUT);
    pinMode(PIN_AIN2, OUTPUT);
    pinMode(PIN_BIN1, OUTPUT);
    pinMode(PIN_BIN2, OUTPUT);
    pinMode(PIN_STBY, OUTPUT);

    // Habilitar driver TB6612
    digitalWrite(PIN_STBY, HIGH);

    // Configuración PWM LEDC
    ledcSetup(PWM_CH_A, PWM_FREQ, PWM_RES);
    ledcAttachPin(PIN_PWMA, PWM_CH_A);

    ledcSetup(PWM_CH_B, PWM_FREQ, PWM_RES);
    ledcAttachPin(PIN_PWMB, PWM_CH_B);

    stopCar();

    // Obtener nombre BLE seguro
    String deviceName = String(BT_DEVICE_NAME);
    if (deviceName.length() == 0 || deviceName.startsWith("#")) {
        deviceName = "Carro_BLE_ESP32";
    }

    // Inicializar dispositivo BLE
    BLEDevice::init(deviceName.c_str());

    // Crear Servidor BLE
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new ServerCallbacks());

    // Crear Servicio Nordic UART (NUS)
    BLEService *pService = pServer->createService(SERVICE_UUID);

    // Crear Característica TX (Notificación)
    pTxCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID_TX,
        BLECharacteristic::PROPERTY_NOTIFY
    );
    pTxCharacteristic->addDescriptor(new BLE2902());

    // Crear Característica RX (Escritura desde móvil)
    BLECharacteristic *pRxCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID_RX,
        BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_WRITE_NR
    );
    pRxCharacteristic->setCallbacks(new RxCallbacks());

    // Iniciar servicio
    pService->start();

    // Iniciar Publicidad (Advertising)
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x06); // Funciones para conexión rápida en iOS/Android
    pAdvertising->setMinPreferred(0x12);
    BLEDevice::startAdvertising();

    Serial.print(F("[ESP32 Car] BLE 4.2 iniciado. Nombre: "));
    Serial.println(deviceName);
    Serial.println(F("[ESP32 Car] LED azul en GPIO 2 activo para feedback visual."));
}

void loop() {
    // Si se desconecta el dispositivo, reiniciar la publicidad automáticamente
    if (!deviceConnected && oldDeviceConnected) {
        delay(500); // Dar tiempo al stack bluetooth
        pServer->startAdvertising();
        Serial.println(F("[ESP32 Car] Dispositivo desconectado. Reiniciando publicidad BLE..."));
        oldDeviceConnected = deviceConnected;
    }

    // Si se acaba de conectar
    if (deviceConnected && !oldDeviceConnected) {
        oldDeviceConnected = deviceConnected;
        Serial.println(F("[ESP32 Car] Dispositivo BLE conectado exitosamente."));
    }

    // Comandos de respaldo por puerto serie USB
    if (Serial.available()) {
        char cmd = (char)Serial.read();
        executeCommand(cmd);
    }

    // Actualizar estado del LED azul en GPIO 2
    updateLedStatus();

    delay(10);
}
