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
el tiempo, la distancia de la pelota al centro, el setpoint y la salida del
PID en grados, por ejemplo `15350,-2.41,0.00,3.62`. Si en `main_balancin.c`
se pone `LOG_EXTENDIDO` en 0, manda solo las dos primeras.
La placa no espera a que la línea termine de salir: `HAL_UART_Transmit_IT`
la manda sola por interrupción, así el lazo del PID no se frena.

Con la placa grabada y el adaptador conectado:

```
pip install pyserial matplotlib        (solo la primera vez)
python grafico/grafico.py              (busca solo el adaptador; o agregar el COM, ej. COM8)
```

También está la task **Grafico** en VS Code. Además de graficar, guarda todo
lo que recibe en `matlab/log_escalon.csv`. Ese archivo se pisa cada vez que
se abre el gráfico.

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

El programa arranca en **modo 1** (controlando) con las ganancias del ensayo
de escalón. Si algo no va, poner `modo` en 0 y seguir estos pasos:

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

## Ensayo de escalón e identificación (con MATLAB)

Sirve para medir la planta y elegir las ganancias con un modelo, en vez de a
ojo. Antes hacer la "Primera prueba" de arriba (viga horizontal, centro y
sentido verificados).

Todos los comandos de terminal se escriben parado en la carpeta
`laboratorio-4-control-PID` (la terminal de VS Code ya abre ahí).

### 1. Preparar

1. Conectar el ST-LINK (USB mini, CN1) y el adaptador USB-UART.
2. Ir a Run and Debug (`Ctrl+Shift+D`), elegir **Debug en la placa (OpenOCD)**
   y apretar `F5`. Eso compila, graba la placa y la deja frenada al
   principio de `main`. Apretar `F5` otra vez para que corra.
   (No hace falta la task **Grabar en la placa**: el debug ya graba.)
3. En el panel **WATCH** de Run and Debug, con el `+`, agregar:
   `modo`, `setpoint_cm`, `banda_cm`, `pid_bola`, `posicion_cm`.
4. Revisar que `pid_bola` tenga `kp = 1.5`, `ki = 0`, `kd = 0.8` y que
   `banda_cm` sea 0. Si se usan otras ganancias, poner las mismas en la
   sección 1 del script (`P.Kp`, `P.Ki`, `P.Kd`), si no el modelo sale mal.

**Cómo cambiar una variable:** `F6` (pausa) → en WATCH, doble clic sobre el
valor → escribir el número → `Enter` → `F5` (sigue corriendo). La pausa
dura un instante; con la placa en pausa no salen datos por la UART.

### 2. Medir

1. Revisar que `setpoint_cm` sea 0 y `modo` sea 1. Poner la pelota en el
   centro y esperar a que quede quieta.
2. Abrir el gráfico. En la terminal de VS Code (`` Ctrl+` ``):
   ```
   python grafico/grafico.py
   ```
   (o `Ctrl+Shift+P` → "Tasks: Run Task" → **Grafico**). Tiene que decir
   `Conectado a COM8` y `Guardando el log en ...\matlab\log_escalon.csv`.
   Desde ahí graba.
3. Esperar 2 o 3 segundos y cambiar `setpoint_cm` a `8` (pausa, editar,
   seguir, como arriba).
4. No tocar nada durante unos 15 segundos. Si la pelota se traba, también es
   un dato: dejarla así, el script lo usa para medir la zona muerta.
5. Cerrar la ventana del gráfico. Tiene que decir `Log guardado en ...`.
   **No volver el setpoint a 0 antes de cerrarla**: el script toma un solo
   escalón (del primer valor al último).
6. Guardar una copia, porque el gráfico pisa el archivo cada vez que se abre:
   ```
   Copy-Item matlab\log_escalon.csv matlab\log_escalon_1.csv
   ```
   (`log_escalon_2.csv` para el siguiente, y así).
7. Para dejar la viga quieta: `modo` en 0.

### 3. Correr el script

1. En la ventana de comandos de MATLAB:
   ```
   cd('C:\Users\villa\Documents\Facultad 2026\Sistemas Electronicos de instrumentacion\laboratorio-4-control-PID\matlab')
   identificar_lazo_balancin
   ```
   (o abrir el archivo y apretar **Run** / `F5`). Para analizar una copia
   vieja, cambiar en la línea 29 `archivo = 'log_escalon.csv';` por el
   nombre de la copia.
2. Tiene que decir `Datos REALES: log_escalon.csv`. Si dice `SINTETICOS`, no
   encontró el archivo: revisar que MATLAB esté parado en `matlab/`.
3. Anotar lo que imprime:
   - **K**: cuánto acelera la pelota por grado de servo.
   - **zona muerta (zm)**: grados de servo que no mueven la pelota.
   - **RMS** de los ajustes (a) y (b): si el (b) es mucho menor, la fricción
     es real y pesa.
   - **Relación brazo-viga**: grados de viga por grado de servo. Se puede
     comparar midiendo la inclinación de la viga con el celular.
   - **Tabla de ganancias**: para cada Kp y Kd, sobrepico, error final y
     oscilación pico a pico. Sirven las que tienen oscilación menor a 2 cm.

### 4. Decidir y volver a la maqueta

- **Zona muerta grande** (cerca de los 19 grados que se ven a ojo): ninguna
  ganancia lo arregla. Hay que mejorar la mecánica (varilla en otro agujero
  del brazo, revisar topes y que la viga no tenga pozos) y repetir desde la
  sección 2.
- **Zona muerta chica**: elegir de la tabla una combinación sin oscilación y
  con poco sobrepico.
  1. Cambiar `pid_bola.kp` y `pid_bola.kd` en WATCH (abrir `pid_bola` con la
     flechita) y repetir la sección 2.
  2. En el script, poner los mismos valores en `P.Kp` y `P.Kd` (línea 18) y
     correrlo: el gráfico "Log contra modelo ajustado" tiene que parecerse al
     log. Si se parece, el modelo sirve.
  3. Recién ahí probar subir `pid_bola.ki` de a poco (0.1, después 0.2) para
     sacar el error final.
  4. Dejar grabadas las ganancias finales: en `firmware/app/main_balancin.c`,
     cambiar los números de `PID_Init(&pid_bola, 1.5f, 0.0f, 0.8f, ...)` y,
     si se quiere, `banda_cm = 1.0f`. Después `F5` en Run and Debug para
     compilar y grabar.
