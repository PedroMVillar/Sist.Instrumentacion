/**
 * Desafío 4 — El super-loop y la integración total
 *
 * Los tres sensores conviven en un super-loop disparado por tiempo
 * (Time-Triggered Super-loop). Prohibido HAL_Delay: cada tarea compara
 * contra HAL_GetTick() y decide si le toca correr en esta vuelta.
 *
 *   Tarea 1 — Acelerómetro: se lee en CADA vuelta → LEDs direccionales.
 *   Tarea 2 — Temperatura (lm35, fase B): un valor nuevo cada 1000 ms,
 *             lanzando y recogiendo la conversión sin esperar.
 *   Tarea 3 — Distancia (HC_SR04): medición cada 100 ms.
 *   Tarea 4 — Alarma: obstáculo a < 15 cm → el LED rojo deja de obedecer
 *             al acelerómetro y parpadea a 5 Hz hasta que se despeje.
 *
 * La temperatura y la distancia se miran en Live Expressions
 * (variables `temperatura` y `distancia`).
 */

#include "main.h"
#include "adc.h"
#include "spi.h"
#include "tim.h"
#include "gpio.h"
#include "HC_SR04.h"
#include "lm35.h"

/* Registros del LIS3DSH que usamos (igual que en el desafío 3) */
#define LIS3DSH_REG_CTRL4   0x20
#define LIS3DSH_REG_OUT_X_H 0x29
#define LIS3DSH_REG_OUT_Y_H 0x2B
#define UMBRAL_INCLINACION  16

#define DISTANCIA_ALARMA_CM 15.0f

void SystemClock_Config(void);

HCSR04_HandleTypeDef sensor_distancia;
LM35_HandleTypeDef   sensor_temperatura;

/* Globales y volatile para poder mirarlas en Live Expressions */
volatile float temperatura = 0.0f;
volatile float distancia = 999.0f; /* arranca "lejos" hasta la primera medición */

/* --- Acceso al acelerómetro por SPI (igual que en el desafío 3) ---------- */

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

/* LEDs direccionales según inclinación. Cuando hay alarma, el LED rojo
   es de la alarma y esta función no lo toca. */
static void ActualizarLedsInclinacion(int8_t x, int8_t y, uint8_t alarma)
{
  if (!alarma)
  {
    if (x > UMBRAL_INCLINACION)
    {
      HAL_GPIO_WritePin(LED_ROJO_GPIO_Port, LED_ROJO_Pin, GPIO_PIN_SET);
    }
    else
    {
      HAL_GPIO_WritePin(LED_ROJO_GPIO_Port, LED_ROJO_Pin, GPIO_PIN_RESET);
    }
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

  /* Sensores */
  accel_init();

  LM35_Init(&sensor_temperatura, &hadc1, ADC_CHANNEL_11);

  sensor_distancia.htim       = &htim2;
  sensor_distancia.IC_Channel = TIM_CHANNEL_2;
  sensor_distancia.Trig_Port  = TRIG_GPIO_Port;
  sensor_distancia.Trig_Pin   = TRIG_Pin;
  hcsr04_init(&sensor_distancia);

  /* Última vez que corrió cada tarea temporal */
  uint32_t ultimo_temperatura = 0;
  uint32_t ultimo_distancia = 0;
  uint32_t ultimo_parpadeo = 0;

  uint8_t conversion_en_curso = 0;
  uint8_t alarma = 0;

  while (1)
  {
    uint32_t ahora = HAL_GetTick();

    /* --- Tarea 1: acelerómetro, en cada vuelta ------------------------- */
    int8_t x = (int8_t)accel_leer_registro(LIS3DSH_REG_OUT_X_H);
    int8_t y = (int8_t)accel_leer_registro(LIS3DSH_REG_OUT_Y_H);
    ActualizarLedsInclinacion(x, y, alarma);

    /* --- Tarea 2: temperatura cada 1000 ms, no bloqueante -------------- */
    if (ahora - ultimo_temperatura >= 1000)
    {
      ultimo_temperatura = ahora;
      LM35_Start_Conversion(&sensor_temperatura); /* lanza y no espera */
      conversion_en_curso = 1;
    }
    if (conversion_en_curso)
    {
      float temp_nueva;
      if (LM35_Get_Temperature(&sensor_temperatura, &temp_nueva))
      {
        temperatura = temp_nueva; /* valor consolidado del segundo */
        conversion_en_curso = 0;
      }
    }

    /* --- Tarea 3: distancia cada 100 ms -------------------------------- */
    if (ahora - ultimo_distancia >= 100)
    {
      ultimo_distancia = ahora;

      /* Recoger el resultado de la medición anterior (si llegó)... */
      if (hcsr04_is_data_ready(&sensor_distancia))
      {
        distancia = hcsr04_get_distance_cm(&sensor_distancia);
      }
      /* ...y disparar la siguiente */
      hcsr04_start_measurement(&sensor_distancia);
    }

    /* --- Tarea 4: alarma por obstáculo a menos de 15 cm ----------------- */
    alarma = (distancia < DISTANCIA_ALARMA_CM);

    if (alarma && ahora - ultimo_parpadeo >= 100) /* 5 Hz: cambia cada 100 ms */
    {
      ultimo_parpadeo = ahora;
      HAL_GPIO_TogglePin(LED_ROJO_GPIO_Port, LED_ROJO_Pin);
    }
  }
}

/* La HAL llama a esto en cada captura del timer; se lo pasamos a la
   librería del HC-SR04, que procesa los flancos del Echo. */
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
