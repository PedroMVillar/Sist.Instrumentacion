/*
 * Balancin (Ball and Beam) segun el PDF "Control PID - Ball and Beam".
 * HC-SR04 mide la posicion de la pelota; un servo inclina la viga.
 * El PID (pid.c) es el del PDF. Este archivo agrega el hardware:
 * LeerSensorDistancia() y ActualizarServo(), como pide la seccion 4.
 *
 * Valores editables en Live Expressions mientras corre:
 *   modo (0 = solo medir, servo a 0 grados; 1 = PID),
 *   setpoint_cm (posicion pedida respecto del centro; para ensayos de escalon),
 *   centro_viga_cm,
 *   pid_bola.kp / pid_bola.ki / pid_bola.kd, servo_centro_us, sentido.
 *
 * Grafico por UART (agregado): cada ENVIO_MS el TIM4 interrumpe y manda por
 * USART2 (PA2 = TX, 115200) una linea "tiempo_ms,posicion_cm". Del lado de
 * la PC, grafico/grafico.py la dibuja en tiempo real.
 */
#include "main.h"
#include "gpio.h"
#include "tim.h"
#include "usart.h"
#include "HC_SR04.h"
#include "pid.h"
#include <stdbool.h>
#include <stdio.h>

/* Ts: el PDF recomienda 20 a 50 ms. Se usa 50 ms porque el HC-SR04 necesita
 * tiempo entre disparos (los ecos viejos rebotan) y cada medida usa el periodo. */
#define TS_MS                50U
#define LIMITE_GRADOS        35.0f /* +/- 35 grados. En nuestra maqueta la viga se inclina poco:
                                      cerca del medio la pelota recien rueda con ~19 grados (2026-11-20). */
#define US_POR_GRADO         11.1f /* servo tipico: 1000 us para 90 grados */
#define FALLAS_PARA_CENTRAR  5U
#define FILTRO_N             5U    /* mediana de las ultimas 5 lecturas (250 ms) */
#define SERVO_PERIODO_MS     20U   /* el servo recibe un pulso cada 20 ms (50 Hz) */
#define ENVIO_MS             50U   /* grafico: una muestra cada 50 ms (igual que Ts,
                                      mas rapido repetiria el mismo dato del sensor) */
#define LOG_EXTENDIDO        1     /* 0: "tiempo_ms,posicion_cm"
                                      1: "tiempo_ms,posicion_cm,setpoint_cm,salida_grados"
                                         (para identificar el lazo con los ensayos de escalon) */
/* ---------- Ajustes (editables en Live Expressions) ---------- */
volatile uint32_t modo = 1;              /* 0 = medir, 1 = controlar */
volatile float setpoint_cm = 0.0f;       /* posicion pedida respecto del centro de la viga (0 = centro) */
volatile float centro_viga_cm = 27.3f;   /* medio real de la viga: la pelota marca 3.7 cm contra el sensor y 50.8 cm en la otra punta (2026-11-24) */
volatile float distancia_max_cm = 55.0f; /* punta lejana medida: 50.5 cm */
volatile int32_t servo_centro_us = 1592; /* opcion A del modelo, SIN PROBAR en la maqueta (antes 1525). Medido antes: viga horizontal (2026-11-24): la pelota quieta recien se aleja del sensor
                                            con 1350 o menos y recien va hacia el sensor con 1700 o mas; 1525 es el
                                            medio, asi los +/-35 grados quedan parejos para los dos lados */
volatile float sentido = -1.0f;          /* -1 comprobado en esta maqueta */
volatile float velocidad_servo = 60.0f;  /* grados/s del servo; 0 = de golpe; menos = mas lento. Con 150 el ruido del sensor
                                            hacia que el servo diera golpecitos que sacaban la pelota del centro */
/* ---------- Valores para mirar ---------- */
volatile float distance_cm = 0.0f;   /* ultima lectura cruda del sensor */
volatile float posicion_cm = 0.0f;   /* posicion respecto del centro (filtrada) */
volatile float distancia_filtrada_cm = 0.0f; /* mediana de las ultimas FILTRO_N lecturas */
volatile float salida_angulo = 0.0f; /* lo que pide el PID, en grados */
volatile int32_t servo_us = 0;     /* ultimo pulso enviado al servo */
volatile float angulo_servo = 0.0f;  /* angulo donde esta el servo ahora */
volatile uint32_t lecturas_distancia = 0, errores_distancia = 0;
volatile uint32_t fallas_seguidas = FALLAS_PARA_CENTRAR; /* al inicio no hay pelota */

/* Global (static) solo para poder ver y cambiar kp/ki/kd desde el depurador. */
static PID_Controller pid_bola;

TIM_HandleTypeDef htim2;
static HCSR04_HandleTypeDef distance_sensor;
static bool medicion_en_curso = false;
static float filtro_buffer[FILTRO_N];
static uint32_t filtro_indice = 0U;
static uint32_t filtro_cantidad = 0U;
static float angulo_objetivo = 0.0f;
static float velocidad_objetivo = 0.0f;
static uint32_t ultimo_servo_ms = 0U;
void SystemClock_Config(void);

/* ---------- Grafico por UART: TIM4 interrumpe cada ENVIO_MS ---------- */
/* TIM4 recibe 48 MHz; prescaler 48000-1 -> 1 ms por cuenta. */
static TIM_HandleTypeDef htim4;
static char linea_grafico[64];

static void Grafico_Init(void)
{
    __HAL_RCC_TIM4_CLK_ENABLE();

    htim4.Instance = TIM4;
    htim4.Init.Prescaler = 48000 - 1;
    htim4.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim4.Init.Period = ENVIO_MS - 1;
    htim4.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim4.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    if (HAL_TIM_Base_Init(&htim4) != HAL_OK) Error_Handler();

    /* Prioridad 1: menos urgente que el eco del sensor (TIM3, prioridad 0) */
    HAL_NVIC_SetPriority(TIM4_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(TIM4_IRQn);
    HAL_TIM_Base_Start_IT(&htim4);
}

void TIM4_IRQHandler(void)
{
    HAL_TIM_IRQHandler(&htim4);
}

/* La HAL la llama cuando TIM4 llega al final de su periodo (cada ENVIO_MS). */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM4)
    {
#if LOG_EXTENDIDO
        int largo = snprintf(linea_grafico, sizeof(linea_grafico), "%lu,%.2f,%.2f,%.2f\r\n",
                             (unsigned long)HAL_GetTick(), posicion_cm, setpoint_cm, salida_angulo);
#else
        int largo = snprintf(linea_grafico, sizeof(linea_grafico), "%lu,%.2f\r\n",
                             (unsigned long)HAL_GetTick(), posicion_cm);
#endif
        /* _IT: no espera a que salga la linea, la UART la manda sola por
         * interrupcion. Si la anterior todavia no termino, esta se saltea. */
        HAL_UART_Transmit_IT(&huart2, (uint8_t *)linea_grafico, (uint16_t)largo);
    }
}

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
    oc.Pulse = servo_centro_us; /* arranca con la viga horizontal, sin salto */
    oc.OCPolarity = TIM_OCPOLARITY_HIGH;
    oc.OCFastMode = TIM_OCFAST_DISABLE;
    if (HAL_TIM_PWM_ConfigChannel(&htim2, &oc, TIM_CHANNEL_2) != HAL_OK) Error_Handler();
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_2);
}

/* Escribe ya mismo un angulo en grados (0 = viga horizontal) como pulso. */
static void Servo_Escribir(float angulo)
{
    int32_t pulso = servo_centro_us + (int32_t)(sentido * angulo * US_POR_GRADO);
    if (pulso < 700) pulso = 700;    /* rango seguro de un servo chico */
    if (pulso > 2300) pulso = 2300;
    servo_us = pulso;
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, (uint32_t)pulso);
}

/* Pide al servo ir a "angulo" (grados) moviendose a "velocidad" (grados por
 * segundo), como pidio el profesor. No espera: Servo_Update() lo va acercando
 * cada 20 ms. velocidad <= 0 significa ir de golpe. */
void Servo_Set(float angulo, float velocidad)
{
    if (angulo > pid_bola.limite_out_max) angulo = pid_bola.limite_out_max;
    if (angulo < pid_bola.limite_out_min) angulo = pid_bola.limite_out_min;
    angulo_objetivo = angulo;
    velocidad_objetivo = velocidad;
}

/* Se llama en cada vuelta del while. Cada 20 ms (un periodo del PWM)
 * avanza el angulo como maximo velocidad * tiempo transcurrido. */
static void Servo_Update(uint32_t ahora)
{
    if ((ahora - ultimo_servo_ms) < SERVO_PERIODO_MS) return;
    float dt = (ahora - ultimo_servo_ms) / 1000.0f;
    ultimo_servo_ms = ahora;

    float diferencia = angulo_objetivo - angulo_servo;
    float paso = velocidad_objetivo * dt;
    if (velocidad_objetivo <= 0.0f || (diferencia <= paso && diferencia >= -paso))
        angulo_servo = angulo_objetivo;          /* llega en este paso */
    else if (diferencia > 0.0f)
        angulo_servo += paso;
    else
        angulo_servo -= paso;

    Servo_Escribir(angulo_servo);
}

/* La funcion del PDF: mueve el servo a la velocidad elegida en velocidad_servo. */
void ActualizarServo(float angulo)
{
    Servo_Set(angulo, velocidad_servo);
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
            /* Filtro: mediana de las ultimas FILTRO_N lecturas validas.
             * El sensor da picos falsos (7% de las lecturas se alejan mas de 3 cm);
             * la mediana los descarta, un promedio los reparte.
             * Si la pelota recien aparece, el filtro empieza de cero. */
            if (fallas_seguidas >= FALLAS_PARA_CENTRAR) filtro_cantidad = 0U;
            filtro_buffer[filtro_indice] = distance_cm;
            filtro_indice = (filtro_indice + 1U) % FILTRO_N;
            if (filtro_cantidad < FILTRO_N) filtro_cantidad++;
            float orden[FILTRO_N];
            for (uint32_t i = 0; i < filtro_cantidad; i++)
            {
                /* las ultimas filtro_cantidad lecturas, ordenadas (insercion) */
                float valor = filtro_buffer[(filtro_indice + FILTRO_N - 1U - i) % FILTRO_N];
                uint32_t j = i;
                while (j > 0U && orden[j - 1U] > valor)
                {
                    orden[j] = orden[j - 1U];
                    j--;
                }
                orden[j] = valor;
            }
            if (filtro_cantidad % 2U == 1U)
                distancia_filtrada_cm = orden[filtro_cantidad / 2U];
            else
                distancia_filtrada_cm = 0.5f * (orden[filtro_cantidad / 2U - 1U] + orden[filtro_cantidad / 2U]);
            posicion_cm = distancia_filtrada_cm - centro_viga_cm;
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
    bool centrada = !sin_pelota && posicion_cm <= 1.0f && posicion_cm >= -1.0f; /* +/- 1 cm */

    HAL_GPIO_WritePin(GPIOD, LD5_Pin, sin_pelota ? GPIO_PIN_SET : GPIO_PIN_RESET); /* rojo */
    HAL_GPIO_WritePin(GPIOD, LD4_Pin, centrada ? GPIO_PIN_SET : GPIO_PIN_RESET);   /* verde */
    HAL_GPIO_WritePin(GPIOD, LD6_Pin, modo == 1U ? GPIO_PIN_SET : GPIO_PIN_RESET); /* azul */
}

int main(void)
{
    HAL_Init();
    SystemClock_Config();

    MX_GPIO_Init();
    MX_TIM3_Init();
    MX_USART2_UART_Init(); /* grafico: UART en PA2/PA3 */
    Servo_Init();
    Servo_Escribir(0.0f);
    ultimo_servo_ms = HAL_GetTick();

    distance_sensor.htim = &htim3;
    distance_sensor.IC_Channel = TIM_CHANNEL_4;
    distance_sensor.Trig_Port = GPIOB;
    distance_sensor.Trig_Pin = GPIO_PIN_0;
    hcsr04_init(&distance_sensor);

    /* Opcion A del modelo (horizontal 1592): Kp=4, Ki=0.1, Kd=0.5. Salio de la
     * simulacion y FALTA PROBARLA en la maqueta.
     * Opcion B (horizontal 1525): Kp=4, Ki=0.8, Kd=0.5, tampoco probada.
     * Anterior (horizontal 1525): Kp=3, Ki=0.2, Kd=1.5.
     * El PDF usa Kp=1.5, Ki=0.2, Kd=0.05. Limites +/- LIMITE_GRADOS. Ts = 0.05 s = TS_MS. */
    PID_Init(&pid_bola, 4.0f, 0.1f, 0.5f, TS_MS / 1000.0f, LIMITE_GRADOS, -LIMITE_GRADOS);

    Grafico_Init();        /* grafico: desde aca TIM4 manda una muestra cada ENVIO_MS */

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
                salida_angulo = PID_Compute(&pid_bola, setpoint_cm, posicion_actual);
            }
            else
            {
                /* Midiendo, o sin ver la pelota: viga horizontal y PID reiniciado. */
                salida_angulo = 0.0f;
                PID_Reset(&pid_bola, posicion_actual);
            }
            ActualizarServo(salida_angulo);
        }

        Servo_Update(tiempo_actual);
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

/* USER CODE BEGIN 4 */
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
