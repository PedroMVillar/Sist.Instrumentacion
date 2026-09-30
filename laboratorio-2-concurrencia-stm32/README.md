# Laboratorio 2 — Concurrencia sobre STM32

Serie de seis desafíos de complejidad ascendente sobre el camino que va de la programación secuencial y bloqueante a la concurrencia cooperativa y preemptiva.

**Plataforma:** STM32F411E-DISCO · **Entorno:** STM32CubeIDE (librerías HAL Cube)

> Estado: sin empezar. Por ahora solo está la consigna.

## Hardware

| Elemento | Pin |
|---|---|
| Botón de usuario | `PA0` (activo en alto) |
| LED verde | `PD12` |
| LED naranja | `PD13` |
| LED rojo | `PD14` |
| LED azul | `PD15` |

## Desafíos

| | Título | Concepto |
|---|---|---|
| 1 | Memoria de estado y antirrebote | Detección de flancos y antirrebote bloqueante con `HAL_Delay` |
| 2 | Liberando la CPU | Temporización cooperativa no bloqueante con `HAL_GetTick()` |
| 3 | Máquina de estados finitos | Integración de entradas asíncronas y temporización en un super-loop |
| 4 | Precisión por hardware | Timer dedicado (`TIM2`) por polling del flag de desbordamiento |
| 5 | Temporización en segundo plano | Concurrencia preemptiva con interrupciones (`TIM3`, NVIC e ISR) |
| 6 | Antirrebote profesional | Muestreo del pulsador dentro de una ISR (`TIM4`) y bandera `volatile` |

Consigna completa en [`consigna/guia-laboratorio-stm32.pdf`](./consigna).
