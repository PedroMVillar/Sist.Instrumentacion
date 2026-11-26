import time,sys,numpy as np
from lab import *
p=Placa()
try:
    print("ganancias",[round(f32(int(x,0)),2) for x in p.cmd(f"read_memory 0x{PID:08x} 32 3").split()],"centro",p.cmd(f"read_memory 0x{SERVO_CENTRO:08x} 32 1"))
    p.escribir_f(INTEGRAL,0.0)
    R=[];u=None;t0=time.time()
    while time.time()-t0<30:
        r=p.leer()
        if r[8]!=u: u=r[8];R.append(r)
finally: p.cerrar()
R=np.array(R);np.savetxt(sys.argv[1],R,fmt="%.3f",delimiter=",",comments="",header="tick_ms,setpoint,crudo_cm,posicion_cm,filtrada_cm,salida_grados,servo_us,angulo_servo,lecturas")
t=(R[:,0]-R[0,0])/1000;x=R[:,3]
for a in range(0,30,5):
    m=(t>=a)&(t<a+5)
    if m.any(): print(f"{a}-{a+5}s pos {x[m].min():.1f}..{x[m].max():.1f} dist {R[m,2].min():.1f}..{R[m,2].max():.1f} dentro1 {np.mean(abs(x[m])<=1)*100:.0f}% servo {R[m,6].min():.0f}..{R[m,6].max():.0f}")
