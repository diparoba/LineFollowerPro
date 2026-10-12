#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <esp32-hal-rgb-led.h>

// ==============================================================================
// Firmware ESP32-S3 DevKit 2022 v1.3 - Carro Robot Controlado por BLE
// Incluye indicador visual mediante LED NeoPixel integrado en GPIO 48
// ==============================================================================

// Búfer fijo de 32 bytes con token de reemplazo único.
// La herramienta de flasheo web reemplaza "##BT_CAR_CUSTOM_NAME_TOKEN##"
// por el nombre configurado por el usuario antes de grabar la flash.
__attribute__((used)) const char BT_DEVICE_NAME[32] = "##BT_CAR_CUSTOM_NAME_TOKEN##\0\0\0";

// --- LED NeoPixel Integrado (GPIO 48 en ESP32-S3 DevKit) ---
#define PIN_NEOPIXEL 48

// --- Asignación de Pines Driver TB6612FNG (ESP32-S3) ---
// Motor A (Izquierdo)
#define PIN_PWMA 4
#define PIN_AIN1 5
#define PIN_AIN2 6

// Motor B (Derecho)
#define PIN_PWMB 7
#define PIN_BIN1 15
#define PIN_BIN2 16

// Standby
#define PIN_STBY 17

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

// Estado y temporizador para feedback visual del NeoPixel
unsigned long lastRxActivityTime = 0;
bool rxActivityActive = false;
uint8_t rxActiveR = 0;
uint8_t rxActiveG = 255;
uint8_t rxActiveB = 0;

// Velocidad actual (0 - 255)
uint8_t currentSpeed = 200; // ~78%

void setNeoPixel(uint8_t r, uint8_t g, uint8_t b) {
    neopixelWrite(PIN_NEOPIXEL, r, g, b);
}

void updateNeoPixelStatus() {
    unsigned long now = millis();

    // Si hubo recepción reciente de comando BLE, mostrar destello de actividad
    if (rxActivityActive) {
        if (now - lastRxActivityTime < 100) {
            setNeoPixel(rxActiveR, rxActiveG, rxActiveB);
            return;
        } else {
            rxActivityActive = false;
        }
    }

    if (deviceConnected) {
        // Conectado por BLE y esperando comandos: Verde/Cian suave
        setNeoPixel(0, 30, 20);
    } else {
        // Desconectado / Publicando anuncio BLE: Pulso suave Azul
        // Efecto respiración suave entre 5 y 40 de brillo
        uint8_t breath = (uint8_t)(15 + 15 * sin((now % 2000) * (2.0 * PI / 2000.0)));
        setNeoPixel(0, 0, breath);
    }
}

void triggerNeoPixelPulse(uint8_t r, uint8_t g, uint8_t b) {
    rxActiveR = r;
    rxActiveG = g;
    rxActiveB = b;
    rxActivityActive = true;
    lastRxActivityTime = millis();
    setNeoPixel(r, g, b);
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
    // Señal visual en NeoPixel al procesar cada instrucción
    switch (cmd) {
        case 'F': // Adelante (Verde brillante)
            triggerNeoPixelPulse(0, 255, 0);
            setMotors(currentSpeed, currentSpeed);
            break;
        case 'B': // Atrás (Azul/Violeta)
            triggerNeoPixelPulse(120, 0, 255);
            setMotors(-currentSpeed, -currentSpeed);
            break;
        case 'L': // Giro Izquierda (Ámbar / Naranja)
            triggerNeoPixelPulse(255, 140, 0);
            setMotors(-currentSpeed, currentSpeed);
            break;
        case 'R': // Giro Derecha (Ámbar / Naranja)
            triggerNeoPixelPulse(255, 140, 0);
            setMotors(currentSpeed, -currentSpeed);
            break;
        case 'G': // Adelante + Izquierda
            triggerNeoPixelPulse(0, 255, 100);
            setMotors(currentSpeed / 3, currentSpeed);
            break;
        case 'I': // Adelante + Derecha
            triggerNeoPixelPulse(0, 255, 100);
            setMotors(currentSpeed, currentSpeed / 3);
            break;
        case 'H': // Reversa + Izquierda
            triggerNeoPixelPulse(150, 0, 200);
            setMotors(-currentSpeed / 3, -currentSpeed);
            break;
        case 'J': // Reversa + Derecha
            triggerNeoPixelPulse(150, 0, 200);
            setMotors(-currentSpeed, -currentSpeed / 3);
            break;
        case 'S': // Detener (Rojo)
        case 'D':
            triggerNeoPixelPulse(255, 0, 0);
            stopCar();
            break;

        // Comandos de velocidad universales (0 = 0% hasta 9 = 100%) (Destello Blanco)
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
            triggerNeoPixelPulse(100, 100, 100);
            break;
        case 'q':
        case 'Q':
            currentSpeed = 255;
            triggerNeoPixelPulse(100, 100, 100);
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

    // Inicializar LED NeoPixel
    setNeoPixel(0, 0, 50); // Azul tenue inicial

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
        deviceName = "Carro_S3_BLE";
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

    Serial.print(F("[ESP32-S3 Car] BLE iniciado. Nombre: "));
    Serial.println(deviceName);
    Serial.println(F("[ESP32-S3 Car] NeoPixel en GPIO 48 activo para feedback visual."));
}

void loop() {
    // Si se desconecta el dispositivo, reiniciar la publicidad automáticamente
    if (!deviceConnected && oldDeviceConnected) {
        delay(500); // Dar tiempo al stack bluetooth
        pServer->startAdvertising();
        Serial.println(F("[ESP32-S3 Car] Dispositivo desconectado. Reiniciando publicidad BLE..."));
        oldDeviceConnected = deviceConnected;
    }

    // Si se acaba de conectar
    if (deviceConnected && !oldDeviceConnected) {
        oldDeviceConnected = deviceConnected;
        Serial.println(F("[ESP32-S3 Car] Dispositivo BLE conectado exitosamente."));
    }

    // Comandos de respaldo por puerto serie USB
    if (Serial.available()) {
        char cmd = (char)Serial.read();
        executeCommand(cmd);
    }

    // Actualizar color y estado del LED NeoPixel en GPIO 48
    updateNeoPixelStatus();

    delay(10);
}
