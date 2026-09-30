`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 24.08.2026 16:15:27
// Design Name: 
// Module Name: Frequency_meter
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


module Frequency_meter(
    input clk,
    input freq_in,
    output [7:0] D0_SEG,
    output [3:0] D0_AN
    );

    logic [13:0] cnt_display;

    disp7seg_controller dispA(  .clk(cnt_display[13]),
                                .bcd_dig({XXXX,XXXX,XXXX,XXXX}), // Aca se deben conectar los BCD de salida del LATCH
                                .blank_dig(4'b0000),
                                .seg(D0_seg),
                                .dig_en(D0_a));
                               
                                

    always @(posedge clk) cnt_display <= cnt_display + 1;


endmodule






