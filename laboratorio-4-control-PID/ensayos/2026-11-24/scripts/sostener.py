# Espera a que pongan la pelota cerca del centro, la sostiene con el PID (ganancias viejas)
# y graba posicion y salida para estimar el angulo que hace falta para tenerla quieta.
# Uso: python sostener.py salida.csv segundos
import sys, time, struct, socket, subprocess
salida, T = sys.argv[1], float(sys.argv[2])
srv = subprocess.Popen(["openocd", "-f", "interface/stlink.cfg", "-f", "target/stm32f4x.cfg"],
                       stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
time.sleep(1.5)
s = socket.create_connection(("127.0.0.1", 6666))
def cmd(c):
    s.sendall(c.encode() + b"\x1a"); buf = b""
    while not buf.endswith(b"\x1a"): buf += s.recv(4096)
    return buf[:-1].decode()
fw = lambda x: struct.unpack("<I", struct.pack("<f", x))[0]
f32 = lambda w: struct.unpack("<f", struct.pack("<I", w))[0]
def leer():
    tick = int(cmd("read_memory 0x2000020c 32 1").split()[0], 0)
    b = [int(x, 0) for x in cmd("read_memory 0x200002a4 32 6").split()]
    return tick, f32(b[3]), f32(b[0]), f32(b[5])
try:
    cmd("mww 0x2000000c 0")
    cmd("mww 0x20000018 1315")
    print("esperando la pelota cerca del centro...", flush=True)
    t0 = time.time(); cerca = 0
    while time.time() - t0 < 240:
        _, p, _, _ = leer()
        cerca = cerca + 1 if abs(p) < 4 else 0
        if cerca >= 10:
            break
        time.sleep(0.05)
    else:
        print("no aparecio la pelota en el centro"); sys.exit(1)
    # ganancias viejas, banda 1 cm, PID andando
    cmd(f"mww 0x200002cc 0x{fw(2.5):08x}")
    cmd(f"mww 0x200002d0 0x{fw(0.3):08x}")
    cmd(f"mww 0x200002d4 0x{fw(0.8):08x}")
    cmd(f"mww 0x200002a8 0x{fw(1.0):08x}")
    cmd("mww 0x2000000c 1")
    print("pelota detectada, PID andando", flush=True)
    f = open(salida, "w"); ultimo = None; t0 = time.time()
    while time.time() - t0 < T:
        tick, p, sp, out = leer()
        if ultimo is None or tick - ultimo >= 50:
            ultimo = tick
            f.write(f"{tick},{p:.2f},{sp:.2f},{out:.2f}\n")
    f.close()
    print("listo", flush=True)
finally:
    s.close(); srv.terminate()
