%% IDENTIFICACION DEL LAZO REAL DEL BALANCIN (ball and beam con STM32)
% Lee el log de un ensayo de escalon (CSV de la UART) y obtiene:
%   K        : ganancia efectiva de la planta [cm/s^2 por grado de servo]
%   zonaMuerta: grados de servo que no producen movimiento (friccion estatica)
% y despues prueba ganancias Kp/Kd sobre ese modelo, antes de tocar la maqueta.
%
% Si no existe el CSV, el script genera datos SINTETICOS (K = 2, zona muerta = 4)
% para comprobar que el metodo recupera esos valores.
%
% Formato del CSV (firmware con LOG_EXTENDIDO = 1), sin encabezado:
%   tiempo_ms, posicion_cm, setpoint_cm, salida_grados
%
% Requiere: MATLAB base (no usa toolboxes).

clear; clc; close all; rng(1);

%% 1. Parametros del ensayo (iguales a los del firmware cuando se hizo el log)
P.Kp = 1.5;   P.Ki = 0;   P.Kd = 0.8;   % grados/cm, grados/(cm*s), grados/(cm/s)
P.Ts = 0.05;                            % TS_MS / 1000
P.lim = 35;                             % LIMITE_GRADOS
P.alfa = 0.7;                           % ALFA_VEL de pid.c (suavizado de la velocidad)
P.banda = 0;                            % banda_cm (dejar en 0 durante el ensayo)
P.slew = 150;                           % velocidad_servo [grados/s]
P.retardo = 1;                          % la lectura llega 1 Ts despues de lanzarse
P.mediana = 5;                          % FILTRO_N
P.zonaMuerta = 0;                       % se estima mas abajo
P.ruido = 0;  P.picos = 0;              % solo para datos sinteticos

archivo = 'log_escalon.csv';

%% 2. Datos: CSV real, o sinteticos si no existe
if isfile(archivo)
    D = readmatrix(archivo);
    tlog  = (D(:,1) - D(1,1)) / 1000;
    ylog  = D(:,2);
    splog = D(:,3);
    fprintf('Datos REALES: %s (%d muestras)\n', archivo, numel(tlog));
else
    Kverdadero = 2.0;  zmVerdadera = 4.0;
    Q = P;  Q.ruido = 0.4;  Q.picos = 0.07;  Q.zonaMuerta = zmVerdadera;
    spSint = @(t) 8*(t >= 1);                     % escalon de 8 cm en t = 1 s
    [tlog, ylog] = simularLazo(Kverdadero, Q, spSint, 14);
    splog = spSint(tlog);
    fprintf('Datos SINTETICOS: K = %.2f, zona muerta = %.1f grados\n', Kverdadero, zmVerdadera);
end

k0 = find(splog ~= splog(1), 1);              % primera muestra con el setpoint nuevo
t0 = tlog(k0);
A  = splog(end) - splog(1);                   % amplitud del escalon [cm]
fprintf('Escalon de %.1f cm en t = %.2f s\n', A, t0);

figure; plot(tlog, ylog, 'LineWidth', 1.2); hold on; grid on;
stairs(tlog, splog, '--');
xlabel('t [s]'); ylabel('posicion [cm]'); legend('Posicion', 'Setpoint');
title('Log del ensayo');

%% 3. Estimacion inicial con el metodo de sobrepico y tiempo de pico (Chen)
% Con el PD sobre la medida, el lazo cerrado ideal es de segundo orden y
% wn^2 = K*Kp, asi que K = wn^2/Kp. En este lazo es solo APROXIMADO: el
% filtro de la derivada, el muestreo y los retardos no estan en esa formula.
idx = (k0-1):numel(tlog);
[info, okChen] = identificarEscalon(tlog(idx), ylog(idx), A, 3);
if okChen
    K_chen = info.wn^2 / P.Kp;
    fprintf('Chen: zeta = %.2f  wn = %.2f rad/s  =>  K aprox = %.2f\n', info.zeta, info.wn, K_chen);
else
    K_chen = 1;
    fprintf('Chen: no hay sobrepico detectable. Se usa K = 1 como punto de partida.\n');
end

%% 4. Ajuste por simulacion del lazo completo
% Se simula el lazo con la misma estructura que el firmware (Ts, mediana,
% retardo, filtro de la derivada, limite de velocidad del servo) y se busca
% el K que hace coincidir la simulacion con el log.
spfun = @(t) interp1(tlog, splog, t, 'previous', 'extrap');
opciones = optimset('TolX', 1e-3, 'Display', 'off');

% (a) Solo K
costoK = @(K) costoLazo(K, 0, P, spfun, tlog, ylog, t0);
[K_a, rms_a] = fminbnd(costoK, 0.05, 20, opciones);

% (b) K y zona muerta (friccion estatica)
costoKZ = @(v) costoLazo(abs(v(1)), abs(v(2)), P, spfun, tlog, ylog, t0);
[v_b, rms_b] = fminsearch(costoKZ, [K_a, 1], opciones);
K_b = abs(v_b(1));  zm_b = abs(v_b(2));

fprintf('\nAjuste (a) solo K          : K = %.2f            RMS = %.2f cm\n', K_a, rms_a);
fprintf('Ajuste (b) K + zona muerta : K = %.2f, zm = %.1f   RMS = %.2f cm\n', K_b, zm_b, rms_b);
fprintf('(con datos sinteticos: K = 2.0, zm = 4.0)\n');

% Comparacion grafica
[ta, ya] = simularLazo(K_a, setfield(P, 'zonaMuerta', 0),    spfun, tlog(end));
[tb, yb] = simularLazo(K_b, setfield(P, 'zonaMuerta', zm_b), spfun, tlog(end));
figure; hold on; grid on;
plot(tlog, ylog, 'Color', [0.2 0.7 1.0], 'LineWidth', 4, 'DisplayName', 'Log');
plot(ta, ya, '--', 'Color', [0.6 0.6 0.6], 'LineWidth', 2, 'DisplayName', sprintf('Solo K = %.2f', K_a));
plot(tb, yb, '--', 'Color', [1.0 0.5 0.0], 'LineWidth', 2, 'DisplayName', sprintf('K = %.2f + zona muerta %.1f', K_b, zm_b));
legend('Location', 'best'); xlabel('t [s]'); ylabel('posicion [cm]');
title('Log contra modelo ajustado');

% Si el ajuste (b) reduce mucho el RMS respecto del (a), la zona muerta (friccion
% estatica) es real y hay que tenerla en cuenta: es lo que limita al PID lineal.

%% 5. Que significa K
% Una pelota de ping pong es hueca: x'' = (3/5)*g*theta. Por grado de VIGA:
g = 981;                                  % cm/s^2
Kviga = 0.6 * g * pi/180;                 % cm/s^2 por grado de viga (~10.3)
razon = K_b / Kviga;                      % grados de viga por grado de servo
fprintf('\nK por grado de viga (teorico)        : %.1f cm/s^2\n', Kviga);
fprintf('K por grado de servo (identificado) : %.2f cm/s^2\n', K_b);
fprintf('Relacion brazo-viga implicita       : %.2f grados de viga por grado de servo\n', razon);
fprintf('Contrastar con la medicion de angulo real de la viga (inclinometro del celular).\n');

%% 6. Probar ganancias sobre el modelo (sin tocar la maqueta)
% Escalon de 8 cm con el modelo identificado. Se reporta:
%   sobrepico, error final y oscilacion pico a pico en los ultimos 3 s.
% Una oscilacion pico a pico grande (> 2 cm) indica lazo inestable o con ciclo limite.
Kps = [1.0 1.5 2.5];
Kds = [0.4 0.8 1.5];
Ptest = P;  Ptest.zonaMuerta = zm_b;  Ptest.Ki = 0;
spTest = @(t) 8*(t >= 1);

fprintf('\nPrueba de ganancias (K = %.2f, zona muerta = %.1f, Ki = 0)\n', K_b, zm_b);
fprintf('sobrepico%% / error final [cm] / oscilacion pico a pico [cm]\n');
figure; hold on; grid on; colores = lines(numel(Kps)*numel(Kds)); n = 0;
for kp = Kps
    fprintf('Kp = %.1f: ', kp);
    for kd = Kds
        Ptest.Kp = kp;  Ptest.Kd = kd;
        [tt, yy] = simularLazo(K_b, Ptest, spTest, 20);
        m = tt >= 1;
        sobrepico = (max(yy(m)) - 8) / 8 * 100;
        final = yy(tt >= tt(end) - 3);
        errorFinal = 8 - mean(final);
        pp = max(final) - min(final);
        fprintf('[Kd %.1f: %4.0f%% / %5.1f / %5.1f]  ', kd, sobrepico, errorFinal, pp);
        if pp < 2                              % solo se grafican los que no oscilan
            n = n + 1;
            plot(tt, yy, 'Color', colores(n,:), 'DisplayName', sprintf('Kp=%.1f Kd=%.1f', kp, kd));
        end
    end
    fprintf('\n');
end
legend('Location', 'best'); xlabel('t [s]'); ylabel('posicion [cm]');
title('Respuestas de las combinaciones que no oscilan');

%% Funciones locales (en un Live Script van al final)

function [info, ok] = identificarEscalon(t, y, A, nfilt)
% Estima K, zeta y wn de un 2do orden subamortiguado desde la respuesta al escalon.
    if nargin < 4, nfilt = 1; end
    t = t(:) - t(1);  y = y(:);
    y = y - y(1);
    if nfilt > 1, y = movmean(y, nfilt); end
    n10 = max(5, round(0.10*numel(y)));
    yss = mean(y(end-n10+1:end));
    [ymax, i] = max(y);
    OS = (ymax - yss) / yss;
    ok = OS > 0.001 && t(i) > 0;
    info = struct('K', yss/A, 'zeta', NaN, 'wn', NaN, 'OS', OS, 'Tp', t(i));
    if ~ok, return; end
    l = log(OS);
    info.zeta = -l / sqrt(pi^2 + l^2);
    info.wn   = (pi / t(i)) / sqrt(1 - info.zeta^2);
end

function r = costoLazo(K, zm, P, spfun, tlog, ylog, t0)
% Error RMS entre el log y la simulacion (sin ruido) para un K y una zona muerta.
    Q = P;  Q.zonaMuerta = zm;  Q.ruido = 0;  Q.picos = 0;
    [ts, ys] = simularLazo(K, Q, spfun, tlog(end));
    ysim = interp1(ts, ys, tlog, 'previous', 'extrap');
    m = tlog >= t0;
    r = sqrt(mean((ysim(m) - ylog(m)).^2));
end

function [tlog, ylog] = simularLazo(K, P, spfun, Tfin)
% Simula el lazo cerrado con la misma logica del firmware:
%   planta  : x'' = K * u   (u = angulo del servo menos la zona muerta)
%   sensor  : lectura retrasada P.retardo ticks + ruido + picos + mediana de P.mediana
%   PID     : P sobre el error, D sobre la medida con suavizado, I con anti-windup
%   servo   : velocidad limitada a P.slew grados/s, actualizado cada 20 ms
    dt = 0.001;
    pasosTick = round(P.Ts / dt);
    nTicks = floor(Tfin / P.Ts) + 1;
    tlog = (0:nTicks-1)' * P.Ts;
    ylog = zeros(nTicks, 1);
    xhist = zeros(nTicks, 1);
    x = 0;  v = 0;  ang = 0;  objetivo = 0;
    integral = 0;  medPrev = 0;  vFilt = 0;  buf = [];
    for k = 1:nTicks
        t = tlog(k);
        xhist(k) = x;
        % --- sensor
        xm = xhist(max(k - P.retardo, 1));
        r = xm + P.ruido * randn;
        if rand < P.picos
            r = r + sign(randn) * (3 + 5*rand);
        end
        buf = [buf, r];                          %#ok<AGROW>
        if numel(buf) > P.mediana, buf(1) = []; end
        pos = median(buf);
        if k == 1, medPrev = pos; end
        % --- PID (igual que pid.c)
        vel = (pos - medPrev) / P.Ts;
        medPrev = pos;
        vFilt = P.alfa * vFilt + (1 - P.alfa) * vel;
        dTerm = -P.Kd * vFilt;
        e = spfun(t) - pos;
        if abs(e) <= P.banda, e = 0; end
        pTerm = P.Kp * e;
        sal = pTerm + integral + dTerm;
        if ~((sal >= P.lim && e > 0) || (sal <= -P.lim && e < 0))
            integral = integral + P.Ki * e * P.Ts;
        end
        integral = min(max(integral, -P.lim), P.lim);
        objetivo = min(max(pTerm + integral + dTerm, -P.lim), P.lim);
        ylog(k) = pos;
        % --- servo y planta, paso de 1 ms
        for j = 1:pasosTick
            ms = (k-1) * pasosTick + (j-1);
            if mod(ms, 20) == 0
                paso = P.slew * 0.02;
                dif = objetivo - ang;
                if P.slew <= 0 || abs(dif) <= paso
                    ang = objetivo;
                else
                    ang = ang + sign(dif) * paso;
                end
            end
            if abs(ang) < P.zonaMuerta
                u = 0;
            else
                u = ang - sign(ang) * P.zonaMuerta;
            end
            v = v + K * u * dt;
            x = x + v * dt;
        end
    end
end
