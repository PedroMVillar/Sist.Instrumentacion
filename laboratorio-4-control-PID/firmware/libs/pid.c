#include "pid.h"

/* PID de la guia con tres decisiones de diseno, todas justificables:
 *
 * 1. Derivada sobre la MEDIDA (no sobre el error). Con setpoint fijo es lo
 *    mismo, pero si el setpoint salta (ensayo de escalon) no genera un golpe
 *    de derivada.
 * 2. Velocidad suavizada: el sensor ultrasonico tiene ruido de ~0.4 cm y al
 *    dividir por ts se amplifica. Se promedia 85% anterior + 15% nueva (alfa_vel).
 * 3. La integral se guarda ya multiplicada por ki (en grados). Asi se puede
 *    cambiar ki en vivo sin que la integral pegue un salto, y el anti-windup
 *    es simple: no integrar si la salida esta al tope empujando hacia el tope.
 */

volatile float alfa_vel = 0.85f; /* suavizado de la velocidad para el D: cerca de 1 = mas suave pero mas lento; editable en vivo */

void PID_Init(PID_Controller *pid, float kp, float ki, float kd,
              float ts, float max, float min) {
    pid->kp = kp;
    pid->ki = ki;
    pid->kd = kd;
    pid->ts = ts;
    pid->limite_out_max = max;
    pid->limite_out_min = min;
    PID_Reset(pid, 0.0f);
}

void PID_Reset(PID_Controller *pid, float medida_actual) {
    pid->integral = 0.0f;
    pid->medida_previa = medida_actual;
    pid->vel_filtrada = 0.0f;
}

float PID_Compute(PID_Controller *pid, float setpoint, float medida_actual) {
    float error = setpoint - medida_actual;

    /* Derivada: velocidad de la bola, suavizada. */
    float velocidad = (medida_actual - pid->medida_previa) / pid->ts;
    pid->medida_previa = medida_actual;
    pid->vel_filtrada = alfa_vel * pid->vel_filtrada + (1.0f - alfa_vel) * velocidad;
    float d_term = -pid->kd * pid->vel_filtrada;

    /* Proporcional */
    float p_term = pid->kp * error;

    /* Integral con anti-windup: si la salida ya esta al tope y el error
     * empuja hacia ese mismo tope, no se acumula. */
    float salida = p_term + pid->integral + d_term;
    int satura_arriba = (salida >= pid->limite_out_max) && (error > 0.0f);
    int satura_abajo  = (salida <= pid->limite_out_min) && (error < 0.0f);
    if (!satura_arriba && !satura_abajo) {
        pid->integral += pid->ki * error * pid->ts;
    }
    if (pid->integral > pid->limite_out_max) pid->integral = pid->limite_out_max;
    if (pid->integral < pid->limite_out_min) pid->integral = pid->limite_out_min;

    /* Salida total, limitada al rango del actuador. */
    salida = p_term + pid->integral + d_term;
    if (salida > pid->limite_out_max) salida = pid->limite_out_max;
    if (salida < pid->limite_out_min) salida = pid->limite_out_min;
    return salida;
}
