# Prueba varias configuraciones con el mismo ensayo (escalon a -6 y vuelta a 0) y las compara.
import subprocess, sys
import numpy as np
KP, KI, KD, VEL, EMP, SP = "200002d8", "200002dc", "200002e0", "20000024", "20000028", "200002b4"
configs = {
    "A_kp2_kd1.5_v60":  {KP: 2.0, KI: 0.1, KD: 1.5, VEL: 60, EMP: 12},
    "B_kp2_kd2.5_v60":  {KP: 2.0, KI: 0.1, KD: 2.5, VEL: 60, EMP: 12},
    "C_kp2_kd1.5_v100": {KP: 2.0, KI: 0.1, KD: 1.5, VEL: 100, EMP: 12},
}
sel = sys.argv[1:] or list(configs)
for nombre in sel:
    acc = [f"0.1:{a}={v}" for a, v in configs[nombre].items()]
    acc += [f"4:{SP}=-6", f"16:{SP}=0"]
    subprocess.run([sys.executable, "crudo3.py", f"cfg_{nombre}.csv", "28"] + acc, capture_output=True)
    d = np.genfromtxt(f"cfg_{nombre}.csv", delimiter=",", names=True)
    t = (d["tick_ms"] - d["tick_ms"][0]) / 1000
    e = d["posicion_cm"] - d["setpoint"]
    r = []
    for a, b in ((10, 16), (22, 28)):
        m = (t >= a) & (t < b)
        r.append((np.mean(np.abs(e[m])), np.ptp(d["posicion_cm"][m]), np.mean(np.abs(e[m]) <= 1) * 100))
    m1 = (t >= 4) & (t < 16)
    sobre = -6 - d["posicion_cm"][m1].min()
    print(f"{nombre}: sobrepico {sobre:.1f} cm | en -6 (6 s finales): error medio {r[0][0]:.1f} cm, pico a pico {r[0][1]:.1f}, dentro 1cm {r[0][2]:.0f}% "
          f"| en 0: error medio {r[1][0]:.1f}, pico a pico {r[1][1]:.1f}, dentro 1cm {r[1][2]:.0f}%", flush=True)
