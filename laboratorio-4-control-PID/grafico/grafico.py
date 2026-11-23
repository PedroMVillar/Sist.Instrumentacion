# Grafico en tiempo real del balancin.
#
# La placa manda por UART una linea "tiempo_ms,posicion_cm" cada 50 ms.
# Este programa las lee y dibuja la posicion de la pelota respecto del
# centro de la viga. Sirve para calibrar: ver cuanto oscila y cuanto
# tarda en quedarse quieta.
#
# Uso:  python grafico.py          (busca solo el adaptador USB-UART)
#       python grafico.py COM8     (o se le dice el COM a mano)
# Necesita:  pip install pyserial matplotlib

import sys
from collections import deque

import serial
import matplotlib.pyplot as plt

from serial.tools import list_ports


def buscar_adaptador():
    """Busca el adaptador USB-UART (CP210x, CH340, FTDI o PL2303).
    Los puertos Bluetooth se ignoran."""
    for p in list_ports.comports():
        if p.vid in (0x10C4, 0x1A86, 0x0403, 0x067B):
            return p.device
    return None


puerto = sys.argv[1] if len(sys.argv) > 1 else buscar_adaptador()
if puerto is None:
    print("No encuentro el adaptador USB-UART. Puertos que hay:")
    for p in list_ports.comports():
        print(f"  {p.device}: {p.description}")
    print("Revisar que este enchufado y que tenga el driver instalado.")
    sys.exit(1)

try:
    placa = serial.Serial(puerto, 115200, timeout=0.05)
except serial.SerialException as error:
    print(f"No pude abrir {puerto}: {error}")
    print("Si es un puerto Bluetooth, no es el adaptador. Si es el adaptador,")
    print("cerrar cualquier otro programa que lo tenga abierto.")
    sys.exit(1)
print(f"Conectado a {puerto}. Cerrar la ventana del grafico para salir.")

# Ultimos 20 segundos (20 muestras por segundo)
tiempos = deque(maxlen=400)
posiciones = deque(maxlen=400)

figura, eje = plt.subplots()
curva, = eje.plot([], [])
eje.axhline(0, color="gray", linewidth=0.8)  # el centro de la viga
eje.set_xlabel("tiempo [s]")
eje.set_ylabel("distancia al centro [cm]")
eje.set_ylim(-30, 30)
eje.set_title("Balancin: posicion de la pelota")

while plt.fignum_exists(figura.number):
    # Leer todas las lineas que llegaron desde la ultima vuelta
    while placa.in_waiting:
        linea = placa.readline().decode(errors="ignore").strip()
        partes = linea.split(",")
        if len(partes) != 2:
            continue  # linea cortada, se descarta
        try:
            tiempo = int(partes[0]) / 1000.0
            posicion = float(partes[1])
        except ValueError:
            continue
        tiempos.append(tiempo)
        posiciones.append(posicion)
        print(f"\rposicion = {posicion:7.2f} cm   ", end="")

    if tiempos:
        curva.set_data(tiempos, posiciones)
        eje.set_xlim(max(0.0, tiempos[-1] - 20.0), tiempos[-1] + 0.5)
    plt.pause(0.1)
