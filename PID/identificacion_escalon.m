%% IDENTIFICACION POR RESPUESTA AL ESCALON (2do orden subamortiguado)
%  Metodo: Overshoot + tiempo de pico (pagina de U. Washington, lec07)
%  Requiere: Control System Toolbox. (System Identification Toolbox opcional)
%
%  Modelo:  G(s) = K*wn^2 / (s^2 + 2*zeta*wn*s + wn^2),  0 < zeta < 1
%     K    = yss / A                       (A = amplitud del escalon de entrada)
%     %OS  = (ymax - yss)/yss*100
%     zeta = -ln(OS) / sqrt(pi^2 + ln(OS)^2)     (OS en tanto por uno)
%     wd   = pi / Tp ;   wn = wd / sqrt(1 - zeta^2)
%
%  Correr por secciones (Ctrl+Enter en cada %%) para ir viendo los graficos.

clear; clc; close all;

%% ========================================================================
%  PARTE 1: Un ejemplo "de manual" paso a paso (sin ruido)
%  ========================================================================
% Sistema "verdadero" (el que en la practica NO conoceriamos)
K_real = 2;  zeta_real = 0.35;  wn_real = 6;            % rad/s
G_real = tf(K_real*wn_real^2, [1, 2*zeta_real*wn_real, wn_real^2]);

A = 1;                                  % amplitud del escalon aplicado
t = (0:0.005:5)';                       % vector de tiempo del "experimento"
y = step(A*G_real, t);                  % "medicion" (en la maqueta seria el log)

[G_est, info] = identificar_chen(t, y, A);

fprintf('--- PARTE 1: sin ruido ---\n');
fprintf('          real     estimado\n');
fprintf('K      : %7.3f  %7.3f\n', K_real,    info.K);
fprintf('zeta   : %7.3f  %7.3f\n', zeta_real, info.zeta);
fprintf('wn     : %7.3f  %7.3f\n', wn_real,   info.wn);
fprintf('OS [%%] : %7.2f\n', info.OS*100);
fprintf('Tp [s] : %7.3f\n', info.Tp);

figure('Name','Parte 1');
plot(t, y, 'b', 'LineWidth', 1.5); hold on;
step(A*G_est, t);
plot(info.Tp, info.ymax, 'ro', 'MarkerFaceColor','r');
yline(info.yss, '--k'); yline(info.ymax, ':r');
legend('Medicion (real)', 'Modelo identificado', 'Pico', 'yss', 'ymax');
title('Identificacion por overshoot y tiempo de pico'); grid on;

%% ========================================================================
%  PARTE 2: Barrido de zeta  -> ¿cuando funciona bien el metodo?
%  ========================================================================
fprintf('\n--- PARTE 2: barrido de zeta (sin ruido) ---\n');
zetas = [0.1 0.2 0.3 0.5 0.7 0.9];
fprintf(' zeta_real  zeta_est   wn_est(real=6)\n');
for z = zetas
    G = tf(wn_real^2, [1, 2*z*wn_real, wn_real^2]);
    tt = (0:0.002:15)';
    yy = step(G, tt);
    [~, inf2] = identificar_chen(tt, yy, 1);
    fprintf('  %6.2f     %6.3f     %6.3f\n', z, inf2.zeta, inf2.wn);
end
% Con zeta alto el overshoot es chico (zeta=0.9 -> OS ~ 0.15%) y
% cualquier ruido de medicion lo tapa: el metodo se vuelve muy sensible.

%% ========================================================================
%  PARTE 3: Efecto del RUIDO de medicion (como en un sensor real)
%  ========================================================================
rng(1);
sigma = 0.02;                           % desvio del ruido (2% del escalon)
y_ruido = y + sigma*randn(size(y));

[G_n1, info1] = identificar_chen(t, y_ruido, A, 1);     % sin filtrar
[G_n2, info2] = identificar_chen(t, y_ruido, A, 15);    % media movil de 15 muestras

fprintf('\n--- PARTE 3: con ruido (sigma = %.2f) ---\n', sigma);
fprintf('sin filtro   : K=%.3f zeta=%.3f wn=%.3f\n', info1.K, info1.zeta, info1.wn);
fprintf('filtro (15)  : K=%.3f zeta=%.3f wn=%.3f\n', info2.K, info2.zeta, info2.wn);
fprintf('real         : K=%.3f zeta=%.3f wn=%.3f\n', K_real, zeta_real, wn_real);

figure('Name','Parte 3');
plot(t, y_ruido, 'Color',[.7 .7 .7]); hold on;
plot(t, y, 'b', 'LineWidth', 1.2);
step(A*G_n2, t);
legend('Medicion con ruido','Real (sin ruido)','Modelo identificado (filtrado)');
grid on; title('Efecto del ruido y del filtrado');

%% ========================================================================
%  PARTE 4: Monte Carlo -> dispersion de la estimacion con ruido
%  ========================================================================
N = 200;  Z = zeros(N,1);  W = zeros(N,1);  Kk = zeros(N,1);
for k = 1:N
    yk = y + sigma*randn(size(y));
    [~, ik] = identificar_chen(t, yk, A, 15);
    Z(k) = ik.zeta;  W(k) = ik.wn;  Kk(k) = ik.K;
end
fprintf('\n--- PARTE 4: Monte Carlo (%d corridas) ---\n', N);
fprintf('K    : %.3f +/- %.3f (real %.2f)\n', mean(Kk), std(Kk), K_real);
fprintf('zeta : %.3f +/- %.3f (real %.2f)\n', mean(Z),  std(Z),  zeta_real);
fprintf('wn   : %.3f +/- %.3f (real %.2f)\n', mean(W),  std(W),  wn_real);

figure('Name','Parte 4');
subplot(1,2,1); histogram(Z, 20); xline(zeta_real,'r','LineWidth',2); title('\zeta estimado');
subplot(1,2,2); histogram(W, 20); xline(wn_real,'r','LineWidth',2);   title('\omega_n estimado');

%% ========================================================================
%  PARTE 5: Planta que NO es de 2do orden (el caso realista)
%  ========================================================================
%  3er orden: el 2do orden + un polo extra rapido (ej. dinamica del servo).
%  Estimamos un 2do orden y vemos cuanto error de modelo aparece.
G3 = G_real * tf(1, [0.08 1]);          % polo extra en 12.5 rad/s
y3 = step(A*G3, t);
[G3_est, info3] = identificar_chen(t, y3, A);

fprintf('\n--- PARTE 5: planta de 3er orden ---\n');
fprintf('K=%.3f zeta=%.3f wn=%.3f  (la planta no es de 2do orden: es una aproximacion)\n', ...
        info3.K, info3.zeta, info3.wn);

figure('Name','Parte 5');
plot(t, y3, 'b', 'LineWidth', 1.5); hold on; step(A*G3_est, t);
legend('Planta 3er orden','Modelo 2do orden identificado'); grid on;
title('Error de modelo cuando la planta no es exactamente 2do orden');

% Comparacion opcional con ajuste por optimizacion (System Identification Toolbox)
if license('test','System_Identification_Toolbox')
    d  = iddata(y3, A*ones(size(t)), t(2)-t(1));
    Gt = tfest(d, 2, 0);                 % 2 polos, 0 ceros
    figure('Name','Parte 5b'); compare(d, Gt, G3_est);
    title('tfest (ajuste por minimos cuadrados) vs. metodo de overshoot');
end

%% ========================================================================
%  PARTE 6: BALL & BEAM  — ¿por que NO se puede hacer el escalon a lazo abierto?
%  ========================================================================
%  Modelo simplificado: entrada = angulo de la viga theta [rad]
%                       salida  = posicion de la bola x [m]
%     x'' = (5/7) * g * theta   =>   G(s) = Kb / s^2 ,   Kb = 5g/7 ~ 7 m/s^2/rad
%  Es un DOBLE INTEGRADOR: ante un escalon en theta, la bola acelera sin parar
%  hasta caerse de la viga. No hay valor final => no hay K, OS ni Tp.
Kb = 5*9.81/7;
Gbb = tf(Kb, [1 0 0]);

figure('Name','Parte 6a');
step(0.05*Gbb, 1.5); grid on;           % 0.05 rad ~ 2.9 grados
title('Ball&Beam a lazo ABIERTO: x crece sin limite (parabola)');

%% ------------------------------------------------------------------------
%  Solucion: identificar a LAZO CERRADO con un controlador PD.
%  Con la derivada aplicada sobre la MEDICION (no sobre el error):
%       u = Kp*(r - x) - Kd*dx/dt
%  el lazo cerrado queda EXACTAMENTE de 2do orden, sin cero:
%       T(s) = Kb*Kp / (s^2 + Kb*Kd*s + Kb*Kp)
%  => wn^2 = Kb*Kp   ,   2*zeta*wn = Kb*Kd    (K lazo cerrado = 1)
%  Entonces:  Kb = wn^2 / Kp      (lo que buscamos de la planta)
%             y se verifica con  Kb = 2*zeta*wn / Kd
%  ------------------------------------------------------------------------
Kp_test = 1.0;   Kd_test = 0.35;        % elegidos para que sea subamortiguado
T_cl = tf(Kb*Kp_test, [1, Kb*Kd_test, Kb*Kp_test]);

r_amp = 0.05;                           % escalon de referencia: 5 cm
tc = (0:0.005:6)';
xc = step(r_amp*T_cl, tc) + 0.0003*randn(size(tc));   % + ruido del sensor (0.3 mm)

[~, icl] = identificar_chen(tc, xc, r_amp, 9);
Kb_est_1 = icl.wn^2 / Kp_test;
Kb_est_2 = 2*icl.zeta*icl.wn / Kd_test;

fprintf('\n--- PARTE 6: Ball&Beam por lazo cerrado ---\n');
fprintf('K lazo cerrado (debe ser ~1): %.3f\n', icl.K);
fprintf('zeta=%.3f  wn=%.3f\n', icl.zeta, icl.wn);
fprintf('Kb real = %.3f | via wn^2/Kp = %.3f | via 2*zeta*wn/Kd = %.3f\n', ...
        Kb, Kb_est_1, Kb_est_2);

figure('Name','Parte 6b');
plot(tc, xc, 'Color',[.7 .7 .7]); hold on;
step(r_amp*tf(1,1)*T_cl, tc); grid on;
title('Ball&Beam a lazo cerrado (PD) -> identificacion de Kb');
legend('Medicion simulada','Real');

%% ========================================================================
%  PARTE 7: Con el modelo identificado, DISEÑAR/AJUSTAR el PID
%  ========================================================================
%  Planta identificada (agregamos la dinamica del servo como polo rapido
%  conocido o medido; aca tau_servo = 0.05 s como ejemplo)
tau_servo = 0.05;
Gp = tf(Kb_est_1, [1 0 0]) * tf(1, [tau_servo 1]);

% PID en forma estandar como en el PDF:  C = Kp*(1 + 1/(Ti*s) + Td*s/(Tf*s+1))
pid_std = @(Kp,Ti,Td,Tf) Kp*(1 + tf(1,[Ti 0]) + tf([Td 0],[Tf 1]));

% (a) Sintonia automatica de partida, para ver un orden de magnitud
C0 = pidtune(Gp, 'PIDF');
fprintf('\n--- PARTE 7: pidtune ---\n'); disp(C0);

% (b) Parametros "a mano" (Kp, Ti, Td) y analisis
Kp = 0.8;  Ti = 3;  Td = 0.5;  Tf = Td/10;
C  = pid_std(Kp, Ti, Td, Tf);
L  = C*Gp;                              % lazo abierto
T  = feedback(L, 1);                    % lazo cerrado

figure('Name','Parte 7a'); step(T, 10); grid on;
title(sprintf('Lazo cerrado: Kp=%.2f Ti=%.1f Td=%.2f', Kp, Ti, Td));
s = stepinfo(T);  disp(s);
figure('Name','Parte 7b'); margin(L); grid on;
figure('Name','Parte 7c'); rlocus(L); grid on; title('Lugar de raices');

%% ------------------------------------------------------------------------
%  (c) Barridos: ver el efecto de cada parametro (Kp, Ti, Td)
%  ------------------------------------------------------------------------
figure('Name','Parte 7d');
subplot(1,3,1); hold on; grid on; title('Barrido Kp');
for kp = [0.4 0.8 1.6 2.4]
    step(feedback(pid_std(kp,Ti,Td,Tf)*Gp,1), 10);
end
legend('0.4','0.8','1.6','2.4');

subplot(1,3,2); hold on; grid on; title('Barrido Ti');
for ti = [1.5 3 6 12]
    step(feedback(pid_std(Kp,ti,Td,Tf)*Gp,1), 10);
end
legend('1.5','3','6','12');

subplot(1,3,3); hold on; grid on; title('Barrido Td');
for td = [0.1 0.3 0.5 1.0]
    step(feedback(pid_std(Kp,Ti,td,td/10)*Gp,1), 10);
end
legend('0.1','0.3','0.5','1.0');

%% ------------------------------------------------------------------------
%  (d) Discretizacion a Ts = 20 ms (como en el STM32) y comparacion
%  ------------------------------------------------------------------------
Ts  = 0.02;
Cd  = c2d(C, Ts, 'tustin');
Gd  = c2d(Gp, Ts, 'zoh');
Td_cl = feedback(Cd*Gd, 1);
figure('Name','Parte 7e');
step(T, 10); hold on; step(Td_cl, 10); grid on;
legend('Continuo','Discreto Ts = 20 ms');
title('Efecto del muestreo del microcontrolador');

%% ========================================================================
%  FUNCION: identificar_chen
%  ========================================================================
function [G, info] = identificar_chen(t, y, A, nfilt)
% IDENTIFICAR_CHEN  Identifica un 2do orden subamortiguado desde un escalon.
%   [G, info] = identificar_chen(t, y, A, nfilt)
%     t, y  : tiempo y salida medida (vectores columna), arranca en y(0)~0
%     A     : amplitud del escalon aplicado a la entrada
%     nfilt : (opcional) ventana de media movil para filtrar ruido (default 1)
    if nargin < 4, nfilt = 1; end
    t = t(:);  y = y(:);
    % Saca el offset inicial. IMPORTANTE: t=0 debe ser el instante del escalon
    % y(1) debe ser la salida en reposo (antes de aplicar el escalon).
    y = y - y(1);

    if nfilt > 1
        y = movmean(y, nfilt);          % media movil centrada (sin retardo de fase)
    end

    % --- Valor final: promedio del ultimo 10 % de la medicion
    n10 = max(5, round(0.10*numel(y)));
    yss = mean(y(end-n10+1:end));
    K   = yss / A;

    % --- Pico, overshoot y tiempo de pico
    [ymax, idx] = max(y);
    Tp = t(idx);
    OS = (ymax - yss) / yss;            % en tanto por uno
    if OS <= 0.001
        error(['No hay sobrepico detectable: el metodo no aplica ' ...
               '(sistema sobreamortiguado o amortiguamiento muy alto).']);
    end

    % --- Parametros del 2do orden
    lnOS = log(OS);
    zeta = -lnOS / sqrt(pi^2 + lnOS^2);
    wd   = pi / Tp;
    wn   = wd / sqrt(1 - zeta^2);

    G = tf(K*wn^2, [1, 2*zeta*wn, wn^2]);

    info = struct('K',K,'zeta',zeta,'wn',wn,'OS',OS,'Tp',Tp, ...
                  'yss',yss,'ymax',ymax,'Ts_aprox',4/(zeta*wn));
end