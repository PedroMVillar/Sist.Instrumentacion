# Ensayo por el ST-LINK (sin adaptador UART). Levanta OpenOCD como servidor,
# lee por el puerto TCL uwTick + posicion/setpoint/salida lo mas rapido posible
# y guarda un CSV "tiempo_ms,posicion_cm,setpoint_cm,salida_grados" (formato LOG_EXTENDIDO),
# una linea por cada tick nuevo de 50 ms o mas.
# Uso: python ensayo_stlink.py salida.csv T [cmd_en_t ...]  donde cmd = "t:addr=valor_float"
#      o "t:addr=i:entero"
import sys, time, struct, socket, subprocess

salida, T = sys.argv[1], float(sys.argv[2])
acciones = []
for a in sys.argv[3:]:
    t, resto = a.split(":", 1)
    addr, val = resto.split("=")
    if val.startswith("i"):
        w = int(val[2:]) & 0xFFFFFFFF
    else:
        w = struct.unpack("<I", struct.pack("<f", float(val)))[0]
    acciones.append([float(t), int(addr, 16), w, False])

srv = subprocess.Popen(["openocd", "-f", "interface/stlink.cfg", "-f", "target/stm32f4x.cfg"],
                       stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
time.sleep(1.5)
s = socket.create_connection(("127.0.0.1", 6666))

def cmd(c):
    s.sendall(c.encode() + b"\x1a")
    buf = b""
    while not buf.endswith(b"\x1a"):
        buf += s.recv(4096)
    return buf[:-1].decode()

def words(addr, n):
    r = cmd(f"read_memory 0x{addr:08x} 32 {n}")
    return [int(x, 0) for x in r.split()]

f32 = lambda w: struct.unpack("<f", struct.pack("<I", w))[0]
f = open(salida, "w")
inicio = time.time()
ultimo = None
n = 0
try:
    while time.time() - inicio < T:
        ahora = time.time() - inicio
        for a in acciones:
            if not a[3] and ahora >= a[0]:
                cmd(f"mww 0x{a[1]:08x} 0x{a[2]:08x}")
                a[3] = True
                print(f"[t={ahora:.1f}s escribi 0x{a[2]:08x} en 0x{a[1]:08x}]", flush=True)
        tick = words(0x2000020c, 1)[0]
        b = words(0x200002a4, 6)   # setpoint, banda, distance, posicion, ?, salida
        if ultimo is None or tick - ultimo >= 50:
            ultimo = tick
            linea = f"{tick},{f32(b[3]):.2f},{f32(b[0]):.2f},{f32(b[5]):.2f}"
            f.write(linea + "\n")
            n += 1
            if n % 20 == 0:
                print(linea, f"dist={f32(b[2]):.1f}", flush=True)
finally:
    f.close()
    s.close()
    srv.terminate()
print(f"{n} lineas en {salida}")
