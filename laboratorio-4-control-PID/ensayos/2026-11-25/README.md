# Ensayos del 25 de noviembre de 2026: ajuste final en la maqueta

Los CSV se leyeron de la RAM de la placa: `tick_ms,setpoint,crudo_cm,posicion_cm,filtrada_cm,salida_grados,servo_us,angulo_servo,lecturas`.
Las corridas se lanzaron con scripts que escriben las variables por OpenOCD (puerto 6666).
El nombre de cada archivo lleva los valores usados, por ejemplo `e1_1.5_0.05_2_0.85_1560_10_12`
= Kp, Ki, Kd, `alfa_vel`, horizontal, rampa del empuje (°/s) y tope del empuje (°).

## 1. Opción A del modelo (`A_*.csv`)

Horizontal 1592, Kp 4 / Ki 0,1 / Kd 0,5. **Falló**: la pelota iba de punta a punta (±24 cm).

## 2. Barrido rápido (`b1_*`, `b2_*`, `r1_*`)

PID puro con el servo a 150 y 300 °/s y ±35°. Los movimientos fueron tan bruscos que hubo que
desenchufar la maqueta. **Desde acá todo se probó con ±20°,
60 °/s, corte automático si la pelota se va a más de 15 a 17 cm y `modo` 0 al terminar.**

## 3. PID puro con límites seguros (`s1`–`s3`, `k1_*`, `k2r*`)

| Resultado | Detalle |
|---|---|
| Mejor: 1,5 / 0,2 / 1,5 | Sostiene el centro, 49 a 65 % del tiempo dentro de 1 cm |
| Problema | En los escalones se traba cerca de +3,7 cm, con cualquier ganancia |
| Ki 0,4 | La integral se carga mientras la pelota está trabada y al soltarse la dispara |

## 4. Banda de 1 cm y empuje en rampa (`e1`–`e5`, `f1`, `f2`)

Se agregó al firmware: dentro de 1 cm del setpoint el PID ve error cero; si la pelota está quieta
fuera de la banda, un empuje crece de a poco hacia el setpoint hasta despegarla.

| Corridas | Valores | Resultado |
|---|---|---|
| `e1`–`e5` | 1,5 / 0,05 / 2, horizontal 1560, empuje 10 °/s hasta 12° | Centra y sigue el escalón a +5, pero en 3 de 5 se traba cerca de +3 a la vuelta |
| `f*_1580_10_12` | Igual con horizontal 1580 | 97 a 100 % dentro de 1 cm en +5 y a la vuelta; 14 a 63 % en el primer tramo |
| `f*_1580_10_15`, `f*_1575_15_15` | Tope 15° | Parecido; la elección fue pareja |

**Elegido y grabado en el firmware:** Kp 1,5, Ki 0,05, Kd 2, `alfa_vel` 0,85, horizontal 1580,
banda 1 cm, empuje 10 °/s hasta 12°, ±20°, 60 °/s.

## 5. Comprobación con los valores de la flash (`fin_a`–`fin_c`)

Tres corridas: 27 a 61 % dentro de 1 cm en 0, 47 a 55 % en +5 y 32 a 46 % a la vuelta.
El sensor tuvo tres veces más ruido que en las corridas de selección.

## 6. Distintos setpoints (`sp1.csv`)

Setpoints 0, −4, −8, −4, 0, +4, +8, +4, 0, 14 s cada uno. Se miden los últimos 7 s de cada tramo.

| Setpoint | Error medio (cm) | Dentro de 1 cm | Dentro de 2 cm |
|---|---|---|---|
| 0 | 0,9 | 100 % | 100 % |
| −4 | 1,5 | 30 % | 78 % |
| −8 | 1,6 | 26 % | 69 % |
| −4 | 1,4 | 43 % | 80 % |
| 0 | 2,0 | 24 % | 52 % |
| +4 | 0,7 | 100 % | 100 % |
| +8 | 0,2 | 100 % | 100 % |
| +4 | 0,7 | 91 % | 100 % |
| 0 | 1,4 | 22 % | 89 % |

En el primer tramo la pelota ya estaba quieta a 0,9 cm, dentro de la banda, así que ese 100 % no dice mucho.
La pelota estuvo siempre entre −11,8 y +8,6 cm. Una segunda pasada se cortó porque se perdió la
conexión con el ST-LINK y no dejó datos.

**Conclusión:** controla y no se va a las puntas. Del lado lejano (+4, +8) queda quieta; en 0 y del
lado del sensor oscila 1 a 2 cm. La causa no está probada: puede ser el roce, la horizontal, el
sensor o la viga despareja. Desde las puntas de la viga no se probó.

## Scripts

- `seguro.py`: una corrida con corte automático; lee las direcciones de las variables del `.elf`.
- `serie.py`, `serie2.py`: series de corridas cambiando valores.
- `final.py`: comprobación con los valores de la flash.
- `setpoints.py`: la pasada por varios setpoints.
- `ver.py`: resumen de un CSV.
- `barrido25.py`: el barrido rápido de la sección 2. Tiene direcciones de memoria fijas de un firmware viejo: **no correrlo** con el firmware actual.
