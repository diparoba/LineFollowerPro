# full-pid-and-unbounded-gains Specification

## Purpose
Define el ajuste de precisión de las ganancias del controlador PD en la interfaz gráfica con soporte para hasta 4 decimales (`step="0.0001"`), sin límites superiores artificiales, sliders adaptativos, etiquetas semánticas de Sensibilidad ($K_p$) y Corrección ($K_d$), y transmisión serial de alta resolución.

## Requirements

### Requirement: Unbounded 4-Decimal User Input for PD Gains
La interfaz de usuario SHALL permitir el ingreso de ganancias Proporcional ($K_p$) y Derivativa ($K_d$) con una resolución de 4 decimales (`step="0.0001"`), con límite inferior en $0.0000$ y sin límites superiores artificiales en las cajas de entrada numéricas (`<input type="number">`).

#### Scenario: Usuario ingresa valores decimales finos y altos
- **WHEN** el usuario ingresa un valor como `0.0005` o `35.7525` en cualquiera de las cajas numéricas de $K_p$ o $K_d$
- **THEN** la interfaz acepta el valor sin truncarlo ni forzarlo a rangos predeterminados y el slider se adapta dinámicamente

---

### Requirement: Dynamically Adaptive Range Sliders
Las barras deslizantes (`<input type="range">`) de la interfaz de usuario SHALL adaptar su valor máximo dinámicamente según el número digitado por el usuario (`max = Math.max(defaultMax, Math.ceil(valor))`), manteniendo la utilidad de control táctil independientemente de la magnitud de la ganancia.

#### Scenario: Expansión dinámica del slider ante valores altos
- **WHEN** el usuario digita un valor de ganancia mayor al rango visual base (por ejemplo $K_p = 5.2$ cuando la base es $1.5$)
- **THEN** el slider recalcula su límite superior a $6.0$ y posiciona la barra correspondientemente sin desbordamiento

---

### Requirement: Semantic Labels for Sensibilidad (Kp) and Corrección (Kd)
La interfaz de usuario en `TuningPanel.svelte` y `SimulationModal.svelte` SHALL rotular explícitamente el parámetro $K_p$ con la etiqueta de **Sensibilidad** y el parámetro $K_d$ con la etiqueta de **Corrección**, permitiendo una identificación inmediata del efecto dinámico de cada variable.

#### Scenario: Visualización de etiquetas en el panel de ajuste y simulador
- **WHEN** el usuario abre el panel de ajuste o la ventana de simulación
- **THEN** la interfaz muestra claramente las etiquetas "Sensibilidad (Kp)" y "Corrección (Kd)" con indicadores de su función de control

---

### Requirement: 4-Decimal High Precision Serial Transmission
El puente de comunicación serial SHALL formatear las constantes $K_p$ y $K_d$ con 4 decimales de precisión en la trama de comando `$PID,kp,kd,base,max,brake,fork,color` enviada hacia el microcontrolador.

#### Scenario: Envío de constantes finas al robot
- **WHEN** el usuario actualiza una ganancia fina como $K_p = 0.3275$ y presiona enviar o guardar
- **THEN** la trama transmitida contiene `$PID,0.3275,...` y el firmware responde con confirmación exitosa
