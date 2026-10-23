/**
 * Desafío 2 — LM35, modularización y soporte multi-instancia
 *
 * Prueba de la librería propia lm35 (libs/lm35.h y libs/lm35.c):
 * se crean DOS instancias lógicas del sensor sobre el mismo canal
 * físico (vale según la consigna: valida la lógica multi-instancia
 * del software) y se las lee con las dos APIs:
 *   - Fase A: LM35_Read_Blocking (espera activa)
 *   - Fase B: LM35_Start_Conversion + LM35_Get_Temperature (no bloqueante)
 *
 * Verificación: correr en modo Debug (F5) y mirar las variables
 * temp_bloqueante y temp_no_bloqueante en Live Expressions.
 * El LED verde parpadea como señal de que el programa corre.
 */

#include "main.h"
#include "adc.h"
#include "spi.h"
#include "tim.h"
#include "gpio.h"
#include "lm35.h"

void SystemClock_Config(void);

LM35_HandleTypeDef lm35_1; /* instancia 1: se lee bloqueante */
LM35_HandleTypeDef lm35_2; /* instancia 2: se lee no bloqueante */

/* Globales y volatile para poder mirarlas en Live Expressions */
volatile float temp_bloqueante = 0.0f;
volatile float temp_no_bloqueante = 0.0f;

int main(void)
{
  /* Inicialización generada por CubeMX: HAL, reloj y periféricos */
  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();
  MX_ADC1_Init();
  MX_SPI1_Init();
  MX_TIM2_Init();

  /* Dos instancias lógicas sobre el mismo ADC y el mismo canal (PC1) */
  LM35_Init(&lm35_1, &hadc1, ADC_CHANNEL_11);
  LM35_Init(&lm35_2, &hadc1, ADC_CHANNEL_11);

  while (1)
  {
    /* Fase A: lectura bloqueante de la instancia 1 */
    temp_bloqueante = LM35_Read_Blocking(&lm35_1);

    /* Fase B: lectura no bloqueante de la instancia 2.
       Acá esperamos con un while para probarla aislada; en el
       desafío 4 se usa sin esperar, dentro del super-loop. */
    LM35_Start_Conversion(&lm35_2);

    float temperatura;
    while (!LM35_Get_Temperature(&lm35_2, &temperatura))
    {
      /* mientras da false, el ADC sigue convirtiendo:
         en un programa real acá se haría otro trabajo */
    }
    temp_no_bloqueante = temperatura;

    /* Latido: señal visible de que el loop está corriendo */
    HAL_GPIO_TogglePin(LED_VERDE_GPIO_Port, LED_VERDE_Pin);
    HAL_Delay(500);
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
