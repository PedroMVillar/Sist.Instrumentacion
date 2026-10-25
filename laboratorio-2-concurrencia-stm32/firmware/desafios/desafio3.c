/**
 * Desafío 3 — Máquina de estados finitos (integración)
 *
 * Tres estados que rotan con cada presión del botón:
 *   Estado 0: todos los LEDs apagados.
 *   Estado 1: todos parpadean lento (1 Hz → cambian cada 500 ms).
 *   Estado 2: todos parpadean rápido (5 Hz → cambian cada 100 ms).
 *
 * Sin HAL_Delay en el super-loop (salvo un antirrebote corto en la
 * transición, permitido por la consigna). El botón se detecta por
 * FLANCO: recién presionado = ahora lee 0 y antes leía 1.
 */

#include "main.h"
#include "tim.h"
#include "gpio.h"

void SystemClock_Config(void);

/* Apaga los cuatro LEDs */
static void ApagarTodosLosLeds(void)
{
  HAL_GPIO_WritePin(LED_VERDE_GPIO_Port, LED_VERDE_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(LED_NARANJA_GPIO_Port, LED_NARANJA_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(LED_ROJO_GPIO_Port, LED_ROJO_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(LED_AZUL_GPIO_Port, LED_AZUL_Pin, GPIO_PIN_RESET);
}

/* Invierte los cuatro LEDs (prendidos ↔ apagados) */
static void InvertirTodosLosLeds(void)
{
  HAL_GPIO_TogglePin(LED_VERDE_GPIO_Port, LED_VERDE_Pin);
  HAL_GPIO_TogglePin(LED_NARANJA_GPIO_Port, LED_NARANJA_Pin);
  HAL_GPIO_TogglePin(LED_ROJO_GPIO_Port, LED_ROJO_Pin);
  HAL_GPIO_TogglePin(LED_AZUL_GPIO_Port, LED_AZUL_Pin);
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

  uint8_t estado = 0;                         /* 0 = apagado, 1 = lento, 2 = rápido */
  GPIO_PinState botonAnterior = GPIO_PIN_SET; /* arranca suelto (suelto = 1) */
  uint32_t millisAnterior = 0;
  uint32_t periodo = 0;                       /* ms entre cambios de los LEDs */

  while (1)
  {
    /* Flanco de presión: ahora presionado (0) y antes suelto (1) */
    GPIO_PinState botonActual = HAL_GPIO_ReadPin(BOTON_GPIO_Port, BOTON_Pin);
    if (botonActual == GPIO_PIN_RESET && botonAnterior == GPIO_PIN_SET)
    {
      HAL_Delay(20);            /* antirrebote corto, solo en la transición */
      estado = (estado + 1) % 3; /* 0 → 1 → 2 → 0 → ... */
      ApagarTodosLosLeds();      /* cada estado arranca con todo apagado */
    }
    botonAnterior = botonActual;

    /* Período de parpadeo según el estado */
    if (estado == 1)
    {
      periodo = 500; /* 1 Hz */
    }
    if (estado == 2)
    {
      periodo = 100; /* 5 Hz */
    }

    /* En estado 1 o 2, parpadear sin bloquear (igual que en el desafío 2) */
    if (estado != 0 && HAL_GetTick() - millisAnterior >= periodo)
    {
      millisAnterior = HAL_GetTick();
      InvertirTodosLosLeds();
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
