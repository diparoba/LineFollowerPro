# codex8-hardware-and-telemetry Specification

## Purpose
Define el soporte de hardware para puentes H con pin de activación STBY conectado al pin digital D8, la lectura directa de 8 sensores analógicos en pines A0 a A7 sin dependencias de librerías externas y la emisión inmediata de telemetría serial continua en el arranque.

## Requirements

### Requirement: Motor Driver Standby Activation on D8
El firmware SHALL inicializar el pin digital 8 (`D8`) en modo `OUTPUT` y colocarlo en nivel lógico `HIGH` durante el arranque para habilitar los puentes H del controlador de motores (como TB6612FNG).

#### Scenario: Puente H habilitado en inicialización
- **WHEN** el microcontrolador se enciende o reinicia
- **THEN** el pin digital 8 se configura como salida y se establece en nivel alto antes de aceptar comandos de tracción

---

### Requirement: Direct 8-Channel Analog Sensing Without External Libraries
El firmware SHALL leer los 8 canales analógicos conectados directamente a los pines A0, A1, A2, A3, A4, A5, A6 y A7 utilizando exclusivamente rutinas C++ nativas del microcontrolador ATmega328P sin librerías externas, garantizando tiempo suficiente de estabilización en el multiplexor del ADC para fototransistores de alta impedancia.

#### Scenario: Muestreo analógico limpio y sin diafonía
- **WHEN** se ejecuta la rutina de lectura de la regleta de 8 sensores en reposo o carrera
- **THEN** el conversor analógico-digital muestrea secuencialmente los canales A0 a A7 arrojando lecturas analógicas limpias y normalizadas entre 0 y 1000 puntos

---

### Requirement: Immediate Continuous Telemetry On Startup
El firmware SHALL iniciar con la telemetría serial continua habilitada por defecto (`telemetry_active = true`), transmitiendo tramas `$TEL` periódicas a 25 Hz inmediatamente después del arranque por el puerto serie a 115200 baudios sin requerir un comando de enlace previo.

#### Scenario: Visualización inmediata en monitor serial
- **WHEN** el usuario conecta el robot por USB y abre el monitor serial o la aplicación de escritorio
- **THEN** el sistema transmite de inmediato las tramas de telemetría con los valores en bruto de los 8 sensores

---

### Requirement: Multi-Environment Safe Compilation (Arduino IDE and PlatformIO)
El firmware SHALL proporcionar selectores explícitos en `config.h` para que la arquitectura de 8 sensores directos pueda compilarse de manera segura tanto en PlatformIO como en el IDE oficial de Arduino, evitando que los pines A0 a A4 se configuren erróneamente como salidas digitales de multiplexor.

#### Scenario: Compilación directa en Arduino IDE
- **WHEN** el código fuente se abre y compila en Arduino IDE sin banderas especiales de línea de comandos
- **THEN** el firmware compila con la configuración de 8 sensores analógicos directos y pines A0 a A5 como entradas
