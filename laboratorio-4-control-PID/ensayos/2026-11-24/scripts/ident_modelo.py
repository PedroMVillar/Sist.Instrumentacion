# Identificacion del modelo de la pelota con todos los ensayos que tienen el angulo real del servo.
# Modelo: a = K*angulo(t - retardo) - b*v + c0 - Fc*signo(v) + d(x)
import glob, numpy as np
from scipy.signal import savgol_filter

US_POR_GRADO, HORIZ, TS = 11.1, 1525, 0.05
E = "C:/Users/villa/Documents/Facultad 2026/Sistemas Electronicos de instrumentacion/laboratorio-4-control-PID/ensayos/2026-11-24/"
archivos = sorted(glob.glob("ident/abierto_*.csv")) + [E + f for f in (
    "11_crudo_vs_filtrado.csv", "12_empujon12_sin_espera.csv", "13_empujon12_espera300ms_kp3.csv",
    "14a_config_A_kp2_kd1.5_v60.csv", "14b_config_B_kp2_kd2.5_v60.csv", "14c_config_C_kp2_kd1.5_v100.csv",
    "15_final_A_ensayo_largo.csv")]

def cargar(arch):
    d = np.genfromtxt(arch, delimiter=",", names=True)
    # angulo efectivo de la viga en grados de servo, sacado del pulso real (positivo = hacia la punta lejana)
    ang = -(d["servo_us"] - HORIZ) / US_POR_GRADO
    x = d["posicion_cm"]
    return x, ang

def preparar(retardo):
    filas = []
    for arch in archivos:
        x, ang = cargar(arch)
        if len(x) < 15: continue
        v = savgol_filter(x, 9, 2, deriv=1, delta=TS)
        a = savgol_filter(x, 9, 2, deriv=2, delta=TS)
        th = np.roll(ang, retardo); th[:retardo] = ang[0]
        m = np.ones(len(x), bool); m[:6] = m[-6:] = False
        m &= (x > -17) & (x < 12)                     # lejos de las puntas
        filas.append(np.column_stack([a[m], th[m], v[m], x[m]]))
    return np.vstack(filas)

def ajustar(D, con_coulomb, con_bache):
    a, th, v, x = D.T
    cols = [th, -v, np.ones_like(v)]; nombres = ["K", "b", "c0"]
    if con_coulomb:
        cols.append(-np.tanh(v / 1.0)); nombres.append("Fc")
    if con_bache:
        centros = np.arange(-15, 12, 2.0)
        for c in centros:
            cols.append(np.exp(-0.5 * ((x - c) / 1.2) ** 2)); nombres.append(f"d({c:+.0f})")
    A = np.column_stack(cols)
    p, *_ = np.linalg.lstsq(A, a, rcond=None)
    r = a - A @ p
    return dict(zip(nombres, p)), 1 - r.var() / a.var(), len(a)

for ret in range(0, 6):
    D = preparar(ret)
    _, r2, n = ajustar(D, False, False)
    print(f"retardo {ret*50:3d} ms: R2 lineal = {r2:.3f}  ({n} muestras)")
ret = max(range(6), key=lambda r: ajustar(preparar(r), True, True)[1])
D = preparar(ret)
print(f"\nretardo elegido: {ret*50} ms")
for cc, bb in ((False, False), (True, False), (False, True), (True, True)):
    p, r2, n = ajustar(D, cc, bb)
    txt = ", ".join(f"{k}={v:.2f}" for k, v in p.items())
    print(f"coulomb={cc!s:5} bache={bb!s:5}  R2={r2:.3f}  {txt}")
np.save("ident/datos_ident.npy", D)
