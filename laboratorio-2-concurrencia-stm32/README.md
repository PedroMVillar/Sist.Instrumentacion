# Laboratorio 2 — Concurrencia sobre STM32

Serie de seis desafíos de complejidad ascendente sobre el camino que va de la programación secuencial y bloqueante a la concurrencia cooperativa y preemptiva.

**Placa:** NUCLEO-F411RE · **Entorno:** VS Code + CMake (librerías HAL Cube)

> La guía original asume la F411E-DISCO; el mapeo de pines adaptado, el cableado de los LEDs externos y la configuración de CubeMX están en [`setup.md`](./setup.md).

## Hardware

| Elemento | Pin | Dónde |
|---|---|---|
| Botón de usuario | `PC13` (activo en bajo) | En la placa (B1) |
| LED verde | `PA5` | En la placa (LD2) |
| LED naranja | `PA6` (D12) | Externo, en protoboard |
| LED rojo | `PA7` (D11) | Externo, en protoboard |
| LED azul | `PB6` (D10) | Externo, en protoboard |

## Desafíos

| | Título | Concepto |
|---|---|---|
| 1 | Memoria de estado y antirrebote | Detección de flancos y antirrebote bloqueante con `HAL_Delay` |
| 2 | Liberando la CPU | Temporización cooperativa no bloqueante con `HAL_GetTick()` |
| 3 | Máquina de estados finitos | Integración de entradas asíncronas y temporización en un super-loop |
| 4 | Precisión por hardware | Timer dedicado (`TIM2`) por polling del flag de desbordamiento |
| 5 | Temporización en segundo plano | Concurrencia preemptiva con interrupciones (`TIM3`, NVIC e ISR) |
| 6 | Antirrebote profesional | Muestreo del pulsador dentro de una ISR (`TIM4`) y bandera `volatile` |

## Estructura

| | Qué contiene |
|---|---|
| [`consigna/`](./consigna) | Guía oficial de la cátedra |
| [`setup.md`](./setup.md) | Cableado y configuración de CubeMX, paso a paso |
| [`firmware/`](./firmware) | Proyecto CubeMX + CMake; los desafíos están en [`firmware/desafios/`](./firmware/desafios) |
| [`explicacion.md`](./explicacion.md) | Explicación de cada desafío, para estudiar |
