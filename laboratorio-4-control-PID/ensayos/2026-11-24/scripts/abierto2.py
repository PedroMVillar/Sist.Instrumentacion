# Ensayo en lazo abierto: centra la pelota con el PID, fija el servo en un pulso y graba
# como rueda. Corta antes de las puntas y vuelve a centrar.
# Uso: python abierto.py carpeta d1,d2,...   (d = microsegundos respecto de la horizontal)
import sys, os, time
from lab import *

carpeta = sys.argv[1]
deltas = [int(x) for x in sys.argv[2].split(",")]
inicio = float(sys.argv[3]) if len(sys.argv) > 3 else 0.0
os.makedirs(carpeta, exist_ok=True)
p = Placa()

def centrar(max_s=25):
    p.escribir_i(SERVO_CENTRO, HORIZONTAL_US)
    p.escribir_f(SETPOINT, inicio)
    p.escribir_i(MODO, 1)
    t0 = time.time(); ok = 0; prev = None
    while time.time() - t0 < max_s:
        r = p.leer(); pos = r[3]
        if prev is not None and abs(pos - inicio) < 2.5 and abs(pos - prev) < 0.2:
            ok += 1
        else:
            ok = 0
        prev = pos
        if ok >= 20:            # ~1 s quieta y cerca del centro
            return True
        time.sleep(0.05)
    return False

try:
    for d in deltas:
        if not centrar():
            print(f"{d:+d} us: no se centro (pos={p.leer()[3]:.1f}), sigo igual", flush=True)
        # lazo abierto: modo 0 deja el angulo en 0, asi el pulso queda en la horizontal + d
        p.escribir_i(MODO, 0)
        time.sleep(0.3)           # el servo vuelve a 0 grados (60 grados/s)
        filas = []; ult = None
        p.escribir_i(SERVO_CENTRO, HORIZONTAL_US + d)
        t0 = time.time()
        while time.time() - t0 < 4.0:
            r = p.leer()
            if r[8] != ult:
                ult = r[8]; filas.append(r)
            if r[3] > 12 or r[3] < -18:
                break
        p.escribir_i(SERVO_CENTRO, HORIZONTAL_US)
        p.escribir_i(MODO, 1)
        arch = os.path.join(carpeta, f"abierto_desde{inicio:+.0f}_{d:+04d}us.csv")
        with open(arch, "w") as f:
            f.write("tick_ms,setpoint,crudo_cm,posicion_cm,filtrada_cm,salida_grados,servo_us,angulo_servo,lecturas\n")
            for r in filas:
                f.write(",".join(f"{v:.3f}" if isinstance(v, float) else str(v) for v in r) + "\n")
        mov = filas[-1][3] - filas[0][3]
        print(f"{d:+d} us: {len(filas)} muestras, pos {filas[0][3]:.1f} -> {filas[-1][3]:.1f} ({mov:+.1f} cm)", flush=True)
        time.sleep(0.5)
finally:
    p.escribir_i(SERVO_CENTRO, HORIZONTAL_US)
    p.escribir_i(MODO, 1)
    p.cerrar()
