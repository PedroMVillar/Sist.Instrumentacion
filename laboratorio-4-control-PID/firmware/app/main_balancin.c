/*
 * Balancin (Ball and Beam) segun el PDF "Control PID - Ball and Beam".
 * HC-SR04 mide la posicion de la pelota; un servo inclina la viga.
 * El PID (pid.c) es el del PDF. Este archivo agrega el hardware:
 * LeerSensorDistancia() y ActualizarServo(), como pide la seccion 4.
 *
 * Todo se ajusta por la consola serie (USART2, 115200, por el USB del
 * ST-LINK). La placa manda una linea de telemetria por cada Ts y acepta
 * comandos "nombre valor", por ejemplo:
 *   modo 1      (0 = solo medir, 1 = PID, 2 = prueba de sentido)
 *   kp 3        ki 0.2       kd 0.005
 *   centro 28.5     sentido -1     servo 1200     max 55
 */
#include "main.h"
#include "gpio.h"
#include "tim.h"
#include "usart.h"
#include "HC_SR04.h"
#include "pid.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* Ts: el PDF recomienda 20 a 50 ms. Se usa 50 ms porque el HC-SR04 necesita
 * tiempo entre disparos (los ecos viejos rebotan) y cada medida usa el periodo. */
#define TS_MS                50U
#define LIMITE_GRADOS        15.0f /* +/- 15 grados, como el ejemplo del PDF */
#define US_POR_GRADO         11.1f /* servo tipico: 1000 us para 90 grados */
#define FALLAS_PARA_CENTRAR  5U

/* ---------- Ajustes (editables por la consola) ---------- */
volatile uint32_t modo = 0;              /* 0 = medir, 1 = controlar, 2 = probar sentido */
volatile float centro_viga_cm = 28.5f;   /* medio de la viga, medido con la pelota */
volatile float distancia_max_cm = 55.0f; /* punta lejana medida: 50.5 cm */
volatile int32_t servo_centro_us = 1200; /* pulso con la viga horizontal (medido aprox.) */
volatile float sentido = -1.0f;          /* -1 comprobado con modo 2 en esta maqueta */

/* ---------- Valores para mirar ---------- */
volatile float distance_cm = 0.0f;   /* ultima lectura cruda del sensor */
volatile float posicion_cm = 0.0f;   /* posicion respecto del centro (filtrada) */
volatile float salida_angulo = 0.0f; /* lo que pide el PID, en grados */
volatile int32_t servo_us = 1700;
volatile uint32_t lecturas_distancia = 0, errores_distancia = 0;
volatile uint32_t fallas_seguidas = FALLAS_PARA_CENTRAR; /* al inicio no hay pelota */

/* Global (static) para poder cambiar kp/ki/kd desde la consola. */
static PID_Controller pid_bola;

TIM_HandleTypeDef htim2;
static HCSR04_HandleTypeDef distance_sensor;
static bool medicion_en_curso = false;

void SystemClock_Config(void);

/* ---------- Servo: TIM2 canal 2 en PA1, 50 Hz ---------- */
/* TIM2 recibe 48 MHz; prescaler 47 -> 1 us por cuenta; periodo 20000 us. */
static void Servo_Init(void)
{
    GPIO_InitTypeDef gpio = {0};
    TIM_OC_InitTypeDef oc = {0};

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_TIM2_CLK_ENABLE();

    gpio.Pin = GPIO_PIN_1;
    gpio.Mode = GPIO_MODE_AF_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    gpio.Alternate = GPIO_AF1_TIM2;
    HAL_GPIO_Init(GPIOA, &gpio);

    htim2.Instance = TIM2;
    htim2.Init.Prescaler = 47;
    htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim2.Init.Period = 19999;
    htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
    if (HAL_TIM_PWM_Init(&htim2) != HAL_OK) Error_Handler();

    oc.OCMode = TIM_OCMODE_PWM1;
    oc.Pulse = 1500;
    oc.OCPolarity = TIM_OCPOLARITY_HIGH;
    oc.OCFastMode = TIM_OCFAST_DISABLE;
    if (HAL_TIM_PWM_ConfigChannel(&htim2, &oc, TIM_CHANNEL_2) != HAL_OK) Error_Handler();
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_2);
}

/* Angulo en grados (0 = viga horizontal) -> pulso del servo. */
void ActualizarServo(float angulo)
{
    if (angulo > LIMITE_GRADOS) angulo = LIMITE_GRADOS;
    if (angulo < -LIMITE_GRADOS) angulo = -LIMITE_GRADOS;

    int32_t pulso = servo_centro_us + (int32_t)(sentido * angulo * US_POR_GRADO);
    if (pulso < 700) pulso = 700;    /* rango seguro de un servo chico */
    if (pulso > 2300) pulso = 2300;
    servo_us = pulso;
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, (uint32_t)pulso);
}

/* Se llama una vez por Ts. Toma el resultado de la medicion lanzada en la
 * llamada anterior y lanza la siguiente; no espera nada (no bloqueante).
 * Devuelve la posicion respecto del centro de la viga, en cm, filtrada. */
float LeerSensorDistancia(void)
{
    if (medicion_en_curso && hcsr04_is_data_ready(&distance_sensor))
    {
        distance_cm = hcsr04_get_distance_cm(&distance_sensor);
        if (distance_cm >= 2.0f && distance_cm <= distancia_max_cm)
        {
            float nueva = distance_cm - centro_viga_cm;
            if (fallas_seguidas >= FALLAS_PARA_CENTRAR)
                posicion_cm = nueva;                  /* recien aparece: sin filtro */
            else
                posicion_cm = 0.5f * posicion_cm + 0.5f * nueva; /* pasabajos simple */
            lecturas_distancia++;
            fallas_seguidas = 0U;
        }
        else
        {
            errores_distancia++;
            if (fallas_seguidas < FALLAS_PARA_CENTRAR) fallas_seguidas++;
        }
    }
    else if (medicion_en_curso)
    {
        /* No llego el eco en todo un Ts: cancelar la captura. */
        HAL_TIM_IC_Stop_IT(distance_sensor.htim, distance_sensor.IC_Channel);
        errores_distancia++;
        if (fallas_seguidas < FALLAS_PARA_CENTRAR) fallas_seguidas++;
    }

    hcsr04_start_measurement(&distance_sensor);
    medicion_en_curso = true;
    return posicion_cm;
}

static void Update_Leds(void)
{
    bool sin_pelota = fallas_seguidas >= FALLAS_PARA_CENTRAR;
    bool centrada = !sin_pelota && posicion_cm < 2.0f && posicion_cm > -2.0f;

    HAL_GPIO_WritePin(GPIOD, LD5_Pin, sin_pelota ? GPIO_PIN_SET : GPIO_PIN_RESET); /* rojo */
    HAL_GPIO_WritePin(GPIOD, LD4_Pin, centrada ? GPIO_PIN_SET : GPIO_PIN_RESET);   /* verde */
    HAL_GPIO_WritePin(GPIOD, LD6_Pin, modo == 1U ? GPIO_PIN_SET : GPIO_PIN_RESET); /* azul */
}

/* ---------- Consola serie (USART2 por el USB del ST-LINK) ----------
 * Telemetria: una linea CSV por Ts -> "ms,modo,distancia,posicion,angulo,servo".
 * Comandos: una linea "nombre valor" terminada en Enter. */

static uint8_t rx_byte;
static char linea_comando[32];
static volatile uint8_t largo_comando = 0;
static volatile bool comando_listo = false;

static void EnviarTexto(const char *texto)
{
    HAL_UART_Transmit(&huart2, (const uint8_t *)texto, (uint16_t)strlen(texto), 10);
}

static void EnviarTelemetria(uint32_t tiempo_ms)
{
    char texto[64];
    snprintf(texto, sizeof(texto), "%lu,%lu,%.1f,%.2f,%.2f,%ld\r\n",
             (unsigned long)tiempo_ms, (unsigned long)modo, distance_cm,
             posicion_cm, salida_angulo, (long)servo_us);
    EnviarTexto(texto);
}

static void ProcesarComando(char *linea)
{
    char respuesta[48];
    char *espacio = strchr(linea, ' ');
    if (espacio == NULL)
    {
        EnviarTexto("falta el valor (ej: kp 3)\r\n");
        return;
    }
    *espacio = '\0';
    float valor = strtof(espacio + 1, NULL);

    if      (strcmp(linea, "modo") == 0)    modo = (uint32_t)valor;
    else if (strcmp(linea, "kp") == 0)      pid_bola.kp = valor;
    else if (strcmp(linea, "ki") == 0)      pid_bola.ki = valor;
    else if (strcmp(linea, "kd") == 0)      pid_bola.kd = valor;
    else if (strcmp(linea, "centro") == 0)  centro_viga_cm = valor;
    else if (strcmp(linea, "sentido") == 0) sentido = valor;
    else if (strcmp(linea, "servo") == 0)   servo_centro_us = (int32_t)valor;
    else if (strcmp(linea, "max") == 0)     distancia_max_cm = valor;
    else
    {
        snprintf(respuesta, sizeof(respuesta), "no entiendo: %s\r\n", linea);
        EnviarTexto(respuesta);
        return;
    }
    snprintf(respuesta, sizeof(respuesta), "ok %s = %.3f\r\n", linea, valor);
    EnviarTexto(respuesta);
}

/* La HAL llama a esto por cada byte recibido: junta la linea y avisa al loop. */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART2)
    {
        if (rx_byte == '\n' || rx_byte == '\r')
        {
            if (largo_comando > 0 && !comando_listo)
            {
                linea_comando[largo_comando] = '\0';
                comando_listo = true;
            }
        }
        else if (largo_comando < sizeof(linea_comando) - 1 && !comando_listo)
        {
            linea_comando[largo_comando++] = (char)rx_byte;
        }
        HAL_UART_Receive_IT(&huart2, &rx_byte, 1);
    }
}
/* ---------- fin consola ---------- */

int main(void)
{
    HAL_Init();
    SystemClock_Config();

    MX_GPIO_Init();
    MX_TIM3_Init();
    MX_USART2_UART_Init();
    Servo_Init();
    ActualizarServo(0.0f);

    distance_sensor.htim = &htim3;
    distance_sensor.IC_Channel = TIM_CHANNEL_4;
    distance_sensor.Trig_Port = GPIOB;
    distance_sensor.Trig_Pin = GPIO_PIN_0;
    hcsr04_init(&distance_sensor);

    /* Ganancias ajustadas en la maqueta: Kp=3, Ki=0.2, Kd=0.005 (el PDF usa
     * Kp=1.5, Ki=0.2, Kd=0.05). Limites +/- 15 grados.
     * Ts = 0.05 s para coincidir con TS_MS. */
    PID_Init(&pid_bola, 3.0f, 0.2f, 0.005f, TS_MS / 1000.0f, LIMITE_GRADOS, -LIMITE_GRADOS);

    HAL_UART_Receive_IT(&huart2, &rx_byte, 1); /* consola: arrancar la recepcion */

    float setpoint_centro = 0.0f; /* Centro de la viga */
    uint32_t tiempo_anterior = HAL_GetTick();

    while (1)
    {
        uint32_t tiempo_actual = HAL_GetTick();

        /* Ejecucion determinista no bloqueante */
        if ((tiempo_actual - tiempo_anterior) >= TS_MS)
        {
            tiempo_anterior = tiempo_actual;

            float posicion_actual = LeerSensorDistancia();

            if (modo == 1U && fallas_seguidas < FALLAS_PARA_CENTRAR)
            {
                salida_angulo = PID_Compute(&pid_bola, setpoint_centro, posicion_actual);
            }
            else if (modo == 2U)
            {
                /* Prueba de sentido: +10 grados fijos es lo que el PID pide cuando
                 * la pelota esta cerca del sensor. La pelota debe rodar ALEJANDOSE
                 * del sensor. Si rueda hacia el sensor, poner sentido = -1. */
                salida_angulo = 10.0f;
                pid_bola.integral = 0.0f;
                pid_bola.error_previo = setpoint_centro - posicion_actual;
            }
            else
            {
                /* Midiendo, o sin ver la pelota: viga horizontal y PID reiniciado. */
                salida_angulo = 0.0f;
                pid_bola.integral = 0.0f;
                pid_bola.error_previo = setpoint_centro - posicion_actual;
            }
            ActualizarServo(salida_angulo);

            EnviarTelemetria(tiempo_actual); /* consola: una linea por Ts */
        }

        /* consola: procesar el comando juntado por la interrupcion */
        if (comando_listo)
        {
            ProcesarComando(linea_comando);
            largo_comando = 0;
            comando_listo = false;
        }

        Update_Leds();
    }
}

void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
    hcsr04_tim_ic_callback(&distance_sensor, htim);
}

void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 192;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 8;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3) != HAL_OK)
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

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
}
#endif /* USE_FULL_ASSERT */
