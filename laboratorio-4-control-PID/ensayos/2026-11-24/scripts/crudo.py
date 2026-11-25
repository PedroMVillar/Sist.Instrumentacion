# Graba cruda y filtrada a la vez, mas salida del PID y angulo real del servo.
# Una fila por cada lectura nueva del sensor (lecturas_distancia cambia).
# Uso: python crudo.py salida.csv T [t:addr=valor ...]
import sys, time, struct, socket, subprocess
salida, T = sys.argv[1], float(sys.argv[2])
acciones = []
for a in sys.argv[3:]:
    t, resto = a.split(":", 1); addr, val = resto.split("=")
    acciones.append([float(t), int(addr, 16), struct.unpack("<I", struct.pack("<f", float(val)))[0], False])
srv = subprocess.Popen(["openocd", "-f", "interface/stlink.cfg", "-f", "target/stm32f4x.cfg"],
                       stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
time.sleep(1.5)
s = socket.create_connection(("127.0.0.1", 6666))
def cmd(c):
    s.sendall(c.encode() + b"\x1a"); buf = b""
    while not buf.endswith(b"\x1a"): buf += s.recv(4096)
    return buf[:-1].decode()
f32 = lambda w: struct.unpack("<f", struct.pack("<I", w))[0]
f = open(salida, "w")
f.write("tick_ms,setpoint,crudo_cm,posicion_cm,filtrada_cm,salida_grados,servo_us,angulo_servo\n")
ultimo = None; n = 0; inicio = time.time()
try:
    while time.time() - inicio < T:
        ahora = time.time() - inicio
        for a in acciones:
            if not a[3] and ahora >= a[0]:
                cmd(f"mww 0x{a[1]:08x} 0x{a[2]:08x}"); a[3] = True
        tick = int(cmd("read_memory 0x20000210 32 1").split()[0], 0)
        b = [int(x, 0) for x in cmd("read_memory 0x200002a8 32 8").split()]
        if b[7] != ultimo:
            ultimo = b[7]
            sp, crudo, pos, filt, sal, us, ang = f32(b[0]), f32(b[1]), f32(b[2]), f32(b[3]), f32(b[4]), b[5], f32(b[6])
            f.write(f"{tick},{sp:.2f},{crudo:.2f},{pos:.2f},{filt:.2f},{sal:.2f},{us},{ang:.2f}\n")
            n += 1
finally:
    f.close(); s.close(); srv.terminate()
print(f"{n} filas en {salida}")
