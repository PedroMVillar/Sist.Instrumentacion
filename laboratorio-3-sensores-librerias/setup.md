# Setup — F4Discovery, cableado y configuración

Este lab va en la **STM32F4Discovery (F407VG)** — la placa que presta la cátedra — porque los desafíos 3 y 4 usan el acelerómetro que trae soldado. Los LEDs también son los de la placa (PD12–PD15), así que lo único que se cablea son los dos sensores.

## Cableado

| Sensor | Pin del sensor | Va a | Nota |
|---|---|---|---|
| **HC-SR04** | VCC | `5V` | El sensor funciona con 5 V |
| | GND | `GND` | |
| | TRIG | `PA2` | Salida: el pulso de disparo de 10 µs |
| | ECHO | `PA1` | Entrada del timer (TIM2 canal 2, Input Capture) |
| **LM35** | +Vs (pata izq.) | `5V` | Mirando la cara plana de frente |
| | OUT (pata centro) | `PC1` | Entrada analógica (ADC1 canal 11) |
| | GND (pata der.) | `GND` | |

> **ECHO a 5 V:** el HC-SR04 saca el Echo a 5 V, pero `PA1` es un pin tolerante a 5 V, así que se puede conectar directo. Si querés estar 100 % tranquilo, poné un divisor resistivo (1 kΩ + 2 kΩ) en el medio.

> **El acelerómetro no se cablea:** el LIS3DSH ya está soldado a la placa, conectado por SPI1 (PA5/PA6/PA7) con el chip select en PE3.

## Configuración (ya generada en `firmware/`)

El proyecto está generado con toolchain CMake, igual que el lab 2; el `.ioc` se puede abrir en CubeMX (File → Load Project) para ver todo esto en pantalla. Reloj: HSI interno a 16 MHz, sin PLL.

| Periférico | Configuración | Para qué |
|---|---|---|
| GPIO | `PD12`–`PD15` salidas (labels `LED_VERDE/NARANJA/ROJO/AZUL`), `PA2` salida (`TRIG`), `PE3` salida que arranca en alto (`ACCEL_CS`) | LEDs, trigger del sensor, chip select del acelerómetro |
| TIM2 | Prescaler `16-1` → cuenta a 1 MHz (**1 cuenta = 1 µs**), período al máximo (32 bits), canal 2 en Input Capture, interrupción habilitada | La librería `HC_SR04` mide el ancho del pulso de Echo en µs |
| ADC1 | 12 bits, canal 11 (`PC1`), muestreo de 480 ciclos, disparo por software | Lectura del LM35 |
| SPI1 | Master full-duplex, 1 MHz, CPOL alto / CPHA 2.º flanco (modo 3) | Acelerómetro LIS3DSH |

La cuenta del TIM2 es la misma idea del lab 2 pero a otra escala: 16 MHz / 16 = 1 MHz → el contador avanza una vez por microsegundo, que es la unidad en la que el HC-SR04 "habla" (58,3 µs de eco por cm de distancia).

## Compilar y grabar desde VS Code

Idéntico al lab 2: abrir **la carpeta `firmware/`** en VS Code, conectar la Discovery por el USB mini (el del ST-LINK, arriba), y usar las tareas:

- **Elegir desafío y configurar** (1 a 4) → **Compilar** (`Ctrl+Shift+B`) → **Grabar en la placa**.

O por terminal, parado en `firmware/`:

```
cmake --preset Debug -DDESAFIO=2
cmake --build build/Debug
openocd -f interface/stlink.cfg -f target/stm32f4x.cfg -c "program build/Debug/firmware.elf verify reset exit"
```

Para el desafío 2 hace falta el modo debug (mirar temperaturas en **Live Expressions**): F5 → *Debug en la placa (OpenOCD)*, y en la pestaña de watch agregar `temp_bloqueante` y `temp_no_bloqueante`.

> **Ojo con el modelo de placa:** si la Discovery que te prestan es muy vieja (dice "STM32F4DISCOVERY" sin "-DISC1"), puede traer el acelerómetro LIS302DL en vez del LIS3DSH. Si el desafío 3 no responde a la inclinación, avisá — se cambian dos registros y listo.
