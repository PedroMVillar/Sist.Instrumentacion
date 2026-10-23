/**
 * lm35.h — Librería propia para el sensor de temperatura LM35 (desafío 2).
 *
 * Esta librería NO configura el hardware a bajo nivel (de los pines,
 * relojes y el ADC se encarga el código generado por CubeMX): solo
 * vincula recursos del micro (un ADC y un canal) a instancias lógicas
 * del sensor, para poder manejar varios LM35 a la vez.
 */

#ifndef LM35_H
#define LM35_H

#include "stm32f4xx_hal.h"
#include <stdbool.h>

/* Una instancia lógica de sensor: qué ADC usa y en qué canal está */
typedef struct {
    ADC_HandleTypeDef *hadc;  /* puntero al ADC (ej: &hadc1) */
    uint32_t           canal; /* canal del ADC (ej: ADC_CHANNEL_11) */
} LM35_HandleTypeDef;

/* Asocia los recursos de hardware (ADC y canal) a la instancia */
void LM35_Init(LM35_HandleTypeDef *hlm35, ADC_HandleTypeDef *hadc, uint32_t canal);

/* --- Fase A: servicio bloqueante ----------------------------------------
   Dispara la conversión, espera activamente a que termine y devuelve
   la temperatura en grados Celsius. */
float LM35_Read_Blocking(LM35_HandleTypeDef *hlm35);

/* --- Fase B: servicios no bloqueantes ------------------------------------
   Dispara la lectura en el hardware y vuelve enseguida, sin esperar. */
void LM35_Start_Conversion(LM35_HandleTypeDef *hlm35);

/* Consulta si la conversión terminó. Si hay dato nuevo lo convierte a
   grados, lo deja en *out_temp y devuelve true. Si el ADC todavía está
   convirtiendo, devuelve false inmediatamente. */
bool LM35_Get_Temperature(LM35_HandleTypeDef *hlm35, float *out_temp);

#endif /* LM35_H */
