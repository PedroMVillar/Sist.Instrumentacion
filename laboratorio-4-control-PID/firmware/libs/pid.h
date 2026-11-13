/**
 * pid.h — Librería propia del controlador PID (laboratorio 4).
 *
 * Sigue la forma de la guía: una estructura encapsula las ganancias y
 * el estado del lazo, así pueden convivir varios controladores a la vez.
 * No toca hardware: recibe una medida y devuelve una salida.
 */

#ifndef PID_H
#define PID_H

typedef struct {
    float kp;
    float ki;
    float kd;
    float ts;              /* tiempo de muestreo en segundos */
    float error_previo;
    float integral;
    float limite_out_max;  /* límites físicos del actuador (grados del servo) */
    float limite_out_min;
} PID_Controller;

/* Carga las ganancias y deja el estado en cero */
void PID_Init(PID_Controller *pid, float kp, float ki, float kd,
              float ts, float max, float min);

/* Calcula la salida de control para esta vuelta del lazo */
float PID_Compute(PID_Controller *pid, float setpoint, float medida_actual);

#endif /* PID_H */
