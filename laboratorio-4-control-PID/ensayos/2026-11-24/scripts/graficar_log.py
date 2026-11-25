# Grafica un CSV de ensayo (formato "tick_ms,posicion,setpoint,salida", sin encabezado)
# o uno con encabezado de crudo.py. Uso: python graficar_log.py archivo.csv salida.png "titulo"
import sys
import numpy as np
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

arch, png, titulo = sys.argv[1], sys.argv[2], sys.argv[3]
with open(arch) as f:
    primera = f.readline()
if primera.startswith("tick_ms"):
    d = np.genfromtxt(arch, delimiter=",", names=True)
    tick, pos, sp, sal = d["tick_ms"], d["posicion_cm"], d["setpoint"], d["salida_grados"]
else:
    d = np.loadtxt(arch, delimiter=",")
    tick, pos, sp, sal = d[:, 0], d[:, 1], d[:, 2], d[:, 3]
t = (tick - tick[0]) / 1000

fig, ax = plt.subplots(2, 1, figsize=(10, 6), sharex=True)
ax[0].plot(t, pos, color="tab:blue", lw=1.5, label="posicion")
ax[0].step(t, sp, "--", color="tab:red", where="post", label="setpoint")
ax[0].axhspan(-1, 1, color="tab:green", alpha=0.08, label="+/- 1 cm")
ax[0].set_ylabel("posicion [cm]"); ax[0].legend(loc="best"); ax[0].grid(alpha=0.3)
ax[0].set_title(titulo)
ax[1].plot(t, sal, color="tab:orange", lw=1)
ax[1].set_ylabel("salida PID [grados]"); ax[1].set_xlabel("t [s]"); ax[1].grid(alpha=0.3)
fig.tight_layout(); fig.savefig(png, dpi=90)
