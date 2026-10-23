# Laboratorio 3 — Sensores y librerías

Cuatro desafíos sobre adquisición de datos del entorno: integrar una librería de terceros, escribir una librería propia multi-instancia, medir el costo de bloquear el procesador y cerrar con un super-loop disparado por tiempo que atiende tres sensores a la vez.

> Estado: en preparación. Por ahora solo está la consigna.

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
