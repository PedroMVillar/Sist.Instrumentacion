`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 05.02.2025 16:59:22
// Design Name: 
// Module Name: disp7seg_controller
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


module disp7seg_controller(
    input clk,
    input [3:0] bcd_dig [0:3],      // bcd_digX[3:0] -> bcd value 
    input [3:0] blank_dig,          // blank_dig[X] -> 1 = digit blank / 0 = digit enable
    output [7:0] seg,
    output [3:0] dig_en
    );

    logic [4:0] cnt;
    logic [7:0] BCD_7S_lut [0 : 15];
    logic [7:0] disp_seg;
    logic [3:0] disp_aSync;
    logic [3:0] disp_a;
    logic [1:0] current_digit;
    
    initial begin
        BCD_7S_lut <= {
        //     pgfedcba
            8'b11000000,    //"0"
            8'b11111001,    //"1"
            8'b10100100,    //"2"
            8'b10110000,    //"3"
            8'b10011001,    //"4"
            8'b10010010,    //"5"
            8'b10000010,    //"6"
            8'b11111000,    //"7"
            8'b10000000,    //"8"
            8'b10010000,    //"9"
            8'b10001000,    //"A"
            8'b10000011,    //"B"
            8'b11000110,    //"C"
            8'b10100001,    //"D"
            8'b10000110,    //"E"
            8'b10001110     //"F"
        };
        disp_aSync <= 4'b0111;
        current_digit <= 2'h0;
    end
    
    // Generacion del contador de sync
    always @(posedge clk) cnt <= cnt + 1;
    
    // Generacion de las señales de sync para digitos multiplexados del display
    always @(posedge cnt[1]) begin
        if (cnt[4:2] == 3'b000) begin
            disp_a <= 4'hF;
            disp_aSync <= {disp_aSync[2:0], disp_aSync[3]};
            disp_seg <= BCD_7S_lut[bcd_dig[current_digit]];                   
        end
        else if (cnt[4:2] == 3'b001) begin           
            if (blank_dig[current_digit] == 1'b0) disp_a <= disp_aSync;
            current_digit <= current_digit + 1;          
        end   
    end
    
    assign seg = disp_seg;
    assign dig_en = disp_a;
    
endmodule