# PID puro. Cada configuracion: 0 (14 s), +5 (10 s), 0 (10 s). Uso: barrido25.py etiqueta kp,ki,kd,vel,horiz ...
import sys,time,numpy as np
from lab import *
p=Placa(); etq=sys.argv[1]
try:
    for c in sys.argv[2:]:
        kp,ki,kd,ve,hz=map(float,c.split(","))
        p.escribir_f(SETPOINT,0.)
        for a,v in((KP,kp),(KI,ki),(KD,kd),(VEL_SERVO,ve),(INTEGRAL,0.)): p.escribir_f(a,v)
        p.escribir_i(SERVO_CENTRO,hz); p.escribir_i(MODO,1)
        R=[];u=None;t0=time.time();sp=0
        while (T:=time.time()-t0)<34:
            s=5. if 14<=T<24 else 0.
            if s!=sp: p.escribir_f(SETPOINT,s);sp=s
            r=p.leer()
            if r[8]!=u:u=r[8];R.append((T,)+r)
        R=np.array(R);np.savetxt(f"d25/{etq}_{c.replace(',','_')}.csv",R[:,1:],fmt="%.3f",delimiter=",",comments="",header="tick_ms,setpoint,crudo_cm,posicion_cm,filtrada_cm,salida_grados,servo_us,angulo_servo,lecturas")
        t=R[:,0];e=R[:,4]-R[:,2];o=[]
        for a,b in((8,14),(19,24),(29,34)):
            m=(t>=a)&(t<b);o.append(f"err {np.mean(abs(e[m])):4.1f} pp {np.ptp(e[m]):4.1f} in1 {np.mean(abs(e[m])<=1)*100:3.0f}%")
        print(f"{c:22s} inicio {R[0,4]:5.1f} | "+" | ".join(o),flush=True)
finally:
    p.escribir_f(SETPOINT,0.); p.cerrar()
