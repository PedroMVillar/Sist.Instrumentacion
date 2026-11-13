/**
 * Laboratorio 4 — Control PID de un ball and beam
 *
 * La bola rueda sobre la viga, el HC-SR04 mide dónde está y un servo
 * inclina la viga. Cada TS_MS el PID calcula cuántos grados inclinar
 * para llevar la bola al centro.
 *
 * Todo se ajusta desde Live Expressions, sin recompilar:
 *   modo             0 = solo medir (viga horizontal), 1 = controlar
 *   centro_viga_cm   distancia del sensor al centro (se mide en modo 0)
 *   sentido          poner -1 si la viga corrige para el lado equivocado
 *   servo_centro_us  pulso con el que la viga queda horizontal
 *   pid_bola.kp/ki/kd  ganancias (procedimiento en explicacion.md)
 *
 * LEDs: rojo = no ve la bola, verde = bola centrada, azul = controlando.
 */

#include "main.h"
#include "tim.h"
#include "gpio.h"
#include "HC_SR04.h"
#include "pid.h"

/* Ts de 50 ms: la guía pide entre 20 y 50, y el HC-SR04 necesita aire
   entre disparos para que no se mezclen ecos viejos. */
#define TS_MS               50
#define ANGULO_MAX          15.0f /* inclinación máxima, como el ejemplo de la guía */
#define US_POR_GRADO        11.1f /* servo típico: 1000 µs para 90 grados */
#define FALLAS_PARA_CENTRAR 5     /* 5 ecos inválidos seguidos = no hay bola */

/* --- Ajustes, editables en Live Expressions ------------------------------ */
volatile uint32_t modo = 0;              /* 0 = solo medir, 1 = controlar */
volatile float centro_viga_cm = 28.5f;   /* medir con la bola en el centro */
volatile float distancia_max_cm = 55.0f; /* más lejos que esto = eco inválido */
volatile int32_t servo_centro_us = 1500; /* pulso de viga horizontal */
volatile float sentido = 1.0f;           /* -1 invierte el servo */
volatile float setpoint_cm = 0.0f;       /* objetivo respecto del centro */

/* --- Valores para mirar --------------------------------------------------- */
volatile float distancia_cm = 0.0f;  /* última lectura cruda del sensor */
volatile float posicion_cm = 0.0f;   /* posición respecto del centro, filtrada */
volatile float salida_angulo = 0.0f; /* lo que pide el PID, en grados */
volatile int32_t servo_us = 1500;    /* pulso que está saliendo al servo */
volatile uint32_t lecturas = 0;
volatile uint32_t errores = 0;
volatile uint32_t fallas_seguidas = FALLAS_PARA_CENTRAR; /* al inicio no hay bola */

HCSR04_HandleTypeDef sensor_distancia;
PID_Controller pid_bola;
uint8_t medicion_en_curso = 0;

void SystemClock_Config(void);

/* --- Capa de hardware: sensor y actuador ---------------------------------
   El PID no sabe qué sensor ni qué servo hay: solo ve estas dos funciones. */

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
  int32_t pulso = servo_centro_us + (int32_t)(sentido * angulo * US_POR_GRADO);
  if (pulso < 1000) pulso = 1000; /* rango físico del servo */
  if (pulso > 2000) pulso = 2000;

  servo_us = pulso;
  __HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_1, (uint32_t)pulso);
}

/* Se llama una vez por Ts: recoge la medición lanzada la vuelta anterior
   y dispara la siguiente, sin esperar nada. Devuelve la posición respecto
   del centro de la viga, en cm, filtrada. */
static float LeerSensorDistancia(void)
{
  if (medicion_en_curso && hcsr04_is_data_ready(&sensor_distancia))
  {
    distancia_cm = hcsr04_get_distance_cm(&sensor_distancia);

    if (distancia_cm >= 2.0f && distancia_cm <= distancia_max_cm)
    {
      float nueva = distancia_cm - centro_viga_cm;
      if (fallas_seguidas >= FALLAS_PARA_CENTRAR)
      {
        posicion_cm = nueva; /* la bola recién aparece: sin filtro */
      }
      else
      {
        posicion_cm = 0.5f * posicion_cm + 0.5f * nueva; /* pasabajos simple */
      }
      lecturas++;
      fallas_seguidas = 0;
    }
    else
    {
      errores++; /* eco fuera de rango: lectura descartada */
      if (fallas_seguidas < FALLAS_PARA_CENTRAR) fallas_seguidas++;
    }
  }
  else if (medicion_en_curso)
  {
    /* No llegó el eco en todo un Ts: cancelar la captura colgada */
    HAL_TIM_IC_Stop_IT(sensor_distancia.htim, sensor_distancia.IC_Channel);
    errores++;
    if (fallas_seguidas < FALLAS_PARA_CENTRAR) fallas_seguidas++;
  }

  hcsr04_start_measurement(&sensor_distancia);
  medicion_en_curso = 1;

  return posicion_cm;
}

static void ActualizarLedsEstado(void)
{
  uint8_t sin_bola = (fallas_seguidas >= FALLAS_PARA_CENTRAR);
  uint8_t centrada = (!sin_bola && posicion_cm > -2.0f && posicion_cm < 2.0f);

  HAL_GPIO_WritePin(LED_ROJO_GPIO_Port, LED_ROJO_Pin,
                    sin_bola ? GPIO_PIN_SET : GPIO_PIN_RESET);
  HAL_GPIO_WritePin(LED_VERDE_GPIO_Port, LED_VERDE_Pin,
                    centrada ? GPIO_PIN_SET : GPIO_PIN_RESET);
  HAL_GPIO_WritePin(LED_AZUL_GPIO_Port, LED_AZUL_Pin,
                    (modo == 1) ? GPIO_PIN_SET : GPIO_PIN_RESET);
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

  /* Ganancias iniciales de la guía; se afinan en vivo (explicacion.md) */
  PID_Init(&pid_bola, 1.5f, 0.2f, 0.05f, TS_MS / 1000.0f,
           ANGULO_MAX, -ANGULO_MAX);

  uint32_t tiempo_anterior = HAL_GetTick();

  while (1)
  {
    uint32_t tiempo_actual = HAL_GetTick();

    /* Lazo de control no bloqueante: corre una vez cada TS_MS */
    if (tiempo_actual - tiempo_anterior >= TS_MS)
    {
      tiempo_anterior = tiempo_actual;

      float posicion_actual = LeerSensorDistancia();

      if (modo == 1 && fallas_seguidas < FALLAS_PARA_CENTRAR)
      {
        salida_angulo = PID_Compute(&pid_bola, setpoint_cm, posicion_actual);
      }
      else
      {
        /* Solo midiendo, o sin ver la bola: viga horizontal y PID
           reiniciado para que no arrastre historia vieja */
        salida_angulo = 0.0f;
        pid_bola.integral = 0.0f;
        pid_bola.error_previo = setpoint_cm - posicion_actual;
      }

      ActualizarServo(salida_angulo);
      ActualizarLedsEstado();
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
