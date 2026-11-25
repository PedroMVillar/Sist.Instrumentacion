# Acceso a la placa por OpenOCD (puerto TCL) para los ensayos de identificacion.
# Direcciones del firmware limpio (PID puro), sacadas con arm-none-eabi-nm.
import time, struct, socket, subprocess

MODO = 0x2000000c
CENTRO_VIGA = 0x20000010
SERVO_CENTRO = 0x20000018
VEL_SERVO = 0x20000020
TICK = 0x2000020c
BLOQUE = 0x200002a4      # setpoint, crudo, posicion, filtrada, salida, servo_us, angulo, lecturas
PID = 0x200002c8         # kp, ki, kd, ts, max, min, integral, medida_previa, vel_filtrada
KP, KI, KD, INTEGRAL = PID, PID + 4, PID + 8, PID + 24
SETPOINT = BLOQUE
HORIZONTAL_US = 1525

fw = lambda x: struct.unpack("<I", struct.pack("<f", x))[0]
f32 = lambda w: struct.unpack("<f", struct.pack("<I", w))[0]


class Placa:
    def __init__(self):
        self.srv = subprocess.Popen(["openocd", "-f", "interface/stlink.cfg", "-f", "target/stm32f4x.cfg"],
                                    stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        time.sleep(1.5)
        self.s = socket.create_connection(("127.0.0.1", 6666))

    def cmd(self, c):
        self.s.sendall(c.encode() + b"\x1a")
        buf = b""
        while not buf.endswith(b"\x1a"):
            buf += self.s.recv(4096)
        return buf[:-1].decode()

    def escribir_f(self, addr, x):
        self.cmd(f"mww 0x{addr:08x} 0x{fw(x):08x}")

    def escribir_i(self, addr, n):
        self.cmd(f"mww 0x{addr:08x} {int(n) & 0xFFFFFFFF}")

    def leer(self):
        """(tick_ms, setpoint, crudo, posicion, filtrada, salida, servo_us, angulo, lecturas)"""
        tick = int(self.cmd(f"read_memory 0x{TICK:08x} 32 1").split()[0], 0)
        b = [int(x, 0) for x in self.cmd(f"read_memory 0x{BLOQUE:08x} 32 8").split()]
        return (tick, f32(b[0]), f32(b[1]), f32(b[2]), f32(b[3]), f32(b[4]),
                struct.unpack("<i", struct.pack("<I", b[5]))[0], f32(b[6]), b[7])

    def cerrar(self):
        self.s.close()
        self.srv.terminate()
