/**
 * Desafío 2 — Liberando la CPU (no bloqueante con SysTick)
 *
 * El LED azul parpadea a 2 Hz (250 ms prendido, 250 ms apagado) y,
 * al mismo tiempo, el LED verde se enciende solo mientras el botón
 * esté presionado.
 *
 * La clave: NO se usa HAL_Delay. El parpadeo se planifica comparando
 * contra HAL_GetTick() (el contador de milisegundos del sistema), así
 * el super-loop nunca se detiene y el botón responde al instante.
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

  uint32_t millisAnterior = 0;

  while (1)
  {
    /* Parpadeo del LED azul: ¿ya pasaron 250 ms desde el último cambio? */
    uint32_t millisActual = HAL_GetTick();
    if (millisActual - millisAnterior >= 250)
    {
      millisAnterior = millisActual;
      HAL_GPIO_TogglePin(LED_AZUL_GPIO_Port, LED_AZUL_Pin);
    }

    /* El botón se evalúa libremente en cada vuelta del loop.
       Verde encendido solo mientras esté presionado (presionado = 0). */
    if (HAL_GPIO_ReadPin(BOTON_GPIO_Port, BOTON_Pin) == GPIO_PIN_RESET)
    {
      HAL_GPIO_WritePin(LED_VERDE_GPIO_Port, LED_VERDE_Pin, GPIO_PIN_SET);
    }
    else
    {
      HAL_GPIO_WritePin(LED_VERDE_GPIO_Port, LED_VERDE_Pin, GPIO_PIN_RESET);
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
