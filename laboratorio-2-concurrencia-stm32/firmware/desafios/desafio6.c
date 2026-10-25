/**
 * Desafío 6 — Antirrebote profesional (muestreo en ISR)
 *
 * El TIM4 interrumpe cada 10 ms y en su callback se muestrea el botón.
 * Si se lo lee presionado 5 veces seguidas (50 ms de estabilidad
 * garantizada), la ISR levanta una bandera. El super-loop solo mira
 * esa bandera: si está levantada, invierte el LED azul y la limpia.
 *
 * La bandera es `volatile`: le avisa al compilador que su valor puede
 * cambiar en cualquier momento (la modifica la interrupción), así no
 * la "optimiza" asumiendo que siempre vale lo mismo.
 */

#include "main.h"
#include "tim.h"
#include "gpio.h"

void SystemClock_Config(void);

volatile uint8_t bandera_boton = 0; /* obligatorio el uso de volatile */

int main(void)
{
  /* Inicialización generada por CubeMX: HAL, reloj y periféricos */
  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();
  MX_TIM2_Init();
  MX_TIM3_Init();
  MX_TIM4_Init();

  /* Arrancar el TIM4 en modo interrupción (desborda cada 10 ms) */
  HAL_TIM_Base_Start_IT(&htim4);

  while (1)
  {
    /* El super-loop quedó liberado de toda lógica de retardo:
       solo mira la bandera que levanta la interrupción. */
    if (bandera_boton)
    {
      bandera_boton = 0;
      HAL_GPIO_TogglePin(LED_AZUL_GPIO_Port, LED_AZUL_Pin);
    }
  }
}

/* Callback del TIM4: se ejecuta cada 10 ms y muestrea el botón */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  if (htim->Instance == TIM4)
  {
    static uint8_t contador = 0; /* lecturas seguidas en "presionado" */

    /* Presionado = 0 (activo en bajo en la Nucleo) */
    if (HAL_GPIO_ReadPin(BOTON_GPIO_Port, BOTON_Pin) == GPIO_PIN_RESET)
    {
      contador++;
      if (contador == 5) /* 5 lecturas × 10 ms = 50 ms estable */
      {
        bandera_boton = 1; /* notificar al super-loop */
      }
    }
    else
    {
      contador = 0; /* ruido o botón suelto: arrancar de nuevo */
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
