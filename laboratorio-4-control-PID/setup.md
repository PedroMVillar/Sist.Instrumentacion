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

### Consola

Nada que cablear: un cable micro-USB común entre la PC y el conector **CN5**
de la placa (el micro-USB de abajo, que va directo al F407). Al conectarlo,
con el programa ya grabado, aparece un puerto COM nuevo en el Administrador
de dispositivos ("Dispositivo serie USB (COMx)"). Quedan los dos cables
puestos: el mini-USB del ST-LINK (CN1) para grabar y el micro-USB (CN5) para
la consola.

La misma consola también sale por la UART de PA2/PA3 (115200), por si algún
día se usa un adaptador USB-serie.

## Configuración (ya está en `firmware.ioc`)

| Periférico | Configuración | Para qué |
|---|---|---|
| Reloj | HSI + PLL a 96 MHz | Los timers de APB1 reciben 48 MHz; el USB recibe sus 48 MHz del PLL |
| `TIM3` | PSC `48-1` (tick de 1 µs), canal 4 Input Capture + interrupción | Mide el ancho del eco del HC-SR04 |
| `TIM2` | Se configura a mano en `main_balancin.c` (`Servo_Init`), canal 2 PWM en PA1 | Pulso del servo |
| `USB CDC` | USB device en CN5, generado por CubeMX | Consola: aparece como puerto COM en la PC |
| `USART2` | 115200, 8N1, interrupción de recepción | La misma consola por PA2/PA3 (alternativa) |

## Compilar y grabar (VS Code)

1. Abrir la carpeta `laboratorio-4-control-PID/` en VS Code (acá mismo: están el firmware, la consola y este instructivo juntos).
2. `Ctrl+Shift+P` → "Tasks: Run Task" → **Configurar** (solo la primera vez).
3. Task **Compilar** (o `Ctrl+Shift+B`).
4. Conectar el USB mini del ST-LINK (CN1) y correr la task **Grabar en la placa**.

## La consola

Con la placa grabada y el USB conectado:

```
pip install pyserial matplotlib        (solo la primera vez)
python consola/consola.py COM5         (poner el COM que aparezca en tu PC)
```

También hay una task **Consola** en VS Code que hace lo mismo: te pregunta el COM y la abre.

Se abren dos cosas: la terminal, que muestra en vivo la telemetría
(`modo`, `distancia`, `posición`, `ángulo`, `servo`) y una ventana con el
**gráfico en tiempo real** de la posición de la pelota respecto del centro.
Ahí se ve cómo oscila y se va estabilizando cuando el PID controla.

Lo que escribas en la terminal va a la placa como comando `nombre valor`:

| Comando | Qué cambia | Ejemplo |
|---|---|---|
| `modo` | 0 = solo medir, 1 = PID, 2 = prueba de sentido | `modo 1` |
| `kp` / `ki` / `kd` | las ganancias del PID | `kp 3` |
| `centro` | distancia del sensor al centro de la viga (cm) | `centro 28.5` |
| `sentido` | 1 o -1, para qué lado corrige el servo | `sentido -1` |
| `servo` | pulso con el que la viga queda horizontal (µs) | `servo 1200` |
| `max` | distancia máxima válida del sensor (cm) | `max 55` |

La placa contesta `ok kp = 3.000` a cada comando.

## Primera prueba, paso a paso

El programa arranca en **modo 0** (solo mide, viga horizontal) y con los
valores que ya anduvieron en la maqueta: `servo 1200`, `sentido -1`,
`centro 28.5`, Kp=3, Ki=0.2, Kd=0.005.

1. **Viga horizontal.** Grabar, abrir la consola y mirar la viga: si no
   queda horizontal, ajustar con `servo 1250`, `servo 1150`... hasta que quede.
2. **El sensor ve la pelota.** Ponerla en la viga: el LED rojo se apaga y la
   distancia de la consola tiene que seguirla al moverla con la mano. Si
   marca cualquier cosa, revisar TRIG/ECHO.
3. **Verificar el centro.** Con la pelota quieta en el medio, la posición
   tiene que marcar cerca de 0. Si no, mandar la distancia leída con `centro`.
4. **Verificar el sentido.** `modo 2` con la pelota cerca del sensor: la viga
   se inclina fija y la pelota tiene que rodar **alejándose** del sensor. Si
   rueda hacia el sensor, cambiar el signo con `sentido 1` (o `-1`). Volver a `modo 0`.
5. **Controlar.** `modo 1`: debería centrarla. En el gráfico se ve la
   oscilación achicándose. Si oscila de más o queda corta, tocar `kp`, `ki`,
   `kd` (procedimiento en [`explicacion.md`](./explicacion.md)).
6. **Guardar lo encontrado.** Los valores que funcionaron se pasan a los
   valores iniciales de `firmware/app/main_balancin.c` para que queden grabados.

Si la pelota se va de la viga o el sensor la pierde, el programa solo pone la
viga horizontal y resetea el PID (LED rojo encendido); al volver a verla, retoma.
