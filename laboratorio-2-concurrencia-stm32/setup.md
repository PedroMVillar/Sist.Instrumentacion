# Setup — Nucleo F411RE, cableado y CubeMX

La guía de la cátedra asume la placa **STM32F411E-DISCO** (4 LEDs soldados y botón en PA0). Acá usamos la **NUCLEO-F411RE**, que trae un solo LED y el botón en otro pin, así que adaptamos los pines y cableamos 3 LEDs externos.

## Mapeo de pines

| Señal | En la guía (Disco) | Acá (Nucleo F411RE) | Dónde está |
|---|---|---|---|
| Botón | `PA0` (activo en **alto**) | `PC13` (activo en **bajo**) | Botón azul de la placa (B1) |
| LED verde | `PD12` | `PA5` | LED de la placa (LD2 / D13) |
| LED naranja | `PD13` | `PA6` | Pin `D12`, LED externo |
| LED rojo | `PD14` | `PA7` | Pin `D11`, LED externo |
| LED azul | `PD15` | `PB6` | Pin `D10`, LED externo |

> **Ojo con el botón:** en la Nucleo, presionado se lee `GPIO_PIN_RESET` (0) y suelto `GPIO_PIN_SET` (1) — al revés que en la guía. El código ya lo tiene en cuenta.

## Cableado

Hacen falta: protoboard, 3 LEDs y 3 resistencias de 220–330 Ω. Todo va en el conector **CN5** (la tira de arriba, la de `D13`), donde también hay un pin `GND` a dos lugares de `D13`.

```
D12 (PA6) ──[220 Ω]──▶|── naranja ──┐
D11 (PA7) ──[220 Ω]──▶|── rojo    ──┼── GND (pin GND de CN5)
D10 (PB6) ──[220 Ω]──▶|── azul    ──┘
```

Cada rama es: **pin → resistencia → pata larga del LED (ánodo) → pata corta (cátodo) → GND**. El LED verde y el botón ya están en la placa, no se cablea nada más. La placa se alimenta y programa por el mismo cable USB (ST-LINK integrado).

## CubeMX, paso a paso

1. **New Project → Board Selector**, buscar `NUCLEO-F411RE`, **Start Project**. Cuando pregunta *"Initialize all peripherals with their default Mode?"* → **No**.

2. **Pinout & Configuration** — clic en cada pin del dibujo del chip y asignar:

   | Pin | Modo | User Label |
   |---|---|---|
   | `PA5` | GPIO_Output | `LED_VERDE` |
   | `PA6` | GPIO_Output | `LED_NARANJA` |
   | `PA7` | GPIO_Output | `LED_ROJO` |
   | `PB6` | GPIO_Output | `LED_AZUL` |
   | `PC13` | GPIO_Input | `BOTON` |

   (El label se pone con clic derecho sobre el pin → *Enter User Label*. `PC13` queda sin pull interno: la placa ya trae la resistencia de pull-up.)

3. **Timers** (menú de la izquierda → *Timers*). Los tres se configuran igual, solo cambia el período:

   | Timer | Clock Source | Prescaler (PSC) | Counter Period (ARR) | Desborda cada | NVIC |
   |---|---|---|---|---|---|
   | `TIM2` | Internal Clock | `16000-1` | `100-1` | 100 ms | no |
   | `TIM3` | Internal Clock | `16000-1` | `500-1` | 500 ms | ✔ *TIM3 global interrupt* |
   | `TIM4` | Internal Clock | `16000-1` | `10-1` | 10 ms | ✔ *TIM4 global interrupt* |

   La interrupción se habilita en la pestaña **NVIC Settings** de cada timer. La cuenta es simple: el timer corre a 16 MHz; con prescaler 16000 queda contando a 1 kHz, o sea **1 cuenta = 1 ms**, y el período en ms se pone directo en el ARR.

4. **Clock Configuration**: no tocar nada. Queda el HSI interno de 16 MHz (verificar que HCLK diga 16).

5. **Project Manager**:
   - *Project Name*: `firmware`
   - *Project Location*: la carpeta `laboratorio-2-concurrencia-stm32` del repo (el proyecto queda en `laboratorio-2-concurrencia-stm32/firmware/`)
   - *Toolchain / IDE*: **CMake**
   - Pestaña *Code Generator*: marcar *Copy only the necessary library files*

6. **GENERATE CODE**.

## Después de generar

Con el proyecto generado se compila y graba desde VS Code (extensiones **CMake Tools** y **Cortex-Debug**, compilador `arm-none-eabi-gcc` y OpenOCD ya instalados). La configuración de build y grabado está en el propio proyecto (`firmware/`).
