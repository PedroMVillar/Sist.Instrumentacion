`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 24.08.2026 15:54:55
// Design Name: 
// Module Name: FSM_Control
// Project Name: 
// Target Devices: 
// Tool Versions: 
// Description: 
// 
// Dependencies: 
// 
// Revision:
// Revision 0.01 - File Created
// Additional Comments:
// 
//////////////////////////////////////////////////////////////////////////////////


module FSM_Control(
    input logic clk,
    input logic rst,
    input logic tick_1ms,
    output logic cnt_en,
    output logic cnt_rst,
    output logic latch_en
    );


enum {S_MEDICION, S_CAPTURA, S_REINICIO} state, next_state;

// Contador de milisegundos para medir la ventana de 1 segundo (0 a 999)
    // log2(1000) = 9.96 -> Necesitamos 10 bits.
    logic [9:0] ms_count;
    logic reset_ms; // Señal de control interna

    // 2. Lógica del contador de milisegundos (Secuencial)
    always_ff @(posedge clk) begin
        if (rst || reset_ms) begin
            ms_count <= '0;
        end else if (tick_1ms && state == S_MEDICION) begin
            // Solo incrementa si estamos en el estado de medición y llega el tick
            ms_count <= ms_count + 1'b1;
        end
    end

    // 3. Registro de Estado (Secuencial)
    always_ff @(posedge clk) begin
        if (rst) begin
            state <= S_MEDICION;
        end else begin
            state <= next_state;
        end
    end
    
    // 4. Lógica de Próximo Estado y Salidas (Combinacional)
    always_comb begin
        // Valores por defecto para evitar latches inferidos
        next_state = state;
        cnt_en     = 1'b0;
        latch_en   = 1'b0;
        cnt_rst    = 1'b0;
        reset_ms   = 1'b0;

        case (state)
            S_MEDICION: begin
                cnt_en = 1'b1; // Dejamos pasar la señal incógnita a los contadores
                
                // Esperamos hasta llegar a los 1000 ms (de 0 a 999)
                if (tick_1ms && (ms_count == 10'd999)) begin
                    next_state = S_CAPTURA;
                end
            end

            S_CAPTURA: begin
                // Al entrar aquí, cnt_en es 0 (detiene el conteo)
                latch_en = 1'b1; // Dispara el reloj de los Flip-Flops del display
                
                // Pasa al siguiente estado inmediatamente en el próximo ciclo de clk (10 ns)
                next_state = S_REINICIO; 
            end

            S_REINICIO: begin
                cnt_rst  = 1'b1; // Envía un pulso de reset a los contadores BCD
                reset_ms = 1'b1; // Reinicia el contador de tiempo interno
                
                // Vuelve a medir en el próximo ciclo de clk
                next_state = S_MEDICION;
            end
            
            default: next_state = S_MEDICION;
        endcase
    end
    
    



endmodule