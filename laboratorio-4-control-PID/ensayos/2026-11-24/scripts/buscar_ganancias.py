# Busca Kp, Ki, Kd con el modelo identificado. Ensayo: arranca en -6 con setpoint 0 (20 s),
# escalon a +5 (15 s) y vuelta a 0 (15 s). Se prueba con el modelo nominal y con K, b y ang0 corridos
# (robustez: el modelo no es exacto).
import sys, json, itertools, numpy as np
from sim_lazo import simular, costo

nombre = sys.argv[1] if len(sys.argv) > 1 else "M2"
m0 = json.load(open("ident/modelos.json"))[nombre] if nombre != "M2_manual" else None
sps = [(0, 0.0), (20, 5.0), (35, 0.0), (50, None)]
tramos = [(12, 20), (28, 35), (43, 50)]
variantes = []
for fk, dang, fb in ((1, 0, 1), (0.8, 1.0, 1), (1.2, -1.0, 1), (1, 0, 0.7), (1, 0, 1.3)):
    m = dict(m0); m["K"] *= fk; m["ang0"] += dang; m["b"] *= fb
    variantes.append(m)

res = []
for kp, ki, kd in itertools.product([1, 1.5, 2, 3, 4, 6], [0, 0.1, 0.2, 0.4, 0.8], [0.5, 1, 1.5, 2.5, 4]):
    cs, ds = [], []
    for j, m in enumerate(variantes):
        reg = simular(m, kp, ki, kd, sps, x0=-6.0, semilla=j)
        c, d = costo(reg, tramos); cs.append(c); ds.append(d)
    res.append((np.mean(cs), kp, ki, kd, {k: np.mean([d[k] for d in ds]) for k in ds[0]}))
res.sort(key=lambda r: r[0])
print(f"modelo {nombre}: mejores 10")
for c, kp, ki, kd, d in res[:10]:
    print(f"Kp {kp:<4} Ki {ki:<4} Kd {kd:<4} costo {c:5.2f} | error medio {d['iae']:.2f} cm, "
          f"asentado {d['err_asentado']:.2f} cm, dentro 1 cm {d['dentro1']*100:3.0f}%, golpes {d['golpes']*100:.0f}%")
for kp, ki, kd in ((3, 0.2, 1.5), (2, 0.1, 1.5), (1.5, 0.2, 0.05)):
    r = [x for x in res if (x[1], x[2], x[3]) == (kp, ki, kd)]
    if r:
        c, _, _, _, d = r[0]
        print(f"referencia Kp {kp} Ki {ki} Kd {kd}: costo {c:.2f}, puesto {res.index(r[0])+1}, dentro 1 cm {d['dentro1']*100:.0f}%")
json.dump([dict(costo=r[0], kp=r[1], ki=r[2], kd=r[3], **r[4]) for r in res], open(f"ident/busqueda_{nombre}.json", "w"), indent=0)
