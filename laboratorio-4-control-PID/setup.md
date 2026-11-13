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
| `TIM4` | PSC `16-1` (tick de 1 µs), ARR `20000-1` (período 20 ms), canal 1 PWM | Pulso del servo a 50 Hz; el ancho en µs va directo al comparador |
| Reloj | HSI 16 MHz sin PLL | Igual que los labs anteriores |

## Compilar y grabar (VS Code)

1. Abrir la carpeta `firmware/` en VS Code.
2. `Ctrl+Shift+P` → "Tasks: Run Task" → **Configurar** (solo la primera vez).
3. Task **Compilar** (o `Ctrl+Shift+B`).
4. Conectar el USB mini del ST-LINK (CN1) y correr la task **Grabar en la placa**.

## Primera prueba, paso a paso

El programa arranca en **modo 0**: solo mide, con la viga horizontal. Todo se ajusta en vivo desde el debugger (`F5` y cargar en Live Expressions: `modo`, `distancia_cm`, `posicion_cm`, `centro_viga_cm`, `sentido`, `servo_centro_us`, `salida_angulo`, `pid_bola`).

Los LEDs dicen en qué estado está: **rojo** = no ve la bola, **verde** = bola centrada (±2 cm), **azul** = modo control activo.

1. **Viga horizontal.** Grabar y mirar la viga: si no queda horizontal, ajustar `servo_centro_us` en vivo hasta que quede (valores típicos: 1300 a 1700).
2. **El sensor ve la bola.** Poner la bola en la viga: el LED rojo se apaga y `distancia_cm` tiene que seguirla cuando la movés con la mano. Si marca cualquier cosa, revisar TRIG/ECHO.
3. **Medir el centro.** Con la bola quieta en el medio de la viga, leer `distancia_cm` y cargar ese valor en `centro_viga_cm`. Desde ahí `posicion_cm` queda en cero con la bola centrada. Ajustar `distancia_max_cm` al largo útil real si hace falta.
4. **Verificar el sentido.** Poner `modo = 1` con la bola a un costado: la viga tiene que inclinarse para empujarla hacia el centro. Si la empuja para afuera, poner `sentido = -1`.
5. **Controlar.** Con el sentido correcto ya debería intentar centrarla (arranca con las ganancias de la guía: Kp=1.5, Ki=0.2, Kd=0.05). Si oscila o queda corta, sintonizar con el procedimiento de [`explicacion.md`](./explicacion.md) editando `pid_bola.kp/ki/kd` en vivo.
6. **Guardar lo encontrado.** Los valores que funcionaron (`centro_viga_cm`, `servo_centro_us`, `sentido`, las ganancias) se pasan a los valores iniciales en `firmware/app/main_balancin.c` para que queden grabados.

Si en cualquier momento la bola se va de la viga o el sensor la pierde, el programa solo pone la viga horizontal y resetea el PID (LED rojo encendido); al volver a verla, retoma.
