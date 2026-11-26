# Laboratorio 4 — Control PID, explicado

## El lazo de control

Hasta ahora los programas medían y mostraban. Acá por primera vez el programa **actúa sobre lo que mide**: la salida (inclinar la viga) cambia la próxima medición (la posición de la bola). Eso es un lazo cerrado de control.

```mermaid
flowchart LR
    SP[Setpoint: centro de la viga] --> E((error))
    S[HC-SR04 mide la posición] --> E
    E --> PID[PID calcula el ángulo]
    PID --> SRV[Servo inclina la viga]
    SRV --> B[La bola rueda]
    B --> S
```

Cada 50 ms el lazo hace lo mismo: mide dónde está la bola, calcula el error (`setpoint - posicion`), y con ese error decide cuántos grados inclinar la viga.

## Las tres acciones del PID

La salida es la suma de tres términos, y cada uno mira el error desde un tiempo distinto:

**P — Proporcional (el presente).** Corrige según el error de ahora: bola lejos del centro, viga muy inclinada; bola cerca, viga casi plana. Problema: la bola tiene inercia. Cuando llega al centro viene con velocidad, se pasa de largo, y el P la empuja de vuelta... oscila para siempre.

**D — Derivativo (el futuro).** Mira qué tan rápido cambia el error. Si la bola se acerca rápido al centro, el D inclina la viga **en contra** antes de que llegue: es el freno que evita que se pase de largo.

**I — Integral (el pasado).** Suma el error de todas las vueltas anteriores. Si la bola se queda quieta cerca del centro pero no exactamente (por fricción o porque la viga está desbalanceada), el error chico no alcanza para moverla con P solo. El I lo va acumulando hasta que la fuerza alcanza. Es el que elimina el error de régimen.

La ecuación de la guía, en su forma estándar:

> salida = Kp · [ e(t) + (1/Ti) ∫e·dt + Td · de/dt ]

`Ti` y `Td` son tiempos físicos (cuánto tarda el I en igualar al P, cuánto "proyecta hacia el futuro" el D). En el código usamos la forma algorítmica: **Ki = Kp/Ti** y **Kd = Kp·Td**.

## Detalles del código de `pid.c`

**Anti-windup.** El servo tiene un límite de grados. Si la bola queda lejos un rato, la integral acumula un error que el servo no puede corregir, y cuando la bola al fin cruza el centro toda esa acumulación tarda muchísimo en descargarse. Por eso, si la salida ya está al tope y el error empuja hacia ese mismo tope, la integral no acumula. Además la integral se guarda ya multiplicada por `ki` (en grados), así se puede cambiar `ki` en vivo sin que la salida pegue un salto.

**Derivada sobre la medida.** El D usa la velocidad de la bola y no la del error. Con el setpoint fijo da lo mismo, pero si el setpoint cambia de golpe (ensayo de escalón) no aparece un golpe en el servo. Esa velocidad se suaviza (85% la anterior y 15% la nueva, `alfa_vel`) porque el sensor tiene ruido de unos 0,4 cm y al dividir por Ts el ruido se agranda.

**Banda de 1 cm y empuje (en `main_balancin.c`).** El PID puro no alcanzó: la pelota se traba por el roce y el ruido del sensor hace que el servo dé golpecitos. Por eso se agregaron dos cosas. Si la pelota está a menos de 1 cm del setpoint, el PID ve error cero y deja de corregir (el D sigue frenando). Si la pelota está quieta fuera de esa banda, se suma un empuje que crece 10 grados por segundo hacia el setpoint, hasta 12 grados, hasta que se despega. Cuando rueda, el empuje se borra rápido. Por esto el Ki quedó chico (0,05): con Ki alto la integral se carga mientras la pelota está trabada y al soltarse la dispara.

**Capa de hardware.** El PID no sabe qué sensor ni qué servo hay: solo ve `LeerSensorDistancia()` y `ActualizarServo()`. Si mañana cambia el sensor, el algoritmo no se toca. Es la misma idea de librerías del lab 3.

## Por qué el lazo corre cada 50 ms (el tiempo de muestreo)

La guía pide Ts entre 20 y 50 ms, y las tres razones son límites físicos:

- **El sensor:** el eco del HC-SR04 tarda hasta unos milisegundos en volver; con un Ts muy corto el lazo correría sin dato nuevo.
- **El servo:** acepta un comando cada 20 ms (50 Hz). Correr el lazo más rápido es calcular salidas que el servo ignora.
- **El ruido del D:** con Ts muy chico, la posición casi no cambia entre muestras; dividir ese salto diminuto por un Ts diminuto hace que el término derivativo escupa picos enormes y el servo tiemble (*chattering*).

El lazo usa `HAL_GetTick()` como en los labs 2 y 3: nada bloquea, el `if` del tiempo decide cuándo toca correr.

## Cómo sintonizar en la maqueta (Ziegler-Nichols)

Todo se hace con Live Expressions (`setup.md`), cambiando `pid_bola.kp`, `.ki` y `.kd` sin recompilar:

1. **Aislar el P:** poner `ki` y `kd` en 0 (con `modo 1` y la maqueta ya verificada según `setup.md`).
2. **Buscar la oscilación crítica:** subir `kp` de a poco hasta que la bola oscile de forma sostenida, sin crecer ni amortiguarse. Ese `kp` es la **ganancia crítica Ku**.
3. **Medir el período:** medir en el gráfico (`setup.md`) una oscilación completa de la bola (ida y vuelta). Ese tiempo es el **período crítico Tu**, en segundos.
4. **Calcular:**
   - `kp = 0.6 × Ku`
   - `ki = kp / (0.5 × Tu)`
   - `kd = kp × (0.125 × Tu)`
5. Cargar los tres valores y ver cómo responde. Cuando convenza, pasarlos a los valores iniciales de `main_balancin.c` para que queden grabados.

### Ajuste fino: si pasa esto, tocá esto

| Síntoma | Causa probable | Qué tocar |
|---|---|---|
| La bola oscila sin parar | Mucho P o poco D | Bajar `kp` o subir `kd` |
| Se pasa de largo y vuelve (sobreimpulso) | Falta freno | Subir `kd` |
| Queda quieta pero fuera del centro | Falta integral | Subir `ki` de a poco |
| Tarda mucho en centrarse después de estar lejos | Windup o mucho I | Bajar `ki` (el anti-windup ya ayuda) |
| El servo tiembla con la bola quieta | Ruido del sensor en el D | Bajar `kd`, o subir Ts |
| Corrige para el lado equivocado | Signo invertido | Cambiar el signo de `sentido` |

## La idea que cierra los cuatro labs

El lab 2 enseñó a no bloquear, el lab 3 a medir con librerías, y este junta todo y le agrega la vuelta: el dato medido vuelve al hardware como acción. El super-loop no bloqueante no era un capricho: un PID necesita correr a intervalos **exactos y regulares** (el Ts está dentro de las fórmulas de I y D), y eso solo se puede garantizar si ninguna otra tarea frena el lazo.
