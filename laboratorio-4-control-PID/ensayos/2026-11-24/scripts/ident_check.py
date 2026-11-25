# Verificacion del modelo: ajuste vectorizado, validacion por ENSAYO (no por ventana), varios arranques y referencias triviales.
import glob, json, numpy as np
from scipy.signal import savgol_filter
from scipy.optimize import minimize
import os
TS=0.05; N=20
E="C:/Users/villa/Documents/Facultad 2026/Sistemas Electronicos de instrumentacion/laboratorio-4-control-PID/ensayos/2026-11-24/"
A=sorted(glob.glob("ident/abierto_*.csv"))+[E+f for f in ("11_crudo_vs_filtrado.csv","13_empujon12_espera300ms_kp3.csv","14a_config_A_kp2_kd1.5_v60.csv","14b_config_B_kp2_kd2.5_v60.csv","14c_config_C_kp2_kd1.5_v100.csv","15_final_A_ensayo_largo.csv")]
X=[];U=[];V=[];G=[]
for i,f in enumerate(A):
    d=np.genfromtxt(f,delimiter=",",names=True); x=d["posicion_cm"]; u=-(d["servo_us"]-1525)/11.1
    if len(x)<30: continue
    tick=d["tick_ms"]; t=np.arange(tick[0],tick[-1],50.); x=np.interp(t,tick,x); u=np.interp(t,tick,u)
    for k in range(8,len(x)-N,10):
        w=x[k:k+N]
        if w.max()>12 or w.min()<-17: continue
        X.append(w);U.append(u[k-8:k+N]);G.append(i)
        V.append(np.polyfit(np.arange(5)*TS,x[k-4:k+1],1)[0])   # velocidad solo con el pasado
X=np.array(X);U=np.array(U);V=np.array(V);G=np.array(G)
def sim(p,mod,ret,m):
    K,a0,b,Fs,Fc=p; x=X[m,0].copy(); v=V[m].copy(); out=[x.copy()]
    for k in range(8,8+N-1):
        f=K*(U[m,k-ret]-a0)
        for _ in range(5):
            if mod<3: v=v+(f-b*v)*.01
            else:
                st=(abs(v)<.5)&(abs(f)<Fs); s=np.where(abs(v)>=.5,np.sign(v),np.sign(f))
                v=np.where(st,0,v+(f-b*v-Fc*s)*.01)
            x=x+v*.01
        out.append(x.copy())
    return np.array(out).T
rms=lambda e:np.sqrt(np.mean(e**2))
def fit(mod,m,ret):
    n={1:2,2:3,3:5}[mod]; best=None
    for a0 in (-6,0,4):
        for fs in ((2,8) if mod==3 else (0,)):
            q0=np.array([.6,a0,2,fs,fs*.4])[:n]
            def f(q):
                p=np.zeros(5);p[:n]=q
                if p[0]<=0 or min(p[2:])<0: return 1e6
                return np.mean((sim(p,mod,ret,m)-X[m])**2)
            r=minimize(f,q0,method="Nelder-Mead",options=dict(maxiter=500,xatol=1e-2,fatol=1e-4))
            if best is None or r.fun<best[0]: p=np.zeros(5);p[:n]=r.x;best=(r.fun,p)
    return best
todo=np.ones(len(X),bool); g=np.unique(G); rng=np.random.default_rng(1); folds=np.array_split(rng.permutation(g),4)
print(len(X),"ventanas,",len(g),"ensayos; ventanas lazo abierto:",sum('abierto' in A[i] for i in G))
print(f"referencia 'pelota quieta' {rms(X-X[:,:1]):.2f}; 'velocidad constante' {rms(X[:,:1]+V[:,None]*np.arange(N)*TS-X):.2f} cm")
R={}
for mod in (1,2,3):
    ret=min(range(4),key=lambda r:fit(mod,todo,r)[0]); p=fit(mod,todo,ret)[1]; err=[];ps=[]
    for fo in folds:
        te=np.isin(G,fo); q=fit(mod,~te,ret)[1]; ps.append(q); err.append((sim(q,mod,ret,te)-X[te]).ravel())
    cv=rms(np.concatenate(err)); ps=np.array(ps)
    print(f"M{mod} ret {ret*50} ms p=[K,a0,b,Fs,Fc]={np.round(p,2)} RMS ajuste {rms(sim(p,mod,ret,todo)-X):.2f} RMS por ensayo {cv:.2f}")
    print("    parametros en los 4 ajustes:",np.round(ps,2).tolist(),flush=True)
    R[f"M{mod}"]=dict(zip(("K","ang0","b","Fs","Fc"),map(float,p)),retardo_muestras=ret,rms_por_ensayo_cm=float(cv))
json.dump(R,open("ident/modelos_verificados.json","w"),indent=1)
