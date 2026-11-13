#include "pid.h"

/* El algoritmo es el de la guía (sección 4). Agregados propios, marcados:
   integral en cero mientras ki = 0, y la saturación final de la salida,
   que la guía no muestra (la página corta justo ahí). */

void PID_Init(PID_Controller *pid, float kp, float ki, float kd,
              float ts, float max, float min)
{
    pid->kp = kp;
    pid->ki = ki;
    pid->kd = kd;
    pid->ts = ts;
    pid->error_previo = 0.0f;
    pid->integral = 0.0f;
    pid->limite_out_max = max;
    pid->limite_out_min = min;
}

float PID_Compute(PID_Controller *pid, float setpoint, float medida_actual)
{
    float error = setpoint - medida_actual;

    /* 1. Acción proporcional: reacciona al error de AHORA */
    float p_term = pid->kp * error;

    /* 2. Acción integral: acumula el error del PASADO
       (aproximación rectangular: error por tiempo transcurrido) */
    pid->integral += error * pid->ts;

    /* Agregado: con ki = 0 (paso 1 de Ziegler-Nichols) no acumular, así
       al subir ki en vivo no aparece de golpe una integral gigante. */
    if (pid->ki == 0.0f)
    {
        pid->integral = 0.0f;
    }
    float i_term = pid->ki * pid->integral;

    /* Anti-windup: si la integral pide más de lo que el servo puede dar,
       se recorta el acumulador para que no crezca sin sentido */
    if (i_term > pid->limite_out_max)
    {
        i_term = pid->limite_out_max;
        pid->integral = i_term / pid->ki;
    }
    else if (i_term < pid->limite_out_min)
    {
        i_term = pid->limite_out_min;
        pid->integral = i_term / pid->ki;
    }

    /* 3. Acción derivativa: mira cómo cambia el error (el FUTURO)
       y frena antes de que la bola se pase de largo */
    float d_term = pid->kd * ((error - pid->error_previo) / pid->ts);
    pid->error_previo = error;

    /* 4. Salida total */
    float salida = p_term + i_term + d_term;

    /* Agregado: limitar al rango del actuador */
    if (salida > pid->limite_out_max)
    {
        salida = pid->limite_out_max;
    }
    else if (salida < pid->limite_out_min)
    {
        salida = pid->limite_out_min;
    }

    return salida;
}
