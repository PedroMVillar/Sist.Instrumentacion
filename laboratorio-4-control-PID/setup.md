# Setup — Laboratorio 4 (ball and beam)

## Cableado

### HC-SR04 (igual que en el lab 3)

| Pin del sensor | Va a | Nota |
|---|---|---|
| VCC | 5V de la placa | El sensor necesita 5 V |
| TRIG | `PA2` | |
| ECHO | `PA1` | El pin es tolerante a 5 V |
| GND | GND | |

El sensor va montado en un extremo de la viga, apuntando a la bola.

### Servo

| Cable del servo | Va a | Nota |
|---|---|---|
| Señal (naranja/amarillo) | `PB6` | |
| V+ (rojo) | 5 V | Si el servo tiembla o la placa se resetea, alimentarlo con una fuente de 5 V aparte |
| GND (marrón/negro) | GND | **Siempre GND común con la placa**, aunque la fuente sea externa |

## Configuración (ya está en `firmware.ioc`)

| Periférico | Configuración | Para qué |
|---|---|---|
| `TIM2` | PSC `16-1` (tick de 1 µs), canal 2 Input Capture + interrupción | Mide el ancho del eco del HC-SR04 |
| `TIM4` | PSC `16-1` (tick de 1 µs), ARR `20000-1` (período 20 ms), canal 1 PWM | Genera el pulso del servo a 50 Hz; el ancho en µs se escribe directo en el comparador |
| Reloj | HSI 16 MHz sin PLL | Igual que los labs anteriores |

## Parámetros a ajustar con la maqueta armada

Están todos arriba de `firmware/app/main_balancin.c`:

| Define | Qué es | Cómo ajustarlo |
|---|---|---|
| `SETPOINT_CM` | Distancia sensor–centro de la viga | Poner la bola en el centro y leer `posicion` en Live Expressions |
| `SERVO_US_POR_GRADO` | Conversión de grados a µs de pulso | Si la viga corrige para el lado equivocado, cambiarle el signo |
| `ANGULO_MAX` | Inclinación máxima | Que la viga no choque contra nada en los extremos |
| `KP_INICIAL` etc. | Ganancias del PID | Con el procedimiento de `explicacion.md` |

Antes de poner la bola: grabar, verificar que la viga queda horizontal, y mover `setpoint` en Live Expressions para ver que el servo responde para el lado correcto.

## Compilar y grabar (VS Code)

1. Abrir la carpeta `firmware/` en VS Code.
2. `Ctrl+Shift+P` → "Tasks: Run Task" → **Configurar** (solo la primera vez).
3. Task **Compilar** (o `Ctrl+Shift+B`).
4. Conectar el USB mini del ST-LINK (CN1) y correr la task **Grabar en la placa**.

Para sintonizar: correr en modo Debug (`F5`) y usar Live Expressions con `posicion`, `salida_angulo`, `setpoint` y `pid_bola` (las ganancias `kp`, `ki`, `kd` se pueden editar en vivo, sin recompilar).
