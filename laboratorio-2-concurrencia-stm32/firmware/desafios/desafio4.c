/**
 * Desafío 4 — Precisión por hardware (timer por polling)
 *
 * Efecto "Coche Fantástico": los LEDs se encienden en secuencia de ida
 * y vuelta (verde → naranja → rojo → azul → rojo → naranja → ...).
 *
 * El tiempo NO lo cuenta el procesador: lo cuenta el TIM2, configurado
 * en CubeMX para desbordar cada 100 ms. El super-loop solo mira por
 * polling si se levantó la bandera de desborde, la limpia y avanza.
 */

#include "main.h"
#include "tim.h"
#include "gpio.h"

void SystemClock_Config(void);

static uint8_t paso = 0; /* posición actual en la secuencia (0 a 5) */

/* Apaga los cuatro LEDs */
static void ApagarTodosLosLeds(void)
{
  HAL_GPIO_WritePin(LED_VERDE_GPIO_Port, LED_VERDE_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(LED_NARANJA_GPIO_Port, LED_NARANJA_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(LED_ROJO_GPIO_Port, LED_ROJO_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(LED_AZUL_GPIO_Port, LED_AZUL_Pin, GPIO_PIN_RESET);
}

/* Enciende el LED que toca y avanza un paso.
   Secuencia de ida y vuelta, 6 pasos que se repiten:
     paso:  0       1        2      3      4      5
     LED:   verde   naranja  rojo   azul   rojo   naranja        */
static void AvanzarSecuenciaLEDs(void)
{
  ApagarTodosLosLeds();

  if (paso == 0)
  {
    HAL_GPIO_WritePin(LED_VERDE_GPIO_Port, LED_VERDE_Pin, GPIO_PIN_SET);
  }
  if (paso == 1 || paso == 5)
  {
    HAL_GPIO_WritePin(LED_NARANJA_GPIO_Port, LED_NARANJA_Pin, GPIO_PIN_SET);
  }
  if (paso == 2 || paso == 4)
  {
    HAL_GPIO_WritePin(LED_ROJO_GPIO_Port, LED_ROJO_Pin, GPIO_PIN_SET);
  }
  if (paso == 3)
  {
    HAL_GPIO_WritePin(LED_AZUL_GPIO_Port, LED_AZUL_Pin, GPIO_PIN_SET);
  }

  paso = (paso + 1) % 6;
}

int main(void)
{
  /* Inicialización generada por CubeMX: HAL, reloj y periféricos */
  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();
  MX_TIM2_Init();
  MX_TIM3_Init();
  MX_TIM4_Init();

  /* Arrancar el TIM2 (desborda cada 100 ms, configurado en CubeMX) */
  HAL_TIM_Base_Start(&htim2);

  while (1)
  {
    /* Polling: ¿el TIM2 levantó su bandera de desborde? */
    if (__HAL_TIM_GET_FLAG(&htim2, TIM_FLAG_UPDATE) != RESET)
    {
      __HAL_TIM_CLEAR_FLAG(&htim2, TIM_FLAG_UPDATE);
      AvanzarSecuenciaLEDs();
    }
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
