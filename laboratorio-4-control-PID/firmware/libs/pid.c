#include "pid.h"

/* Copiado del PDF "Control PID - Ball and Beam", seccion 4.
 * Agregados propios (marcados): integral en cero mientras ki = 0 y al cruzar el centro,
 * termino D suavizado,
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
    pid->banda = 0.0f; /* agregado: sin region de aceptacion hasta que se pida */
    pid->d_filtrado = 0.0f; /* agregado: D suavizado */
}

float PID_Compute(PID_Controller *pid, float setpoint, float medida_actual) {
    float error = setpoint - medida_actual;

    /* Agregado: si la pelota cruzo el setpoint (el error cambio de signo), la
     * integral ya hizo su trabajo (destrabarla) y se borra. Si no, la empuja
     * de largo para el otro lado y tarda mucho en descargarse. */
    int cruzo = (error > 0.0f && pid->error_previo < 0.0f) ||
                (error < 0.0f && pid->error_previo > 0.0f);

    /* 3. Accion Derivativa (se calcula antes para saber si la salida satura).
     * Usa el error real siempre, tambien dentro de la region de aceptacion:
     * asi sigue frenando la pelota si entra al centro con velocidad. */
    float d_nuevo = pid->kd * ((error - pid->error_previo) / pid->ts);
    /* Agregado: D suavizado. El sensor salta ~0.4 cm entre lecturas y el D lo
     * toma como si la pelota se moviera rapido: el servo temblaba. Se usa 70%
     * del valor anterior + 30% del nuevo, asi sigue frenando pero sin temblar. */
    pid->d_filtrado = 0.7f * pid->d_filtrado + 0.3f * d_nuevo;
    float d_term = pid->d_filtrado;
    pid->error_previo = error;

    /* Agregado: region de aceptacion. Si la pelota esta a menos de 'banda' del
     * setpoint se la da por centrada: P no empuja y la integral no acumula, para
     * que el ruido del sensor o la integral no la saquen del medio. */
    int dentro = (error <= pid->banda) && (error >= -pid->banda);
    if (dentro) {
        error = 0.0f;
    }

    /* 1. Accion Proporcional */
    float p_term = pid->kp * error;

    /* 2. Accion Integral (Aproximacion rectangular) */
    /* Cambio respecto del PDF (anti-windup condicional): si la salida ya esta
     * al tope y el error empuja hacia ese mismo tope, no se acumula. Sin esto,
     * con la pelota un rato en una punta la integral se llena y la salida queda
     * clavada en +/-15 grados mucho despues de que la pelota volvio. */
    float salida_previa = p_term + d_term + pid->ki * pid->integral;
    int satura_arriba = salida_previa >= pid->limite_out_max && error > 0.0f;
    int satura_abajo = salida_previa <= pid->limite_out_min && error < 0.0f;
    if (!dentro && !satura_arriba && !satura_abajo) {
        pid->integral += error * pid->ts;
    }
    /* Agregado: con ki = 0 (Ziegler-Nichols paso 1) no acumular, para que al
     * subir ki en vivo no aparezca de golpe una integral enorme. */
    if (pid->ki == 0.0f || cruzo) {
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
