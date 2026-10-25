/**
 * Desafío 1 — Memoria de estado y antirrebote (bloqueante)
 *
 * Al presionar el botón una vez, el LED rojo cambia de estado (toggle)
 * y se mantiene así. El rebote del pulsador se filtra esperando 50 ms
 * con HAL_Delay y confirmando que el botón sigue presionado.
 *
 * Ojo: en la Nucleo el botón es activo en BAJO (presionado se lee 0).
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

  while (1)
  {
    /* ¿Botón presionado? (activo en bajo: presionado = 0) */
    if (HAL_GPIO_ReadPin(BOTON_GPIO_Port, BOTON_Pin) == GPIO_PIN_RESET)
    {
      HAL_Delay(50); /* Antirrebote bloqueante */

      /* Si después de 50 ms sigue presionado, era una presión de verdad */
      if (HAL_GPIO_ReadPin(BOTON_GPIO_Port, BOTON_Pin) == GPIO_PIN_RESET)
      {
        HAL_GPIO_TogglePin(LED_ROJO_GPIO_Port, LED_ROJO_Pin);

        /* Esperar a que se suelte el botón para evitar múltiples toggles */
        while (HAL_GPIO_ReadPin(BOTON_GPIO_Port, BOTON_Pin) == GPIO_PIN_RESET);
      }
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
