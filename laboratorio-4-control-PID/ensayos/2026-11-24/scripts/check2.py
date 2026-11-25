import json,glob,numpy as np, io, contextlib
src=open("ident_check.py").read().split("todo=np.ones")[0]; exec(src)
R=json.load(open("ident/modelos_verificados.json"))
mov=np.ptp(X,axis=1)>2
print(f"ventanas con movimiento: {mov.sum()} de {len(X)}; quieta RMS {rms((X-X[:,:1])[mov]):.2f}")
for k in ("M1","M2","M3"):
    m=R[k];p=[m[c] for c in("K","ang0","b","Fs","Fc")]
    e=sim(p,int(k[1]),m["retardo_muestras"],np.ones(len(X),bool))-X
    print(k,f"RMS en movimiento {rms(e[mov]):.2f}, quietas {rms(e[~mov]):.2f}")
# frenadas por posicion SOLO lazo abierto: de las corridas que cruzan cada celda de 1 cm, tiempo mediano por celda
print("lazo abierto, corridas que cruzan >8cm: segundos en cada cm (mediana), n")
T={}
for f in sorted(glob.glob("ident/abierto_*.csv")):
    d=np.genfromtxt(f,delimiter=",",names=True);x=d["posicion_cm"];t=d["tick_ms"]/1000
    if np.ptp(x)<8: continue
    i=np.argmax(abs(x-x[0])>1); c=np.floor(x[i:-1]).astype(int); dt=np.diff(t[i:])
    for q in np.unique(c): T.setdefault((q,x[-1]>x[0]),[]).append(dt[c==q].sum())
for lado in(True,False): print("ida lejos" if lado else "ida sensor",[(int(q),round(float(np.median(v)),2),len(v)) for (q,l),v in sorted(T.items()) if l==lado and len(v)>=2])
# lazo cerrado con modelos verificados
from sim_lazo import simular,costo
sps=[(0,0.),(20,5.),(35,0.),(50,None)];tr=[(12,20),(28,35),(43,50)]
for k in("M2","M3"):
    for hz in(1525,1592):
        for g in((4,.1,.5),(4,.8,.5),(3,.2,1.5),(2,.1,1.5)):
            m=dict(R[k]);m["ang0"]+= (hz-1525)/11.1
            D=[costo(simular(m,*g,sps,x0=-6.,semilla=s),tr)[1] for s in range(4)]
            print(k,hz,g,{q:round(float(np.mean([d[q] for d in D])),2) for q in D[0]},flush=True)
