# Busca el pulso de viga horizontal con la pelota en el medio.
# Para cada pulso: centra la pelota con el PID (modo 1), pasa a modo 0 con ese pulso
# y mide hacia donde rueda en T segundos.
# Uso: python horizontal.py p1,p2,... [T]
import sys, time, struct, socket, subprocess
pulsos = [int(x) for x in sys.argv[1].split(",")]
T = float(sys.argv[2]) if len(sys.argv) > 2 else 2.0
CENTRO_PID = 1315
srv = subprocess.Popen(["openocd", "-f", "interface/stlink.cfg", "-f", "target/stm32f4x.cfg"],
                       stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
time.sleep(1.5)
s = socket.create_connection(("127.0.0.1", 6666))
def cmd(c):
    s.sendall(c.encode() + b"\x1a"); buf = b""
    while not buf.endswith(b"\x1a"): buf += s.recv(4096)
    return buf[:-1].decode()
f32 = lambda w: struct.unpack("<f", struct.pack("<I", w))[0]
pos = lambda: f32(int(cmd("read_memory 0x200002b0 32 1").split()[0], 0))

def centrar(max_s=20):
    cmd(f"mww 0x20000018 {CENTRO_PID}")
    cmd("mww 0x2000000c 1")
    t0 = time.time(); quieto = 0; prev = pos()
    while time.time() - t0 < max_s:
        time.sleep(0.1); p = pos()
        quieto = quieto + 1 if abs(p) < 2.5 and abs(p - prev) < 0.3 else 0
        prev = p
        if quieto >= 8:
            return p
    return None

try:
    for p in pulsos:
        c = centrar()
        if c is None:
            print(f"{p} us: no pude centrarla (pos={pos():.1f})", flush=True)
            continue
        cmd(f"mww 0x20000018 {p}")
        cmd("mww 0x2000000c 0")
        t0 = time.time(); vals = []
        while time.time() - t0 < T:
            vals.append(pos()); time.sleep(0.05)
        d = vals[-1] - vals[0]
        lado = "se aleja del sensor" if d > 2 else ("va hacia el sensor" if d < -2 else "QUIETA")
        print(f"{p} us: {vals[0]:.1f} -> {vals[-1]:.1f} cm  ({lado})", flush=True)
finally:
    cmd(f"mww 0x20000018 {CENTRO_PID}")
    cmd("mww 0x2000000c 0")
    s.close(); srv.terminate()
