# Laboratorio 4 — Control PID: ball and beam

Una bola rueda sobre una viga. Un sensor ultrasónico mide dónde está, un servo inclina la viga, y un controlador PID calcula cada 50 ms cuánto inclinarla para mantener la bola en el centro. Todo se mira y se ajusta desde una consola serie con gráfico en tiempo real.

**Placa:** STM32F4Discovery (F407VG) · **Entorno:** VS Code + CMake (librerías HAL Cube)

> El cableado del sensor y el servo, y los parámetros a ajustar con la maqueta armada, están en [`setup.md`](./setup.md). La teoría del PID y el procedimiento de sintonización están en [`explicacion.md`](./explicacion.md).

## Hardware

| Elemento | Pin | Configuración |
|---|---|---|
| HC-SR04 — TRIG | `PB0` | Salida digital |
| HC-SR04 — ECHO | `PB1` | `TIM3` canal 4, Input Capture (tick de 1 µs) |
| Servo — señal | `PA1` | `TIM2` canal 2, PWM (pulso en µs) |
| Consola | `PA2`/`PA3` | `USART2` a 115200, por el USB del ST-LINK |
| LEDs de la placa | `PD12`–`PD15` | Estado: rojo sin pelota, verde centrada, azul controlando |

## Estructura

| | Qué contiene |
|---|---|
| [`consigna/`](./consigna) | Guía de la cátedra: teoría PID e implementación de referencia |
| [`setup.md`](./setup.md) | Cableado, configuración y parámetros de la maqueta |
| [`firmware/`](./firmware) | Proyecto CubeMX + CMake; el programa está en [`firmware/app/`](./firmware/app) |
| [`firmware/libs/`](./firmware/libs) | `HC_SR04` (del classroom, sin modificar) y `pid` (propia) |
| [`consola/`](./consola) | `consola.py`: telemetría en la terminal, comandos y gráfico en tiempo real |
| [`explicacion.md`](./explicacion.md) | Teoría del PID y cómo sintonizarlo en la maqueta |
