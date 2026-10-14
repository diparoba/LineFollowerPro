# Spec Delta: reflective-floor-and-bifurcation

## Purpose

Define el algoritmo de supresión de reflejos parásitos (Noise Gate, saturación y ventana espacial móvil), el sostenimiento temporal de 160 ms para detección de bifurcaciones en Y/T, el seguimiento adaptativo de bordes en sensores y el protocolo serial de conmutación de piso reflectante para seguidores de línea de 8 y 16 canales.

## ADDED Requirements

### Requirement: Anti-Glare Signal Conditioning and Spatial Window Filtering
El firmware SHALL acondicionar la señal de los sensores ópticos aplicando una compuerta de ruido para eliminar lecturas parásitas en fondo blanco y una compensación de saturación sobre cinta negra brillante cuando el modo reflectante esté habilitado.

#### Scenario: Noise gate and black line saturation in reflective mode
- **WHEN** el robot opera con `reflective_mode = 1` y muestrea los sensores en una superficie brillante
- **THEN** toda lectura normalizada menor o igual a 250 se anula a 0, toda lectura mayor o igual a 760 se expande al valor máximo de 1000, y los valores intermedios se escalan linealmente entre 0 y 1000.

#### Scenario: Dynamic spatial window tracking
- **WHEN** el robot avanza en modo reflectante sin haber perdido la línea
- **THEN** el cálculo de centroide limita la ventana de evaluación de sensores a un radio de $\pm 3$ canales en 8 líneas o $\pm 4$ canales en 16 líneas alrededor del último sensor activo, ignorando charcos de luz en los extremos.

#### Scenario: Preserving pure signal in mate/normal mode
- **WHEN** el robot opera con `reflective_mode = 0` (piso mate / normal)
- **THEN** el algoritmo evalúa la normalización directa de 0 a 1000 con umbral digital al 40% y todos los sensores habilitados sin compuertas adicionales.

---

### Requirement: Sustained 160ms Fork Detection and Edge Tracking
El algoritmo de navegación SHALL sostener la decisión de bifurcación de manera ininterrumpida durante una ventana temporal fija de 160 ms ante cruces en "Y" o líneas anchas, guiando el robot exclusivamente por la rama seleccionada.

#### Scenario: Detecting line split or wide path
- **WHEN** la barra de sensores detecta dos o más grupos de líneas independientes (`segments >= 2`) o un ancho continuo anormal de línea
- **THEN** el firmware activa la bandera de bifurcación (`bifurcation_detected = true`) y establece su vigencia continua por 160 ms (`bifurcation_active_until = now + 160`), con un período de enfriamiento post-maniobra de 400 ms.

#### Scenario: Following left branch (FORK_LEFT)
- **WHEN** la bifurcación está activa y el modo configurado es `FORK_LEFT`
- **THEN** el algoritmo restringe la ventana de centroide al sensor más a la izquierda que detecte línea (`last_active`) y su radio contiguo, ignorando completamente la rama derecha.

#### Scenario: Following right branch (FORK_RIGHT)
- **WHEN** la bifurcación está activa y el modo configurado es `FORK_RIGHT`
- **THEN** el algoritmo restringe la ventana de centroide al sensor más a la derecha que detecte línea (`first_active`) y su radio contiguo, ignorando completamente la rama izquierda.

#### Scenario: Following straight center (FORK_STRAIGHT)
- **WHEN** la bifurcación está activa y el modo configurado es `FORK_STRAIGHT`
- **THEN** el algoritmo descarta las ramas laterales y centra la ventana de cálculo en los sensores centrales del arreglo.

---

### Requirement: Dual-Mode Serial Protocol and EEPROM Persistence for Reflective Mode
El protocolo de comunicación serial SHALL permitir activar y consultar el modo reflectante mediante el 8vo parámetro en la trama `$PID`, comandos dedicados `$CMD,REFLECT_ON` / `$CMD,REFLECT_OFF` y persistirlo en la memoria EEPROM física del microcontrolador.

#### Scenario: Updating reflective mode via $PID command
- **WHEN** la aplicación transmite `$PID,kp,kd,base,max,brake,fork,color,reflective`
- **THEN** el firmware actualiza las ganancias, asigna `reflective_mode`, confirma con `$OK,PID_UPDATED` y retorna la trama `$CFG` reflejando el estado actual.

#### Scenario: Updating reflective mode via dedicated command
- **WHEN** la aplicación transmite `$CMD,REFLECT_ON` o `$CMD,REFLECT_OFF`
- **THEN** el robot conmuta inmediatamente el modo reflectante en RAM, responde con `$OK,REFLECTIVE_MODE_ON` o `$OK,REFLECTIVE_MODE_OFF` y emite la configuración actualizada.

#### Scenario: Storing reflective mode in physical EEPROM
- **WHEN** el comando `$EEPROM,SAVE` es recibido por el robot
- **THEN** el firmware graba la configuración incluyendo `reflective_mode` bajo la firma `EEPROM_MAGIC = 0x4C47` ("LG"), valida la lectura bit a bit y responde `$EEPROM_OK,SAVED,...` con destello en el LED 13.
