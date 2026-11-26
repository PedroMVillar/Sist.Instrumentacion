# Serie segura: +/-20 grados, 60 grados/s, se corta toda si la pelota pasa 17 cm (3 lecturas seguidas).
# Cada config kp,ki,kd,alfa,horiz: 0 (10 s), +5 (12 s), 0 (12 s)
import subprocess,time,sys,numpy as np
import lab
ELF="C:/Users/villa/Documents/Facultad 2026/Sistemas Electronicos de instrumentacion/laboratorio-4-control-PID/firmware/build/Debug/firmware.elf"
S={l.split()[2]:int(l.split()[0],16) for l in subprocess.check_output(["arm-none-eabi-nm",ELF],text=True).splitlines() if len(l.split())==3}
lab.TICK=S["uwTick"];lab.BLOQUE=S["setpoint_cm"];P=S["pid_bola"]
assert S["lecturas_distancia"]-S["setpoint_cm"]==28
p=lab.Placa();time.sleep(2)
def apagar():
    try:
        p.escribir_i(S["modo"],0);time.sleep(.1);print("modo al final",p.cmd(f"read_memory 0x{S['modo']:08x} 32 1"))
    finally: p.cerrar()
try:
    r=p.leer()
    if abs(r[3])>12: print("NO arranco, pos %.1f"%r[3]);raise SystemExit
    corte=False
    for c in sys.argv[2:]:
        kp,ki,kd,al,hz=map(float,c.split(","))
        p.escribir_f(S["setpoint_cm"],0.);p.escribir_f(S["velocidad_servo"],60.);p.escribir_i(S["servo_centro_us"],hz);p.escribir_f(S["alfa_vel"],al)
        for o,v in((0,kp),(4,ki),(8,kd),(16,20.),(20,-20.),(24,0.)): p.escribir_f(P+o,v)
        p.escribir_i(S["modo"],1);t0=time.time();u=None;R=[];sp=0;f=0
        while (T:=time.time()-t0)<34:
            s=5. if 10<=T<22 else 0.
            if s!=sp: p.escribir_f(S["setpoint_cm"],s);sp=s
            r=p.leer()
            if r[8]!=u:
                u=r[8];R.append((T,)+r);f=f+1 if abs(r[3])>17 else 0
                if f>=3: corte=True;break
        R=np.array(R);np.savetxt(f"d25/{sys.argv[1]}_{c.replace(',','_')}.csv",R[:,1:],fmt="%.3f",delimiter=",",comments="",header="tick_ms,setpoint,crudo_cm,posicion_cm,filtrada_cm,salida_grados,servo_us,angulo_servo,lecturas")
        t=R[:,0];e=R[:,4]-R[:,2];o=[]
        for a,b in((4,10),(16,22),(28,34)):
            m=(t>=a)&(t<b)
            if m.any():o.append(f"err {np.mean(abs(e[m])):4.1f} in1 {np.mean(abs(e[m])<=1)*100:3.0f} in2 {np.mean(abs(e[m])<=2)*100:3.0f}")
        g=np.mean(abs(np.diff(R[:,6]))>8)*100
        print(f"{c:22s} ini {R[0,4]:5.1f} "+" | ".join(o)+f" | max|x| {abs(R[:,4]).max():.1f} golpes {g:.0f}% movim servo {np.mean(abs(np.diff(R[:,7]))):.1f}us"+(" CORTE" if corte else ""),flush=True)
        if corte: break
finally: apagar()
