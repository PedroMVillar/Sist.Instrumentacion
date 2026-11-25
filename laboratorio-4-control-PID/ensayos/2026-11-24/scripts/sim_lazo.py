# Simulacion del lazo cerrado, copiando el firmware: sensor con ruido -> mediana de 5 ->
# PID de pid.c (derivada sobre la medida, anti-windup) -> servo a 60 grados/s -> modelo de la pelota.
import json, numpy as np

TS, DT, LIM, ALFA_VEL = 0.05, 0.01, 35.0, 0.7

class PID:
    def __init__(s, kp, ki, kd):
        s.kp, s.ki, s.kd = kp, ki, kd; s.integral = 0.0; s.prev = None; s.vel = 0.0
    def calc(s, sp, x):
        e = sp - x
        if s.prev is None: s.prev = x
        v = (x - s.prev) / TS; s.prev = x
        s.vel = ALFA_VEL * s.vel + (1 - ALFA_VEL) * v
        d = -s.kd * s.vel; p = s.kp * e
        out = p + s.integral + d
        if not ((out >= LIM and e > 0) or (out <= -LIM and e < 0)):
            s.integral += s.ki * e * TS
        s.integral = min(max(s.integral, -LIM), LIM)
        return min(max(p + s.integral + d, -LIM), LIM)

def simular(m, kp, ki, kd, setpoints, x0=0.0, vel_servo=60.0, ruido=0.45, semilla=0, guardar=False):
    """setpoints: lista de (t_inicio, valor). Devuelve tiempos, posicion real, medida, salida, servo."""
    rng = np.random.default_rng(semilla)
    K, ang0, b, Fs, Fc, ret = m["K"], m["ang0"], m["b"], m["Fs"], m["Fc"], m["retardo_muestras"]
    t_fin = setpoints[-1][0] if len(setpoints) > 1 and setpoints[-1][1] is None else None
    T = setpoints[-1][0]; sps = setpoints[:-1]
    pid = PID(kp, ki, kd)
    x, v = x0, 0.0
    lect = [x0] * 5; salida = 0.0; servo = 0.0
    hist_servo = [0.0] * (int(ret * TS / DT) + 1)
    reg = []
    n = int(T / DT)
    sp = sps[0][1]
    for i in range(n):
        t = i * DT
        for (ti, val) in sps:
            if t >= ti: sp = val
        if i % int(TS / DT) == 0:
            lectura = x + rng.normal(0, ruido)
            if rng.random() < 0.03: lectura += rng.choice([-1, 1]) * rng.uniform(3, 8)  # picos del HC-SR04
            lect = lect[1:] + [lectura]
            med = float(np.median(lect))
            salida = pid.calc(sp, med)
            reg.append((t, x, med, sp, salida, servo))
        if i % 2 == 0:  # servo cada 20 ms
            paso = vel_servo * 0.02
            servo += min(max(salida - servo, -paso), paso)
        hist_servo = hist_servo[1:] + [servo]
        th = hist_servo[0]
        f = K * (th - ang0)
        if abs(v) < 0.5 and abs(f) < Fs:
            v = 0.0
        else:
            sv = np.sign(v) if abs(v) >= 0.5 else np.sign(f)
            v += (f - b * v - Fc * sv) * DT
        x += v * DT
        if x > 23: x, v = 23.0, 0.0      # topes de la viga
        if x < -23: x, v = -23.0, 0.0
    return np.array(reg)

def costo(reg, tramos):
    """tramos: lista de (t_desde, t_hasta) donde se mide el error ya asentado."""
    t, x, med, sp, sal, srv = reg.T
    e = x - sp
    iae = np.mean(np.abs(e))                                 # error medio en todo el ensayo
    asent = np.concatenate([np.abs(e[(t >= a) & (t < b)]) for a, b in tramos])
    dentro1 = np.mean(asent <= 1.0)
    golpes = np.mean(np.abs(np.diff(sal)) > 5)               # saltos bruscos del servo
    return iae + 3 * np.mean(asent) + 2 * (1 - dentro1) + 5 * golpes, dict(
        iae=iae, err_asentado=np.mean(asent), dentro1=dentro1, golpes=golpes)
