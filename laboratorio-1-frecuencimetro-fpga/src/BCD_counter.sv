`timescale 1ns / 1ps

module BCD_counter(
    input  logic clk,
    input  logic rst,
    input  logic enable,
    output logic [3:0] bcd_out,
    output logic carry_out
    );

    always_ff @(posedge clk) begin
        if (rst) begin
            bcd_out   <= 4'd0;
            carry_out <= 1'b0;
        end else if (enable) begin
            if (bcd_out == 4'd9) begin
                bcd_out   <= 4'd0;
                carry_out <= 1'b1;
            end else begin
                bcd_out   <= bcd_out + 1'b1;
                carry_out <= 1'b0;
            end
        end else begin
            carry_out <= 1'b0;
        end
    end

endmodule
