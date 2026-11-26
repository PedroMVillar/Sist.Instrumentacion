# Una sola prueba corta: 15 s, +/-20 grados, 60 grados/s, corta si |pos|>15 cm. Direcciones leidas del .elf.
import subprocess,time,sys,numpy as np
import lab
ELF="C:/Users/villa/Documents/Facultad 2026/Sistemas Electronicos de instrumentacion/laboratorio-4-control-PID/firmware/build/Debug/firmware.elf"
S={l.split()[2]:int(l.split()[0],16) for l in subprocess.check_output(["arm-none-eabi-nm",ELF],text=True).splitlines() if len(l.split())==3}
lab.TICK=S["uwTick"];lab.BLOQUE=S["setpoint_cm"]
assert [S[n]-S["setpoint_cm"] for n in("distance_cm","posicion_cm","distancia_filtrada_cm","salida_angulo","servo_us","angulo_servo","lecturas_distancia")]==[4,8,12,16,20,24,28]
P=S["pid_bola"];kp,ki,kd=map(float,sys.argv[2].split(","))
p=lab.Placa();R=[];why="15 s completos"
try:
    r=p.leer();u=r[8];time.sleep(.5);r=p.leer()
    print("inicio pos %.1f dist %.1f"%(r[3],r[2]))
    if r[8]==u or abs(r[3])>10: why="NO arranco: pelota lejos del medio o sensor sin leer"
    else:
        for o,v in((0,kp),(4,ki),(8,kd),(16,20.),(20,-20.),(24,0.)): p.escribir_f(P+o,v)
        p.escribir_f(S["setpoint_cm"],0.);p.escribir_f(S["velocidad_servo"],60.)
        chk=[lab.f32(int(x,0)) for x in p.cmd(f"read_memory 0x{P:08x} 32 6").split()];print("pid",np.round(chk,2))
        assert abs(chk[3]-.05)<1e-6 and chk[4]==20 and chk[5]==-20
        p.escribir_i(S["modo"],1);t0=time.time();u=None
        while time.time()-t0<15:
            r=p.leer()
            if r[8]!=u:
                u=r[8];R.append(r)
                if abs(r[3])>15: why="CORTE: paso los 15 cm";break
finally:
    try: p.escribir_i(S["modo"],0);time.sleep(.1);print("modo al final",p.cmd(f"read_memory 0x{S['modo']:08x} 32 1"))
    finally: p.cerrar()
print(why)
if R:
    R=np.array(R);np.savetxt(sys.argv[1],R,fmt="%.3f",delimiter=",",comments="",header="tick_ms,setpoint,crudo_cm,posicion_cm,filtrada_cm,salida_grados,servo_us,angulo_servo,lecturas")
    t=(R[:,0]-R[0,0])/1000;x=R[:,3]
    print("max salto servo",abs(np.diff(R[:,6])).max(),"us; rango",R[:,6].min(),R[:,6].max(),"| dentro 1cm %.0f%%"%(np.mean(abs(x)<=1)*100))
    for i in range(0,len(t),8): print("t=%4.1f pos=%5.1f sal=%5.1f us=%d"%(t[i],x[i],R[i,5],R[i,6]))
