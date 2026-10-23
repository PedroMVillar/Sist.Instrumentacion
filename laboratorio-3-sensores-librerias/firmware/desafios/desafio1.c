/**
 * Desafío 1 — HC-SR04, librerías de terceros y actuadores
 *
 * Radar de proximidad usando la librería HC_SR04 provista por la cátedra:
 *   - distancia > 50 cm        → LED verde   (zona segura)
 *   - distancia entre 20 y 50  → LED naranja (zona de precaución)
 *   - distancia < 20 cm        → LED rojo    (zona de peligro)
 *
 * La librería mide con el TIM2 en Input Capture: el timer cuenta en µs
 * y captura los flancos del pin Echo; el ancho del pulso en µs dividido
 * 58,3 da la distancia en cm.
 */

#include "main.h"
#include "adc.h"
#include "spi.h"
#include "tim.h"
#include "gpio.h"
#include "HC_SR04.h"

void SystemClock_Config(void);

HCSR04_HandleTypeDef sensor_distancia;

int main(void)
{
  /* Inicialización generada por CubeMX: HAL, reloj y periféricos */
  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();
  MX_ADC1_Init();
  MX_SPI1_Init();
  MX_TIM2_Init();

  /* Asignar los recursos de hardware a la instancia del sensor
     (como pide la librería) y arrancar el timer */
  sensor_distancia.htim       = &htim2;
  sensor_distancia.IC_Channel = TIM_CHANNEL_2;
  sensor_distancia.Trig_Port  = TRIG_GPIO_Port;
  sensor_distancia.Trig_Pin   = TRIG_Pin;
  hcsr04_init(&sensor_distancia);

  while (1)
  {
    /* Disparar una medición (no bloqueante: vuelve enseguida) */
    hcsr04_start_measurement(&sensor_distancia);

    /* Darle tiempo al eco: como mucho tarda ~38 ms en volver */
    HAL_Delay(100);

    if (hcsr04_is_data_ready(&sensor_distancia))
    {
      float distancia = hcsr04_get_distance_cm(&sensor_distancia);

      /* Apagar todo y encender solo el LED de la zona actual */
      HAL_GPIO_WritePin(LED_VERDE_GPIO_Port, LED_VERDE_Pin, GPIO_PIN_RESET);
      HAL_GPIO_WritePin(LED_NARANJA_GPIO_Port, LED_NARANJA_Pin, GPIO_PIN_RESET);
      HAL_GPIO_WritePin(LED_ROJO_GPIO_Port, LED_ROJO_Pin, GPIO_PIN_RESET);

      if (distancia > 50.0f)
      {
        HAL_GPIO_WritePin(LED_VERDE_GPIO_Port, LED_VERDE_Pin, GPIO_PIN_SET);
      }
      else if (distancia >= 20.0f)
      {
        HAL_GPIO_WritePin(LED_NARANJA_GPIO_Port, LED_NARANJA_Pin, GPIO_PIN_SET);
      }
      else
      {
        HAL_GPIO_WritePin(LED_ROJO_GPIO_Port, LED_ROJO_Pin, GPIO_PIN_SET);
      }
    }
  }
}

/* La HAL llama a esto en cada captura del timer; se lo pasamos a la
   librería, que es quien sabe qué hacer con los flancos del Echo. */
void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
  hcsr04_tim_ic_callback(&sensor_distancia, htim);
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
