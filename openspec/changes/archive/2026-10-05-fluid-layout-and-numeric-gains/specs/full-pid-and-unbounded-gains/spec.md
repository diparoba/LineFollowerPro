# Spec Delta: full-pid-and-unbounded-gains

## MODIFIED Requirements

### Requirement: Unbounded 4-Decimal User Input for PD Gains
La interfaz de usuario SHALL permitir el ingreso de ganancias Proporcional ($K_p$) y Derivativa ($K_d$) con una resolución de 4 decimales (`step="0.0001"`), con límite inferior en $0.0000$ y sin límites superiores artificiales en cajas de entrada numéricas dedicadas (`<input type="number">`), omitiendo controles de rango deslizante (sliders) para estas variables de ganancia.

#### Scenario: Usuario ingresa valores decimales finos y altos
- **WHEN** el usuario ingresa un valor como `0.0005` o `35.7525` en cualquiera de las cajas numéricas de $K_p$ o $K_d$
- **THEN** la interfaz acepta el valor numérico con hasta 4 decimales sin truncarlo ni requerir interacción con una barra deslizante (slider)

## REMOVED Requirements

### Requirement: Dynamically Adaptive Range Sliders
**Reason**: Los usuarios necesitan ajustar $K_p$ y $K_d$ con precisión matemática directa de 4 decimales mediante teclado numérico; las barras deslizantes para ganancias resultaban imprecisas e innecesarias en pantallas de alta densidad.
**Migration**: Usar exclusivamente los campos numéricos `<input type="number" step="0.0001" min="0">` de $K_p$ y $K_d$. Los sliders se mantienen únicamente para velocidades del motor y freno.
