# En modo 0, prueba pulsos de servo_centro_us de a uno y mira si la pelota se mueve.
# Uso: python umbral.py p1,p2,p3 [segundos_por_paso] [cm_para_considerar_movida]
import sys, time, struct, socket, subprocess
pulsos = [int(x) for x in sys.argv[1].split(",")]
espera = float(sys.argv[2]) if len(sys.argv) > 2 else 2.5
umbral = float(sys.argv[3]) if len(sys.argv) > 3 else 3.0
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
try:
    cmd("mww 0x2000000c 0")
    inicio = pos()
    print(f"arranque pos={inicio:.1f}")
    for p in pulsos:
        cmd(f"mww 0x20000018 {p}")
        t0 = time.time(); vals = []
        while time.time() - t0 < espera:
            vals.append(pos()); time.sleep(0.05)
        print(f"{p} us: {vals[0]:.1f} -> {vals[-1]:.1f}", flush=True)
        if abs(vals[-1] - inicio) > umbral:
            print(f"SE MOVIO con {p}")
            break
finally:
    s.close(); srv.terminate()
