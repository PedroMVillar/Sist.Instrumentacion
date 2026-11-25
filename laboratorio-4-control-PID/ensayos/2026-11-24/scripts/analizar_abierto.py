# Para cada ensayo en lazo abierto: cuando arranca la pelota y con que aceleracion.
import glob, numpy as np
for arch in sorted(glob.glob("ident/abierto_*.csv")):
    d = np.genfromtxt(arch, delimiter=",", names=True)
    t = (d["tick_ms"] - d["tick_ms"][0]) / 1000
    p = d["posicion_cm"]; ang = d["angulo_servo"]
    mov = np.abs(p - p[0]) > 1.0
    if not mov.any():
        print(f"{arch[15:-4]:>6}: no se movio en {t[-1]:.1f} s (pos {p[0]:.1f}, angulo final {ang[-1]:.1f})")
        continue
    i0 = max(np.argmax(mov) - 3, 0)
    tt, pp = t[i0:], p[i0:]
    c = np.polyfit(tt - tt[0], pp, 2)
    print(f"{arch[15:-4]:>6}: arranca a los {t[np.argmax(mov)]:.2f} s, aceleracion {2*c[0]:+6.1f} cm/s2, "
          f"angulo servo {ang[-1]:+5.1f}, {len(tt)} muestras")
