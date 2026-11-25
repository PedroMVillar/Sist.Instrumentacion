import sys, csv
import numpy as np
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

arch = sys.argv[1]; png = sys.argv[2]
d = np.genfromtxt(arch, delimiter=",", names=True)
t = (d["tick_ms"] - d["tick_ms"][0]) / 1000
centro = d["filtrada_cm"][0] - d["posicion_cm"][0]
crudo = d["crudo_cm"] - centro
filt = d["filtrada_cm"] - centro
sp, sal, ang = d["setpoint"], d["salida_grados"], d["angulo_servo"]

dt = np.median(np.diff(t))
print(f"muestras {len(t)}, periodo medio {dt*1000:.0f} ms")

# ruido: diferencia entre lecturas seguidas
dc = np.abs(np.diff(crudo)); df = np.abs(np.diff(filt))
print(f"salto medio entre lecturas: crudo {dc.mean():.2f} cm, filtrada {df.mean():.2f} cm")
print(f"saltos mayores a 3 cm: crudo {np.sum(dc > 3)}, filtrada {np.sum(df > 3)}")

def retardo(a, b, maxlag=20):
    """muestras que b llega tarde respecto de a (correlacion de las derivadas)"""
    da, db = np.diff(a), np.diff(b)
    mejores = []
    for k in range(0, maxlag):
        x, y = da[:len(da) - k], db[k:]
        mejores.append(np.corrcoef(x, y)[0, 1])
    k = int(np.nanargmax(mejores))
    return k, mejores[k]

k1, c1 = retardo(crudo, filt)
k2, c2 = retardo(sal, ang)
print(f"retardo cruda -> filtrada: {k1} muestras = {k1*dt*1000:.0f} ms (corr {c1:.2f})")
print(f"retardo salida PID -> servo: {k2} muestras = {k2*dt*1000:.0f} ms (corr {c2:.2f})")

# cuanto tiempo el servo no llega a lo que pide el PID
dif = np.abs(sal - ang)
print(f"servo atrasado mas de 5 grados: {np.mean(dif > 5)*100:.0f}% del tiempo; diferencia media {dif.mean():.1f} grados")

fig, ax = plt.subplots(2, 1, figsize=(10, 7), sharex=True)
ax[0].plot(t, crudo, ".", ms=3, color="0.6", label="sensor crudo")
ax[0].plot(t, filt, color="tab:blue", lw=1.6, label="filtrada (mediana de 5)")
ax[0].step(t, sp, "--", color="tab:red", where="post", label="setpoint")
ax[0].set_ylabel("posicion [cm]"); ax[0].legend(loc="best"); ax[0].grid(alpha=0.3)
ax[0].set_title("Sensor crudo contra filtrado")
ax[1].plot(t, sal, color="tab:orange", lw=1, label="lo que pide el PID")
ax[1].plot(t, ang, color="tab:green", lw=1.6, label="donde esta el servo (60 grados/s)")
ax[1].set_ylabel("angulo [grados]"); ax[1].set_xlabel("t [s]"); ax[1].legend(loc="best"); ax[1].grid(alpha=0.3)
ax[1].set_title("Salida del PID contra servo real")
fig.tight_layout(); fig.savefig(png, dpi=90)
