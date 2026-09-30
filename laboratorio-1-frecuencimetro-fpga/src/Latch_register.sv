`timescale 1ns / 1ps


module Latch_register(
    input  logic clk,
    input  logic rst,
    input  logic latch_en,
    input  logic [3:0] in_units,
    input  logic [3:0] in_tens,
    input  logic [3:0] in_hundreds,
    input  logic [3:0] in_thousands,
    output logic [3:0] out_units,
    output logic [3:0] out_tens,
    output logic [3:0] out_hundreds,
    output logic [3:0] out_thousands
    );

    always_ff @(posedge clk) begin
        if (rst) begin
            out_units     <= 4'd0;
            out_tens      <= 4'd0;
            out_hundreds  <= 4'd0;
            out_thousands <= 4'd0;
        end else if (latch_en) begin
            out_units     <= in_units;
            out_tens      <= in_tens;
            out_hundreds  <= in_hundreds;
            out_thousands <= in_thousands;
        end
    end

endmodule
