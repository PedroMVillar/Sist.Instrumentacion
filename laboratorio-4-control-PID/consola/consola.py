# Consola del balancín.
#
# Muestra en la terminal lo que manda la placa, grafica la posición de la
# pelota en tiempo real, y lo que escribas (por ejemplo "modo 1" o "kp 3")
# se lo manda a la placa.
#
# Uso:  python consola.py COM5        (el COM del ST-LINK, mirar en
#                                      Administrador de dispositivos)
# Necesita:  pip install pyserial matplotlib

import sys
import threading
from collections import deque

import serial
import matplotlib.pyplot as plt

puerto = sys.argv[1] if len(sys.argv) > 1 else "COM5"
placa = serial.Serial(puerto, 115200, timeout=0.2)
print(f"Conectado a {puerto}. Comandos: modo, kp, ki, kd, centro, sentido, servo, max")
print('Ejemplo: "modo 1" y Enter. Cerrar la ventana del gráfico para salir.\n')

# Historia de ~30 segundos (la placa manda 20 muestras por segundo)
tiempos = deque(maxlen=600)
posiciones = deque(maxlen=600)

def leer_placa():
    """Lee las líneas que manda la placa: telemetría al gráfico, el resto a pantalla."""
    while True:
        linea = placa.readline().decode(errors="ignore").strip()
        if not linea:
            continue
        partes = linea.split(",")
        if len(partes) == 6:  # ms,modo,distancia,posicion,angulo,servo
            ms, modo, dist, pos, ang, us = partes
            tiempos.append(int(ms) / 1000.0)
            posiciones.append(float(pos))
            estado = f"modo={modo}  dist={dist} cm  pos={pos} cm  angulo={ang}  servo={us} us"
            print("\r" + estado + "   ", end="")
        else:
            print("\n" + linea)  # respuesta a un comando ("ok kp = 3.000")

def leer_teclado():
    """Lo que escribas en la terminal va directo a la placa."""
    while True:
        comando = input()
        if comando.strip():
            placa.write((comando.strip() + "\n").encode())

threading.Thread(target=leer_placa, daemon=True).start()
threading.Thread(target=leer_teclado, daemon=True).start()

# Gráfico en tiempo real: posición de la pelota respecto del centro
figura, eje = plt.subplots()
curva, = eje.plot([], [])
eje.axhline(0, color="gray", linewidth=0.8)  # el centro (setpoint)
eje.set_xlabel("tiempo [s]")
eje.set_ylabel("posición [cm]")
eje.set_ylim(-30, 30)
eje.set_title("Balancín: posición de la pelota")

while plt.fignum_exists(figura.number):
    if tiempos:
        curva.set_data(tiempos, posiciones)
        eje.set_xlim(max(0.0, tiempos[-1] - 30.0), tiempos[-1] + 0.5)
    plt.pause(0.2)
