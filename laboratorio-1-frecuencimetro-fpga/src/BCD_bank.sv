`timescale 1ns / 1ps

module BCD_bank(
    input  logic clk,
    input  logic rst,
    input  logic enable,
    output logic [3:0] units,
    output logic [3:0] tens,
    output logic [3:0] hundreds,
    output logic [3:0] thousands
    );

    logic carry_u, carry_d, carry_c;

    BCD_counter cnt_units (
        .clk(clk),
        .rst(rst),
        .enable(enable),
        .bcd_out(units),
        .carry_out(carry_u)
    );

    BCD_counter cnt_tens (
        .clk(clk),
        .rst(rst),
        .enable(carry_u),
        .bcd_out(tens),
        .carry_out(carry_d)
    );

    BCD_counter cnt_hundreds (
        .clk(clk),
        .rst(rst),
        .enable(carry_d),
        .bcd_out(hundreds),
        .carry_out(carry_c)
    );

    BCD_counter cnt_thousands (
        .clk(clk),
        .rst(rst),
        .enable(carry_c),
        .bcd_out(thousands),
        .carry_out()
    );

endmodule
