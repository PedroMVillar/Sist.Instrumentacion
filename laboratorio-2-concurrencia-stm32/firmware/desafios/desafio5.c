/**
 * Desafío 5 — Temporización en segundo plano (interrupciones)
 *
 * El TIM3 genera una interrupción exacta cada 500 ms y en su rutina de
 * atención (ISR) se invierte el LED naranja → parpadea a 1 Hz.
 *
 * Para demostrar que la interrupción es independiente del programa
 * principal, el super-loop está bloqueado con un HAL_Delay(5000): el
 * LED verde cambia recién cada 5 segundos (se nota que el main está
 * "ocupado"), pero el naranja mantiene su ritmo perfecto igual.
 */

#include "main.h"
#include "tim.h"
#include "gpio.h"

void SystemClock_Config(void);

int main(void)
{
  /* Inicialización generada por CubeMX: HAL, reloj y periféricos */
  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();
  MX_TIM2_Init();
  MX_TIM3_Init();
  MX_TIM4_Init();

  /* Arrancar el TIM3 en modo interrupción (desborda cada 500 ms) */
  HAL_TIM_Base_Start_IT(&htim3);

  while (1)
  {
    /* Retardo masivo: el super-loop queda bloqueado 5 segundos.
       El LED verde muestra lo lento que avanza el main. */
    HAL_GPIO_TogglePin(LED_VERDE_GPIO_Port, LED_VERDE_Pin);
    HAL_Delay(5000);
  }
}

/* Callback de los timers: el hardware interrumpe al main y ejecuta esto
   solo, cada vez que el TIM3 desborda (cada 500 ms exactos). */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  if (htim->Instance == TIM3)
  {
    HAL_GPIO_TogglePin(LED_NARANJA_GPIO_Port, LED_NARANJA_Pin);
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
