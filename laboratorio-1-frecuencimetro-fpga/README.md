# Laboratorio 1 — Frecuencímetro digital sobre FPGA

Frecuencímetro de cuatro dígitos por el método de medición directa: se cuentan los flancos de subida de la señal de entrada dentro de una ventana de 1 s, de modo que la cuenta acumulada es directamente la frecuencia en hertz. Rango 0–9999 Hz, resolución 1 Hz.

**Plataforma:** Boolean Board (Xilinx Spartan-7 `xc7s50csga324-1`) · **Lenguaje:** SystemVerilog · **Herramienta:** Vivado 2024.2

## Módulos

| Archivo | Origen | Función |
|---|---|---|
| `Frequency_meter.sv` | Propio, sobre esqueleto | Top-level: sincronizador, detector de flanco y conexionado |
| `BCD_counter.sv` | Propio | Contador de un dígito, 0 a 9, con `carry_out` de un ciclo |
| `BCD_bank.sv` | Propio | Cuatro contadores BCD en cascada |
| `Latch_register.sv` | Propio | Registro de retención de 16 bits |
| `BaseTime1ms.sv` | Cátedra | Divisor de 100 MHz a un pulso cada 1 ms |
| `FSM_Control.sv` | Cátedra | Unidad de control de tres estados |
| `7seg_controller.sv` | Cátedra | Decodificación y multiplexado del display |
| `boolean.xdc` | Cátedra, con un agregado | Restricciones de pines |

Los archivos tal como los entregó la cátedra están en [`src-catedra/`](./src-catedra), sin modificar, para poder ver el diff.

## Conexionado

| Señal | Pin | Uso |
|---|---|---|
| `clk` | F14 | Oscilador de 100 MHz |
| `btn[0]` | J2 | Reset |
| `servo0` | M14 | Entrada de la señal a medir |
| `D0_SEG`, `D0_AN` | — | Display de siete segmentos |

El pin `servo0` no es de los dedicados a reloj, así que el `.xdc` agrega `set_property CLOCK_DEDICATED_ROUTE FALSE [get_nets servo0_IBUF]`.

## Síntesis

Desde [`src/`](./src), sin abrir la interfaz gráfica:

```bash
vivado -mode batch -source ../informe/reporte_utilizacion.tcl
```

Sintetiza, implementa y escribe el Utilization Report. El top-level es `Frequency_meter`.

## Resultados

Medición contra un generador de funciones GW Instek AFG-2005: **7,894 kHz inyectados, 7894 Hz leídos**.

Recursos consumidos, según [`Frequency_meter_utilization_placed.rpt`](./informe):

| Recurso | Usados | Disponibles | Utilización |
|---|---:|---:|---:|
| Slice LUTs | 51 | 32.600 | 0,16 % |
| Slice Registers | 105 | 65.200 | 0,16 % |
| Bonded IOB | 15 | 210 | 7,14 % |

## Informe

[`informe/informe_lab1.tex`](./informe) — 3 páginas, diagramas de bloques y de estados en TikZ. Compila con dos pasadas:

```bash
pdflatex informe_lab1.tex && pdflatex informe_lab1.tex
```
