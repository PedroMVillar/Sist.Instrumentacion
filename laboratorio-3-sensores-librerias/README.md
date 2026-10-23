# Laboratorio 3 — Sensores y librerías

Cuatro desafíos sobre adquisición de datos del entorno: integrar una librería de terceros, escribir una librería propia multi-instancia, medir el costo de bloquear el procesador y cerrar con un super-loop disparado por tiempo que atiende tres sensores a la vez.

**Placa:** STM32F4Discovery (F407VG, prestada por la cátedra) · **Entorno:** VS Code + CMake (librerías HAL Cube)

> El cableado de los sensores y la configuración del proyecto están en [`setup.md`](./setup.md).

## Hardware

| Elemento | Pin | Dónde |
|---|---|---|
| LEDs verde / naranja / rojo / azul | `PD12` / `PD13` / `PD14` / `PD15` | En la placa |
| Acelerómetro LIS3DSH | SPI1 (`PA5/PA6/PA7`) + CS `PE3` | En la placa |
| HC-SR04 (distancia) | TRIG `PA2` · ECHO `PA1` (TIM2 CH2) | Externo, en protoboard |
| LM35 (temperatura) | OUT `PC1` (ADC1 canal 11) | Externo, en protoboard |

## Desafíos

| | Título | Concepto |
|---|---|---|
| 1 | HC-SR04 y librerías de terceros | Integrar la librería de la cátedra (Input Capture) y armar un radar de proximidad con LEDs |
| 2 | LM35 y modularización | Librería propia sobre el ADC, multi-instancia, con API bloqueante y no bloqueante |
| 3 | Acelerómetro y el problema del bloqueo | Lectura por SPI + demostración de cómo `HAL_Delay` arruina la respuesta del sistema |
| 4 | Super-loop e integración total | Los tres sensores conviviendo en un super-loop por tiempo, sin ningún `HAL_Delay` |

## Estructura

| | Qué contiene |
|---|---|
| [`consigna/`](./consigna) | Guía oficial de la cátedra |
| [`setup.md`](./setup.md) | Cableado de los sensores y configuración del proyecto |
| [`firmware/`](./firmware) | Proyecto CubeMX + CMake; los desafíos van en `firmware/desafios/` |
| [`firmware/libs/`](./firmware/libs) | `HC_SR04` (provista por la cátedra, sin modificar) y `lm35` (propia, del desafío 2) |
| `explicacion.md` | Explicación de cada desafío, para estudiar |
