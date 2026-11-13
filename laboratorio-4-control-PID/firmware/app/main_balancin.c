/**
 * Laboratorio 4 — Control PID de un ball and beam
 *
 * La bola rueda sobre la viga, el HC-SR04 mide dónde está y un servo
 * inclina la viga. Cada TS_MS el PID calcula cuántos grados inclinar
 * para llevar la bola al setpoint.
 *
 * Sintonización: correr en Debug y mirar en Live Expressions las
 * variables posicion, salida_angulo, setpoint y la estructura pid_bola
 * (las ganancias se pueden editar en vivo). El procedimiento completo
 * está en explicacion.md.
 */

#include "main.h"
#include "tim.h"
#include "gpio.h"
#include "HC_SR04.h"
#include "pid.h"

/* --- Parámetros de la maqueta (ajustar al armarla) ----------------------- */
#define SETPOINT_CM        20.0f /* posición objetivo: el centro de la viga */
#define TS_MS              40    /* período del lazo (25 Hz, la guía pide 20 a 50 ms) */
#define ANGULO_MAX         15.0f /* inclinación máxima de la viga, en grados */

/* Ganancias iniciales (se afinan con Ziegler-Nichols, ver explicacion.md) */
#define KP_INICIAL         1.5f
#define KI_INICIAL         0.0f
#define KD_INICIAL         0.0f

/* Servo: 1500 µs = viga horizontal. US_POR_GRADO pasa de grados a µs de
   pulso; si la viga se inclina al revés, cambiarle el signo. */
#define SERVO_US_CENTRO    1500
#define SERVO_US_POR_GRADO 10.0f

void SystemClock_Config(void);

HCSR04_HandleTypeDef sensor_distancia;
PID_Controller pid_bola;

/* Globales y volatile para mirar (y editar) en Live Expressions */
volatile float posicion = 0.0f;
volatile float salida_angulo = 0.0f;
volatile float setpoint = SETPOINT_CM;

/* --- Capa de hardware: sensor y actuador ---------------------------------
   El PID no sabe qué sensor ni qué servo hay: solo ve estas dos funciones. */

static float LeerSensorDistancia(void)
{
  /* Recoger la medición que se disparó en la vuelta anterior (si llegó) */
  if (hcsr04_is_data_ready(&sensor_distancia))
  {
    posicion = hcsr04_get_distance_cm(&sensor_distancia);
  }

  /* Disparar la siguiente, frenando antes la captura (ver lab 3) */
  HAL_TIM_IC_Stop_IT(sensor_distancia.htim, sensor_distancia.IC_Channel);
  hcsr04_start_measurement(&sensor_distancia);

  return posicion; /* si no hubo eco nuevo, se usa la última posición */
}

static void ActualizarServo(float angulo)
{
  if (angulo > ANGULO_MAX)
  {
    angulo = ANGULO_MAX;
  }
  else if (angulo < -ANGULO_MAX)
  {
    angulo = -ANGULO_MAX;
  }

  /* El timer cuenta en µs: el ancho del pulso va directo al comparador */
  uint32_t pulso_us = SERVO_US_CENTRO + (int32_t)(angulo * SERVO_US_POR_GRADO);
  __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_1, pulso_us);
}

int main(void)
{
  /* Inicialización generada por CubeMX: HAL, reloj y periféricos */
  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();
  MX_TIM2_Init();
  MX_TIM4_Init();

  /* HC-SR04 igual que en el laboratorio 3 */
  sensor_distancia.htim       = &htim2;
  sensor_distancia.IC_Channel = TIM_CHANNEL_2;
  sensor_distancia.Trig_Port  = TRIG_GPIO_Port;
  sensor_distancia.Trig_Pin   = TRIG_Pin;
  hcsr04_init(&sensor_distancia);

  /* Arrancar el PWM del servo con la viga horizontal */
  HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_1);
  ActualizarServo(0.0f);

  PID_Init(&pid_bola, KP_INICIAL, KI_INICIAL, KD_INICIAL,
           TS_MS / 1000.0f, ANGULO_MAX, -ANGULO_MAX);

  uint32_t tiempo_anterior = HAL_GetTick();

  while (1)
  {
    uint32_t tiempo_actual = HAL_GetTick();

    /* Lazo de control no bloqueante: corre una vez cada TS_MS */
    if (tiempo_actual - tiempo_anterior >= TS_MS)
    {
      tiempo_anterior = tiempo_actual;

      float posicion_actual = LeerSensorDistancia();
      salida_angulo = PID_Compute(&pid_bola, setpoint, posicion_actual);
      ActualizarServo(salida_angulo);
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
