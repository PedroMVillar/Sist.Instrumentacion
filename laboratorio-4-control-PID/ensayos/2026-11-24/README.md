# Ensayos en la maqueta: 24 de noviembre de 2026

Todo lo que se midió ese día, en orden, con los datos y las figuras de cada paso.
Las muestras se tomaron por el ST-LINK (el adaptador USB-UART no estaba conectado),
una cada 50 ms, con los scripts de `scripts/`.

## Formato de los CSV

- Del 01 al 10 y `matlab/log_escalon_usado.csv`: sin encabezado,
  `tiempo_ms, posicion_cm, setpoint_cm, salida_grados` (el mismo formato que manda la UART
  con `LOG_EXTENDIDO = 1`).
- Del 11 al 16 y `modelo/lazo_abierto/`: con encabezado; agregan la lectura cruda del sensor, la filtrada, el pulso
  del servo y el ángulo real del servo.

## 1. La horizontal estaba mal

| Archivo | Qué se hizo | Qué se vio |
|---|---|---|
| `01_traer_pelota_1700us` | Servo fijo en 1700 µs y después en 1315 µs | Con 1315 µs (la "horizontal" del 20/11) la pelota rodaba sola de punta a punta en 2 s |
| `02_sostener_horizontal_1315` | PID con la horizontal vieja (Kp 2.5, Ki 0.3, Kd 0.8) | La pelota quedó trabada en +3 cm (36 cm del sensor) con el servo al tope de -35° |
| (barrido con `umbral.py`) | Servo fijo, subiendo y bajando el pulso | Desde la punta del sensor, la pelota recién se aleja con 1350 µs o menos. Desde el medio, recién va hacia el sensor con 1700 µs o más |

Conclusión: entre 1350 y 1700 µs la pelota no se mueve (unos 31° de servo). Con la horizontal
en 1315 µs, el tope de +35° hacia el sensor era 1703 µs, justo el umbral: por eso se trababa
siempre a 37 cm con el servo al tope. Se cambió la horizontal a **1525 µs**, el medio de esa zona.

La punta lejana (unos 50 cm) es una trampa: si la pelota llega ahí, no sale ni con el servo al máximo.

## 2. Ensayo de escalón y MATLAB

| Archivo | Ganancias | Qué se vio |
|---|---|---|
| `03_sostener_horizontal_1525` | Kp 2.5, Ki 0.3, Kd 0.8 | Con la horizontal nueva la pelota queda cerca del centro |
| `04_escalon_kp1.5_sin_respuesta` | Kp 1.5, Ki 0, Kd 0.8 | Escalón de 8 cm: no se movió en 17 s. La salida (10°) no sale de la zona muerta |
| `05_escalon_kp3_kd0.8` | Kp 3, Ki 0, Kd 0.8 | Escalón 0 a 8 cm: se pasa hasta 15 cm y queda en 7.9 cm a los 6 s. **Es el que usó MATLAB** |

`matlab/`: `identificar_lazo_balancin.m` corrido con `log_escalon_usado.csv` (el 05 recortado:
arranca en 0 y el escalón queda de 6 cm) y `P.Kp = 3`.

- K = 0.45 cm/s² por grado de servo. La primera subida del modelo coincide con la real (`fig2_ajuste_modelo.png`).
- Zona muerta identificada: 0, también arrancando la búsqueda desde 10° (`fig2b_ajuste_zm_inicial10.png`).
  El modelo pone la zona muerta en el ángulo del servo siempre; la maqueta tiene roce estático,
  que frena a la pelota solo cuando está quieta. Por eso el modelo sigue oscilando donde la
  pelota real se clava.
- Chen no se pudo aplicar: el sobrepico fue de más del 100 % y da un amortiguamiento negativo.
- La tabla de ganancias (`fig3_prueba_ganancias.png`) indica subir Kd para bajar el sobrepico.

Salida de MATLAB:

```
Ajuste (a) solo K          : K = 0.45            RMS = 2.24 cm
Ajuste (b) K + zona muerta : K = 0.45, zm = 0.0   RMS = 2.24 cm
Kp = 1.0: [Kd 0.4:   80% /   1.9 /   4.2]  [Kd 0.8:   51% /   0.3 /   0.7]  [Kd 1.5:   18% /  -0.0 /   0.0]
Kp = 1.5: [Kd 0.4:   90% /  -1.1 /   9.1]  [Kd 0.8:   63% /  -0.3 /   1.6]  [Kd 1.5:   30% /  -0.0 /   0.1]
Kp = 2.5: [Kd 0.4:  127% /   6.3 /  13.7]  [Kd 0.8:   81% /   1.3 /   2.9]  [Kd 1.5:   49% /   0.0 /   0.2]
(sobrepico / error final [cm] / oscilación pico a pico [cm])
```

## 3. Ajuste en la maqueta

| Archivo | Cambio | Qué se vio |
|---|---|---|
| `06_escalon_kp3_kd1.5` | Kd 0.8 a 1.5 | Escalón 8 a 0: se pasa solo a -1.1 cm y en 4 s queda a 1 cm |
| `07_escalones_ki0.2_banda1` | Ki 0.2, banda 1 cm | Llega a -6 y vuelve a 0, pero queda en +1.1 cm |
| `08_centro_27.3` | Centro de 33 a 27.3 cm | La pelota tocó 3.7 cm contra el sensor y 50.8 cm en la otra punta: el medio real es 27.3. Con 33 la "centrada" quedaba 5.5 cm corrida |
| `09_kd0.8_servo150` | Kd 0.8 | Peor: oscila entre -15 y +23 cm |
| `10_kd1.5_servo60` | Servo de 150 a 60 °/s | El ruido del sensor hacía que el servo diera golpecitos; más lento los suaviza. 87 % del tiempo dentro de 2 cm |
| `11_crudo_vs_filtrado` | Se grabó el sensor crudo y el filtrado | La mediana baja el salto entre lecturas de 0.48 a 0.19 cm y los picos de 19 a 2, con solo 50 ms de retardo. El servo sigue al PID con 50 ms de retardo. El problema no es el filtro: con la pelota quieta en +2.5 cm el servo pidió -15° durante 8 s y no se movió (roce estático) |

## 4. Compensación del roce (empujón)

Regla agregada en `main_balancin.c`: si la pelota está trabada fuera de la banda, se suman
12° a la salida hacia el lado del setpoint.

| Archivo | Versión | Qué se vio |
|---|---|---|
| `12_empujon12_sin_espera` | Empuja apenas la pelota está quieta | Oscila sin parar (±8 cm): empujaba también en cada punto de vuelta, donde la pelota frena un instante |
| `13_empujon12_espera300ms_kp3` | Empuja solo si lleva 300 ms quieta, Kp 3 | Mejor, pero sigue oscilando ±5 cm después de los escalones |
| `14a/b/c_config_*` | Kp 2 y tres variantes | **A (Kp 2, Ki 0.1, Kd 1.5, servo 60) fue la única que quedó 100 % del tiempo dentro de 1 cm** en los dos setpoints, en una sola corrida. No se repitió en el ensayo largo (15). B (Kd 2.5) y C (servo 100) oscilaban más |
| `15_final_A_ensayo_largo` | A, escalones a -6, 0, +5, 0 | 20 s quieta en 0 dentro de 1 cm. Los escalones se pasan hasta 8 cm y del lado lejano (+5) oscila más. Dentro de 1 cm en los últimos segundos de cada tramo: 25 % en -6, 50 % en 0, 14 % en +5 y 7 % en 0 |
| `16_final_A_quieta_40s` | A, quieta en 0 | Casi todo el tiempo dentro de 1 cm, con salidas chicas de hasta 1.6 cm |

El empujón y la banda se sacaron después del firmware para dejar un PID puro.

## 5. Modelo físico (`modelo/`)

18 corridas en lazo abierto (`lazo_abierto/`, figura `abierto_todos.png`) más 6 registros en lazo cerrado.

- Primer intento (`ident_modelo.py`): derivar dos veces la posición. R² = 0.09, descartado por ruido.
- Segundo intento (`ident_sim.py`, `modelos.json`): ajuste por simulación en ventanas de 1 s. La validación
  mezclaba ventanas solapadas del mismo ensayo y M3 quedó en un mínimo local.
- Versión verificada (`ident_check.py`, `modelos_verificados.json`): validación dejando ensayos enteros
  afuera, velocidad inicial calculada solo con datos anteriores y varios arranques. **Es la que vale.**

| Modelo | K | ang0 | b | Retardo | Error RMS en 1 s |
|---|---|---|---|---|---|
| Referencia: suponer la pelota quieta | | | | | 2.01 cm |
| M1: a = K(ang - ang0) | 0.78 | -5.3° | 0 | 100 ms | 2.73 cm |
| M2: M1 - b v | 1.27 | -5.7° | 5.2 | 150 ms | 1.82 cm |
| M3: M2 + roce estático (Fs 21.8, Fc 0.3) | 1.75 | -3.8° | 7.1 | 150 ms | 1.80 cm |

- El modelo predice poco más que suponer la pelota quieta. En las ventanas con movimiento (error del
  ajuste): 2.50 (M2) contra 2.95 cm.
- K y b cambian según los datos (M2: K 1.1 a 1.7, b 4.3 a 7.8). ang0 sale negativo en los tres.
- ang0 equivale a una horizontal entre 1567 y 1588 µs. El 1592 salió del ajuste anterior (-6.0°).
- En las corridas largas la pelota tarda 0.35 a 0.43 s en pasar por cada cm de +2/+3 y cerca de 0.1 s en el
  resto. Puede ser un bache; algunas corridas arrancaban ahí, así que no es seguro.

### Opciones (solo simulación, `check2.py`)

Dentro de 1 cm al final de cada tramo, con los modelos verificados:

| Opción | Horizontal | Kp / Ki / Kd | M2 | M3 |
|---|---|---|---|---|
| **A** (en el firmware) | 1592 | 4 / 0.1 / 0.5 | 100 % | 100 % |
| B | 1525 | 4 / 0.8 / 0.5 | 100 % | 62 % |
| Anterior | 1525 | 3 / 0.2 / 1.5 | 74 % | 51 % |
| 14a sin el empujón | 1525 | 2 / 0.1 / 1.5 | 32 % | 68 % |

La búsqueda probó 150 combinaciones con el M2 del segundo intento (`busqueda_*.json`). Kd 0.5 es el mínimo
probado, así que el óptimo puede estar más abajo. **A y B no se probaron en la maqueta.**

## Scripts

Las direcciones de memoria de los scripts son las del firmware de cada momento (cambian al
recompilar). Se sacan con `arm-none-eabi-nm firmware/build/Debug/firmware.elf`.

- `ensayo_stlink.py`: graba posición, setpoint y salida por el ST-LINK y cambia variables a tiempos dados.
- `crudo.py`, `crudo2.py`, `crudo3.py`: lo mismo, más la lectura cruda, la filtrada y el servo real.
- `analizar_crudo.py`: ruido, retardos y gráfico crudo contra filtrado.
- `umbral.py`, `horizontal.py`, `sostener.py`: búsqueda de la horizontal.
- `barrido_config.py`: compara configuraciones con el mismo ensayo.
- `graficar_log.py`: el gráfico de cada CSV.
- `lab.py`, `abierto.py`, `abierto2.py`, `analizar_abierto.py`: ensayos en lazo abierto.
- `ident_modelo.py`, `ident_sim.py`, `ident_check.py`: los tres ajustes (se corren en una carpeta con los CSV en `ident/`).
- `sim_lazo.py`, `buscar_ganancias.py`, `check2.py`: simulación del lazo y comparación de ganancias.
