# Identificacion por simulacion (ventanas de 1 s). Compara modelos de simple a completo.
#   M1: a = K*(ang - ang0)                       (el del script de MATLAB, sin zona muerta)
#   M2: a = K*(ang - ang0) - b*v                 (+ roce viscoso)
#   M3: M2 + roce estatico: quieta no arranca hasta que |K*(ang-ang0)| > Fs,
#       y rodando tiene un roce constante Fc en contra
# El angulo entra con un retardo (servo + filtro del sensor).
import glob, json, numpy as np
from scipy.signal import savgol_filter
from scipy.optimize import minimize

US_POR_GRADO, HORIZ, TS = 11.1, 1525, 0.05
E = "C:/Users/villa/Documents/Facultad 2026/Sistemas Electronicos de instrumentacion/laboratorio-4-control-PID/ensayos/2026-11-24/"
archivos = sorted(glob.glob("ident/abierto_*.csv")) + [E + f for f in (
    "11_crudo_vs_filtrado.csv", "13_empujon12_espera300ms_kp3.csv",
    "14a_config_A_kp2_kd1.5_v60.csv", "14b_config_B_kp2_kd2.5_v60.csv", "14c_config_C_kp2_kd1.5_v100.csv",
    "15_final_A_ensayo_largo.csv")]
N = 20  # 1 s por ventana

ventanas = []
for i, arch in enumerate(archivos):
    d = np.genfromtxt(arch, delimiter=",", names=True)
    x = d["posicion_cm"]; ang = -(d["servo_us"] - HORIZ) / US_POR_GRADO
    if len(x) < 30: continue
    v = savgol_filter(x, 7, 2, deriv=1, delta=TS)
    for k in range(8, len(x) - N, 10):
        xs = x[k:k + N]
        if xs.max() > 12 or xs.min() < -17: continue
        ventanas.append((x[k - 8:k + N], ang[k - 8:k + N], v[k], i))
print(len(ventanas), "ventanas de 1 s")
rng = np.random.default_rng(0)
idx = rng.permutation(len(ventanas)); n_val = len(idx) // 4
val = [ventanas[j] for j in idx[:n_val]]; ent = [ventanas[j] for j in idx[n_val:]]

def simular(p, modelo, x0, v0, ang, ret):
    K, ang0, b, Fs, Fc = p
    dt = 0.01; sub = int(TS / dt)
    x, v = x0, v0; xs = [x]
    for k in range(8, len(ang) - 1):
        th = ang[k - ret] if ret > 0 else ang[k]
        for _ in range(sub):
            f = K * (th - ang0)
            if modelo == 1:
                acc = f
            elif modelo == 2:
                acc = f - b * v
            else:
                if abs(v) < 0.5 and abs(f) < Fs:
                    v = 0.0; acc = 0.0
                else:
                    sv = np.sign(v) if abs(v) >= 0.5 else np.sign(f)
                    acc = f - b * v - Fc * sv
            v += acc * dt; x += v * dt
        xs.append(x)
    return np.array(xs)

def error(p, modelo, ret, conj):
    e = []
    for xw, aw, v0, _ in conj:
        xm = xw[8:]
        xs = simular(p, modelo, xm[0], v0, aw, ret)
        e.append(np.mean((xs - xm) ** 2))
    return float(np.mean(e))

def ajustar(modelo, ret):
    libres = {1: [0, 1], 2: [0, 1, 2], 3: [0, 1, 2, 3, 4]}[modelo]
    base = np.array([0.5, 0.0, 0.5, 3.0, 1.0])
    if modelo < 3: base[3] = base[4] = 0
    if modelo < 2: base[2] = 0
    def f(q):
        p = base.copy(); p[libres] = q
        if p[2] < 0 or p[3] < 0 or p[4] < 0 or p[0] <= 0: return 1e6
        return error(p, modelo, ret, ent)
    r = minimize(f, base[libres], method="Nelder-Mead", options={"maxiter": 600, "xatol": 1e-3, "fatol": 1e-4})
    p = base.copy(); p[libres] = r.x
    return p

resultados = {}
for modelo in (1, 2, 3):
    mejor = None
    for ret in (0, 1, 2, 3):
        p = ajustar(modelo, ret)
        e_val = error(p, modelo, ret, val)
        if mejor is None or e_val < mejor[2]:
            mejor = (p, ret, e_val)
    p, ret, e_val = mejor
    print(f"M{modelo}: retardo {ret*50} ms, K={p[0]:.2f} cm/s2/grado, ang0={p[1]:+.2f} grados, b={p[2]:.2f} 1/s, "
          f"Fs={p[3]:.2f}, Fc={p[4]:.2f} cm/s2 | error RMS en 1 s (validacion) = {np.sqrt(e_val):.2f} cm", flush=True)
    resultados[f"M{modelo}"] = dict(K=p[0], ang0=p[1], b=p[2], Fs=p[3], Fc=p[4], retardo_muestras=ret,
                                    rms_val_cm=float(np.sqrt(e_val)))
json.dump(resultados, open("ident/modelos.json", "w"), indent=1)
