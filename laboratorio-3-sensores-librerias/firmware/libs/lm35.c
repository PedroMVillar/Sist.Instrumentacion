#include "lm35.h"

/* VDDA de la F4Discovery: 3.0 V. El ADC de 12 bits devuelve 0..4095. */
#define LM35_VREF_MV    3000.0f
#define LM35_ADC_MAXIMO 4095.0f

/* El LM35 entrega 10 mV por cada grado Celsius */
static float lm35_cuentas_a_grados(uint32_t cuentas)
{
    float milivolts = (float)cuentas * LM35_VREF_MV / LM35_ADC_MAXIMO;
    return milivolts / 10.0f;
}

/* Apunta el secuenciador del ADC al canal de ESTA instancia.
   Es lo que permite que varias instancias compartan el mismo ADC:
   antes de cada conversión, cada una configura su propio canal. */
static void lm35_configurar_canal(LM35_HandleTypeDef *hlm35)
{
    ADC_ChannelConfTypeDef config = {0};

    config.Channel      = hlm35->canal;
    config.Rank         = 1;
    config.SamplingTime = ADC_SAMPLETIME_480CYCLES;

    HAL_ADC_ConfigChannel(hlm35->hadc, &config);
}

void LM35_Init(LM35_HandleTypeDef *hlm35, ADC_HandleTypeDef *hadc, uint32_t canal)
{
    hlm35->hadc  = hadc;
    hlm35->canal = canal;
}

float LM35_Read_Blocking(LM35_HandleTypeDef *hlm35)
{
    lm35_configurar_canal(hlm35);

    HAL_ADC_Start(hlm35->hadc);
    HAL_ADC_PollForConversion(hlm35->hadc, 100); /* espera activa a que termine */
    uint32_t cuentas = HAL_ADC_GetValue(hlm35->hadc);
    HAL_ADC_Stop(hlm35->hadc);

    return lm35_cuentas_a_grados(cuentas);
}

void LM35_Start_Conversion(LM35_HandleTypeDef *hlm35)
{
    lm35_configurar_canal(hlm35);
    HAL_ADC_Start(hlm35->hadc); /* dispara y vuelve enseguida */
}

bool LM35_Get_Temperature(LM35_HandleTypeDef *hlm35, float *out_temp)
{
    /* Timeout 0: mira el flag de fin de conversión y vuelve al toque */
    if (HAL_ADC_PollForConversion(hlm35->hadc, 0) != HAL_OK)
    {
        return false; /* todavía está convirtiendo */
    }

    *out_temp = lm35_cuentas_a_grados(HAL_ADC_GetValue(hlm35->hadc));
    HAL_ADC_Stop(hlm35->hadc);
    return true;
}
