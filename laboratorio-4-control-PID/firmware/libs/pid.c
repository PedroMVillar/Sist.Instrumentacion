#include "pid.h"

/* Copiado del PDF "Control PID - Ball and Beam", seccion 4.
 * Agregados propios (marcados): integral en cero mientras ki = 0,
 * anti-windup condicional (no integrar con la salida al tope), y
 * saturacion final + return, que el PDF no muestra (la pagina 3 corta ahi). */

void PID_Init(PID_Controller *pid, float kp, float ki, float kd,
              float ts, float max, float min) {
    pid->kp = kp;
    pid->ki = ki;
    pid->kd = kd;
    pid->ts = ts;
    pid->error_previo = 0.0f;
    pid->integral = 0.0f;
    pid->limite_out_max = max;
    pid->limite_out_min = min;
}

float PID_Compute(PID_Controller *pid, float setpoint, float medida_actual) {
    float error = setpoint - medida_actual;

    /* 1. Accion Proporcional */
    float p_term = pid->kp * error;

    /* 3. Accion Derivativa (se calcula antes para saber si la salida satura) */
    float d_term = pid->kd * ((error - pid->error_previo) / pid->ts);
    pid->error_previo = error;

    /* 2. Accion Integral (Aproximacion rectangular) */
    /* Cambio respecto del PDF (anti-windup condicional): si la salida ya esta
     * al tope y el error empuja hacia ese mismo tope, no se acumula. Sin esto,
     * con la pelota un rato en una punta la integral se llena y la salida queda
     * clavada en +/-15 grados mucho despues de que la pelota volvio. */
    float salida_previa = p_term + d_term + pid->ki * pid->integral;
    int satura_arriba = salida_previa >= pid->limite_out_max && error > 0.0f;
    int satura_abajo = salida_previa <= pid->limite_out_min && error < 0.0f;
    if (!satura_arriba && !satura_abajo) {
        pid->integral += error * pid->ts;
    }
    /* Agregado: con ki = 0 (Ziegler-Nichols paso 1) no acumular, para que al
     * subir ki en vivo no aparezca de golpe una integral enorme. */
    if (pid->ki == 0.0f) {
        pid->integral = 0.0f;
    }
    float i_term = pid->ki * pid->integral;

    /* Anti-Windup: Saturacion dinamica del acumulador integral */
    if (i_term > pid->limite_out_max) {
        i_term = pid->limite_out_max;
        pid->integral = i_term / pid->ki;
    } else if (i_term < pid->limite_out_min) {
        i_term = pid->limite_out_min;
        pid->integral = i_term / pid->ki;
    }

    /* 4. Calculo de la senal de control total */
    float salida = p_term + i_term + d_term;

    /* Agregado (el PDF no lo muestra): limitar al rango del actuador. */
    if (salida > pid->limite_out_max) {
        salida = pid->limite_out_max;
    } else if (salida < pid->limite_out_min) {
        salida = pid->limite_out_min;
    }
    return salida;
}
