# Setup — Laboratorio 4 (ball and beam)

## Cableado

### HC-SR04

| Pin del sensor | Va a | Nota |
|---|---|---|
| VCC | 5V de la placa | El sensor necesita 5 V |
| TRIG | `PB0` | |
| ECHO | `PB1` | El pin es tolerante a 5 V |
| GND | GND | |

El sensor va montado en un extremo de la viga, apuntando a la pelota.

### Servo

| Cable del servo | Va a | Nota |
|---|---|---|
| Señal (naranja/amarillo) | `PA1` | |
| V+ (rojo) | 5 V | Si el servo tiembla o la placa se resetea, alimentarlo con una fuente de 5 V aparte |
| GND (marrón/negro) | GND | **Siempre GND común con la placa**, aunque la fuente sea externa |

### Adaptador USB-UART (para el gráfico)

| Pin del adaptador | Va a | Nota |
|---|---|---|
| RX | `PA2` (TX de la placa) | Por acá llegan las muestras |
| TX | `PA3` (RX de la placa) | No se usa todavía, se puede dejar sin conectar |
| GND | GND | **Siempre GND común** |

El adaptador tiene que trabajar con lógica de 3.3 V (si tiene jumper 5V/3V3,
ponerlo en 3V3). El pin de VCC del adaptador no se conecta.

## Configuración (ya está en `firmware.ioc`)

| Periférico | Configuración | Para qué |
|---|---|---|
| Reloj | HSI + PLL a 96 MHz | Los timers de APB1 reciben 48 MHz |
| `TIM3` | PSC `48-1` (tick de 1 µs), canal 4 Input Capture + interrupción | Mide el ancho del eco del HC-SR04 |
| `USART2` | 115200, 8N1, PA2/PA3 | Manda las muestras al gráfico |
| `TIM4` | Se configura a mano en `main_balancin.c` (`Grafico_Init`), interrupción cada 50 ms | Marca cuándo mandar una muestra |
| `TIM2` | Se configura a mano en `main_balancin.c` (`Servo_Init`), canal 2 PWM en PA1 | Pulso del servo |

## Compilar y grabar (VS Code)

1. Abrir la carpeta `laboratorio-4-control-PID/` en VS Code (acá mismo: están el firmware y este instructivo juntos).
2. `Ctrl+Shift+P` → "Tasks: Run Task" → **Configurar** (solo la primera vez).
3. Task **Compilar** (o `Ctrl+Shift+B`).
4. Conectar el USB mini del ST-LINK (CN1) y correr la task **Grabar en la placa**.

## Gráfico en tiempo real (para calibrar)

Cada 50 ms el `TIM4` interrumpe y la placa manda por la UART una línea con
el tiempo y la distancia de la pelota al centro, por ejemplo `15350,-2.41`.
La placa no espera a que la línea termine de salir: `HAL_UART_Transmit_IT`
la manda sola por interrupción, así el lazo del PID no se frena.

Con la placa grabada y el adaptador conectado:

```
pip install pyserial matplotlib        (solo la primera vez)
python grafico/grafico.py COM7         (poner el COM del adaptador)
```

También está la task **Grafico** en VS Code, que pregunta el COM.

Se abre una ventana con la posición respecto del centro en los últimos 20
segundos. Una pelota bien calibrada oscila un par de veces y queda pegada a
la línea del 0. El gráfico funciona a la vez que el depurador: se puede
cambiar `kp`, `ki`, `kd` en Live Expressions y ver el efecto en el gráfico.

## Ajustar en vivo (Live Expressions)

Los valores se miran y se cambian con la placa corriendo, desde el depurador
(Run and Debug en VS Code, panel de variables / Live Expressions). Para
cambiar uno, se edita su valor ahí mismo, sin recompilar.

| Variable | Qué es | Valor inicial |
|---|---|---|
| `modo` | 0 = solo medir, 1 = PID | 1 |
| `setpoint_cm` | posición pedida respecto del centro (cm); para ensayos de escalón | 0 |
| `pid_bola.kp` / `.ki` / `.kd` | ganancias del PID (las del ensayo de escalón) | 1.5 / 0 / 0.8 |
| `centro_viga_cm` | distancia del sensor al centro de la viga (cm) | 33 |
| `sentido` | 1 o -1, para qué lado corrige el servo | -1 |
| `servo_centro_us` | pulso con el que la viga queda horizontal (µs) | 1315 |
| `banda_cm` | zona muerta alrededor del centro (cm); 0 = apagada | 0 |
| `velocidad_servo` | grados por segundo del servo; 0 = de golpe | 150 |
| `distancia_max_cm` | distancia máxima válida del sensor (cm) | 55 |

Para mirar: `distance_cm` (lectura cruda), `distancia_filtrada_cm` (mediana
de las últimas 5), `posicion_cm`, `salida_angulo`, `angulo_servo`, `servo_us`.

## Primera prueba, paso a paso

El programa arranca en **modo 1** (controlando) con los valores que ya
anduvieron en la maqueta. Si algo no va, poner `modo` en 0 y seguir estos pasos:

1. **Viga horizontal.** Con `modo` 0 la viga queda horizontal: si no lo está,
   ajustar `servo_centro_us` de a 50 hasta que la pelota quede quieta.
2. **El sensor ve la pelota.** Ponerla en la viga: el LED rojo se apaga y
   `distance_cm` tiene que seguirla al moverla con la mano. Si marca
   cualquier cosa, revisar TRIG/ECHO.
3. **Verificar el centro.** Con la pelota quieta en el medio, `posicion_cm`
   tiene que dar cerca de 0. Si no, poner en `centro_viga_cm` la distancia leída.
4. **Verificar el sentido.** Con `modo` 1, acercar la pelota al sensor: la viga
   tiene que inclinarse para que ruede **alejándose** del sensor. Si la empuja
   hacia el sensor, cambiar el signo de `sentido`.
5. **Controlar.** `modo` 1: debería centrarla (LED verde dentro de la banda).
   Si oscila de más o queda corta, tocar `kp`, `ki`, `kd` (procedimiento en
   [`explicacion.md`](./explicacion.md)).
6. **Guardar lo encontrado.** Los valores que funcionaron se pasan a los
   valores iniciales de `firmware/app/main_balancin.c` para que queden grabados.

Si la pelota se va de la viga o el sensor la pierde, el programa solo pone la
viga horizontal y resetea el PID (LED rojo encendido); al volver a verla, retoma.
