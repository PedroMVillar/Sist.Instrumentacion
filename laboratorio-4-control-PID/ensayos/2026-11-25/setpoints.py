# Serie segura: +/-20 grados, 60 grados/s, se corta toda si la pelota pasa 17 cm (3 lecturas seguidas).
# Cada config kp,ki,kd,alfa,horiz: 0 (10 s), +5 (12 s), 0 (12 s)
import subprocess,time,sys,numpy as np
import lab
ELF="C:/Users/villa/Documents/Facultad 2026/Sistemas Electronicos de instrumentacion/laboratorio-4-control-PID/firmware/build/Debug/firmware.elf"
S={l.split()[2]:int(l.split()[0],16) for l in subprocess.check_output(["arm-none-eabi-nm",ELF],text=True).splitlines() if len(l.split())==3}
lab.TICK=S["uwTick"];lab.BLOQUE=S["setpoint_cm"];P=S["pid_bola"]
NM=["setpoint_cm","distance_cm","posicion_cm","distancia_filtrada_cm","salida_angulo","servo_us","angulo_servo","lecturas_distancia"]
B=min(S[n] for n in NM);NW=(max(S[n] for n in NM)-B)//4+1;assert NW<=24
def leer(self):
    tick=int(self.cmd(f"read_memory 0x{S['uwTick']:08x} 32 1").split()[0],0)
    b=[int(x,0) for x in self.cmd(f"read_memory 0x{B:08x} 32 {NW}").split()]
    w=[b[(S[n]-B)//4] for n in NM]
    return (tick,)+tuple(lab.f32(x) for x in w[:5])+(w[5],lab.f32(w[6]),w[7])
lab.Placa.leer=leer
p=lab.Placa();time.sleep(2)
def apagar():
    try:
        p.escribir_i(S["modo"],0);time.sleep(.1);print("modo al final",p.cmd(f"read_memory 0x{S['modo']:08x} 32 1"))
    finally: p.cerrar()
try:
    r=p.leer()
    if abs(r[3])>12: print("NO arranco, pos %.1f"%r[3]);raise SystemExit
    corte=False
    print("flash",[round(lab.f32(int(x,0)),2) for x in p.cmd(f"read_memory 0x{P:08x} 32 6").split()])
    SP=[0,-4,-8,-4,0,4,8,4,0];D=14;R=[];u=None;f=0;sp=0.;p.escribir_f(S["setpoint_cm"],0.);p.escribir_i(S["modo"],1);t0=time.time()
    while (T:=time.time()-t0)<D*len(SP):
        s=float(SP[int(T//D)])
        if s!=sp: p.escribir_f(S["setpoint_cm"],s);sp=s
        r=p.leer()
        if r[8]!=u:
            u=r[8];R.append((T,)+r);f=f+1 if abs(r[3])>17 else 0
            if f>=3: print("CORTE");break
    R=np.array(R);np.savetxt(f"d25/{sys.argv[1]}.csv",R[:,1:],fmt="%.3f",delimiter=",",comments="",header="tick_ms,setpoint,crudo_cm,posicion_cm,filtrada_cm,salida_grados,servo_us,angulo_servo,lecturas")
    t=R[:,0];x=R[:,4]
    for k,q in enumerate(SP):
        m=(t>=D*k+7)&(t<D*k+14);w=(t>=D*k)&(t<D*k+14)
        if m.any(): print(f"sp {q:3d}: media {x[m].mean():5.1f} err {np.mean(abs(x[m]-q)):.1f} in1 {np.mean(abs(x[m]-q)<=1)*100:3.0f} in2 {np.mean(abs(x[m]-q)<=2)*100:3.0f} rango {x[w].min():.1f}..{x[w].max():.1f}")
    print("salto crudo mediano %.2f picos %d"%(np.median(abs(np.diff(R[:,3]))),sum(abs(R[:,3]-R[:,5])>3)))
finally:
    p.escribir_f(S["setpoint_cm"],0.)
    apagar()