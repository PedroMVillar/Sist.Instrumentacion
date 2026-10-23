/**
 * Desafío 3 — El acelerómetro y el problema del bloqueo
 *
 * El acelerómetro LIS3DSH (soldado en la placa, por SPI1) indica con
 * los LEDs hacia dónde está inclinada la placa. Además hay un "latido"
 * (heartbeat): el LED azul parpadea cada 500 ms.
 *
 * El experimento está en el #define de abajo:
 *   HEARTBEAT_BLOQUEANTE 1 → el latido usa HAL_Delay(500): el loop queda
 *       frenado medio segundo por vuelta y los LEDs de inclinación
 *       responden a los tirones. (El problema)
 *   HEARTBEAT_BLOQUEANTE 0 → el latido usa HAL_GetTick() sin frenar nada:
 *       la inclinación responde fluida y el azul parpadea igual. (La solución)
 *
 * Cambiar el valor, recompilar, grabar y comparar.
 */

#include "main.h"
#include "adc.h"
#include "spi.h"
#include "tim.h"
#include "gpio.h"

#define HEARTBEAT_BLOQUEANTE 1 /* 1 = HAL_Delay (problema), 0 = HAL_GetTick (solución) */

/* Registros del LIS3DSH que usamos */
#define LIS3DSH_REG_CTRL4   0x20
#define LIS3DSH_REG_OUT_X_H 0x29
#define LIS3DSH_REG_OUT_Y_H 0x2B

/* Con ±2 g de fondo de escala, el byte alto da ±128 por g.
   Umbral de 16 ≈ 0,25 g: suficiente inclinación para encender un LED. */
#define UMBRAL_INCLINACION 16

void SystemClock_Config(void);

/* --- Acceso al acelerómetro por SPI --------------------------------------
   Protocolo del chip: CS en bajo, mandar la dirección del registro
   (con el bit 7 en 1 si es lectura) y después el dato. CS en alto. */

static void accel_escribir_registro(uint8_t registro, uint8_t valor)
{
  uint8_t datos[2] = {registro, valor};

  HAL_GPIO_WritePin(ACCEL_CS_GPIO_Port, ACCEL_CS_Pin, GPIO_PIN_RESET);
  HAL_SPI_Transmit(&hspi1, datos, 2, 100);
  HAL_GPIO_WritePin(ACCEL_CS_GPIO_Port, ACCEL_CS_Pin, GPIO_PIN_SET);
}

static uint8_t accel_leer_registro(uint8_t registro)
{
  uint8_t direccion = registro | 0x80; /* bit 7 en 1 = lectura */
  uint8_t valor = 0;

  HAL_GPIO_WritePin(ACCEL_CS_GPIO_Port, ACCEL_CS_Pin, GPIO_PIN_RESET);
  HAL_SPI_Transmit(&hspi1, &direccion, 1, 100);
  HAL_SPI_Receive(&hspi1, &valor, 1, 100);
  HAL_GPIO_WritePin(ACCEL_CS_GPIO_Port, ACCEL_CS_Pin, GPIO_PIN_SET);

  return valor;
}

static void accel_init(void)
{
  /* CTRL4 = 0x67: muestreo a 100 Hz y ejes X, Y, Z habilitados */
  accel_escribir_registro(LIS3DSH_REG_CTRL4, 0x67);
}

/* Enciende el LED del lado hacia el que está inclinada la placa.
   (Si en la práctica queda espejado, dar vuelta los signos.) */
static void ActualizarLedsInclinacion(int8_t x, int8_t y)
{
  if (x > UMBRAL_INCLINACION)
  {
    HAL_GPIO_WritePin(LED_ROJO_GPIO_Port, LED_ROJO_Pin, GPIO_PIN_SET);
  }
  else
  {
    HAL_GPIO_WritePin(LED_ROJO_GPIO_Port, LED_ROJO_Pin, GPIO_PIN_RESET);
  }

  if (x < -UMBRAL_INCLINACION)
  {
    HAL_GPIO_WritePin(LED_VERDE_GPIO_Port, LED_VERDE_Pin, GPIO_PIN_SET);
  }
  else
  {
    HAL_GPIO_WritePin(LED_VERDE_GPIO_Port, LED_VERDE_Pin, GPIO_PIN_RESET);
  }

  if (y > UMBRAL_INCLINACION)
  {
    HAL_GPIO_WritePin(LED_NARANJA_GPIO_Port, LED_NARANJA_Pin, GPIO_PIN_SET);
  }
  else
  {
    HAL_GPIO_WritePin(LED_NARANJA_GPIO_Port, LED_NARANJA_Pin, GPIO_PIN_RESET);
  }

  if (y < -UMBRAL_INCLINACION)
  {
    HAL_GPIO_WritePin(LED_AZUL_GPIO_Port, LED_AZUL_Pin, GPIO_PIN_SET);
  }
  else
  {
    HAL_GPIO_WritePin(LED_AZUL_GPIO_Port, LED_AZUL_Pin, GPIO_PIN_RESET);
  }
}

int main(void)
{
  /* Inicialización generada por CubeMX: HAL, reloj y periféricos */
  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();
  MX_ADC1_Init();
  MX_SPI1_Init();
  MX_TIM2_Init();

  accel_init();

#if !HEARTBEAT_BLOQUEANTE
  uint32_t millisAnterior = 0;
#endif

  while (1)
  {
    /* Leer la inclinación (byte alto de X e Y, con signo) */
    int8_t x = (int8_t)accel_leer_registro(LIS3DSH_REG_OUT_X_H);
    int8_t y = (int8_t)accel_leer_registro(LIS3DSH_REG_OUT_Y_H);

    ActualizarLedsInclinacion(x, y);

#if HEARTBEAT_BLOQUEANTE
    /* EL PROBLEMA: este delay frena TODO el loop medio segundo.
       La placa responde a la inclinación solo 2 veces por segundo. */
    HAL_GPIO_TogglePin(LED_AZUL_GPIO_Port, LED_AZUL_Pin);
    HAL_Delay(500);
#else
    /* LA SOLUCIÓN: mismo parpadeo, pero preguntando la hora en vez
       de frenar. El loop gira libre y la inclinación responde fluida.
       (Nota: acá el azul es del heartbeat, así que la inclinación en Y
       negativa se nota menos — es el mismo LED compartido.) */
    if (HAL_GetTick() - millisAnterior >= 500)
    {
      millisAnterior = HAL_GetTick();
      HAL_GPIO_TogglePin(LED_AZUL_GPIO_Port, LED_AZUL_Pin);
    }
#endif
  }
}

/* ---- Código generado por CubeMX (igual que en Core/Src/main.c) ---------- */

void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

void Error_Handler(void)
{
  __disable_irq();
  while (1)
  {
  }
}
