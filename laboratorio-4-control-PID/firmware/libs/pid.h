/**
 * pid.h - Controlador PID (forma de la guia, con anti-windup).
 *
 * Una estructura guarda las ganancias y el estado del lazo.
 * No toca hardware: recibe una medida y devuelve una salida.
 *
 * Unidades en este proyecto:
 *   medida y setpoint: cm            kp: grados por cm
 *   salida: grados de servo          ki: grados por (cm*s)
 *   ts: segundos                     kd: grados por (cm/s)
 *
 * Equivalencias con la forma estandar de la guia:
 *   ki = kp / Ti      kd = kp * Td
 */

#ifndef PID_H
#define PID_H

typedef struct {
    float kp;
    float ki;
    float kd;
    float ts;              /* tiempo de muestreo [s] */
    float limite_out_max;  /* limites fisicos del actuador [grados] */
    float limite_out_min;
    float banda;           /* zona muerta del error: +/- cm alrededor del setpoint (0 = apagada) */
    float integral;        /* acumulador de la accion I, ya multiplicado por ki [grados] */
    float medida_previa;   /* para derivar la medida [cm] */
    float vel_filtrada;    /* velocidad de la bola suavizada [cm/s] */
} PID_Controller;

/* Carga las ganancias y deja el estado en cero. */
void PID_Init(PID_Controller *pid, float kp, float ki, float kd,
              float ts, float max, float min);

/* Reinicia el estado (integral y derivada) tomando la medida actual como punto de partida.
 * Llamar cuando el lazo estuvo abierto (sin pelota, modo medir). */
void PID_Reset(PID_Controller *pid, float medida_actual);

/* Calcula la salida de control para esta vuelta del lazo. */
float PID_Compute(PID_Controller *pid, float setpoint, float medida_actual);

#endif /* PID_H */
