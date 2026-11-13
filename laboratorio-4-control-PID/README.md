# Laboratorio 4 — Control PID: ball and beam

Una bola rueda sobre una viga. Un sensor ultrasónico mide dónde está, un servo inclina la viga, y un controlador PID calcula cada 40 ms cuánto inclinarla para mantener la bola en el centro.

**Placa:** STM32F4Discovery (F407VG) · **Entorno:** VS Code + CMake (librerías HAL Cube)

> El cableado del sensor y el servo, y los parámetros a ajustar con la maqueta armada, están en [`setup.md`](./setup.md). La teoría del PID y el procedimiento de sintonización están en [`explicacion.md`](./explicacion.md).

## Hardware

| Elemento | Pin | Configuración |
|---|---|---|
| HC-SR04 — TRIG | `PA2` | Salida digital |
| HC-SR04 — ECHO | `PA1` | `TIM2` canal 2, Input Capture (tick de 1 µs) |
| Servo — señal | `PB6` | `TIM4` canal 1, PWM a 50 Hz (pulso en µs) |
| LEDs de la placa | `PD12`–`PD15` | Libres, por si hacen falta para debug |

## Estructura

| | Qué contiene |
|---|---|
| [`consigna/`](./consigna) | Guía de la cátedra: teoría PID e implementación de referencia |
| [`setup.md`](./setup.md) | Cableado, configuración y parámetros de la maqueta |
| [`firmware/`](./firmware) | Proyecto CubeMX + CMake; el programa está en [`firmware/app/`](./firmware/app) |
| [`firmware/libs/`](./firmware/libs) | `HC_SR04` (de la cátedra, sin modificar) y `pid` (propia) |
| [`explicacion.md`](./explicacion.md) | Teoría del PID y cómo sintonizarlo en la maqueta |
