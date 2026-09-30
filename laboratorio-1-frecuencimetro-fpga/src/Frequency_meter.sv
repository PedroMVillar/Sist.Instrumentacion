`timescale 1ns / 1ps

module Frequency_meter(
    input  logic clk,
    input  logic [0:0] btn,
    input  logic servo0,
    output logic [7:0] D0_SEG,
    output logic [3:0] D0_AN
    );

    logic rst;
    logic freq_in;
    logic tick_1ms;
    logic cnt_en, cnt_rst, latch_en;

    logic freq_sync_0, freq_sync_1, freq_prev;
    logic freq_edge;
    logic count_pulse;

    logic [3:0] bcd_units, bcd_tens, bcd_hundreds, bcd_thousands;
    logic [3:0] latch_units, latch_tens, latch_hundreds, latch_thousands;
    logic [13:0] cnt_display;

    assign rst     = btn[0];
    assign freq_in = servo0;

    always_ff @(posedge clk) begin
        if (rst) begin
            freq_sync_0 <= 1'b0;
            freq_sync_1 <= 1'b0;
            freq_prev   <= 1'b0;
        end else begin
            freq_sync_0 <= freq_in;
            freq_sync_1 <= freq_sync_0;
            freq_prev   <= freq_sync_1;
        end
    end

    assign freq_edge   = freq_sync_1 & ~freq_prev;
    assign count_pulse = freq_edge & cnt_en;

    BaseTime1ms base_time (
        .clk_in(clk),
        .rst(rst),
        .tick_out(tick_1ms)
    );

    FSM_Control fsm (
        .clk(clk),
        .rst(rst),
        .tick_1ms(tick_1ms),
        .cnt_en(cnt_en),
        .cnt_rst(cnt_rst),
        .latch_en(latch_en)
    );

    BCD_bank bcd_bank (
        .clk(clk),
        .rst(rst | cnt_rst),
        .enable(count_pulse),
        .units(bcd_units),
        .tens(bcd_tens),
        .hundreds(bcd_hundreds),
        .thousands(bcd_thousands)
    );

    Latch_register latch_reg (
        .clk(clk),
        .rst(rst),
        .latch_en(latch_en),
        .in_units(bcd_units),
        .in_tens(bcd_tens),
        .in_hundreds(bcd_hundreds),
        .in_thousands(bcd_thousands),
        .out_units(latch_units),
        .out_tens(latch_tens),
        .out_hundreds(latch_hundreds),
        .out_thousands(latch_thousands)
    );

    always_ff @(posedge clk) cnt_display <= cnt_display + 1;

    disp7seg_controller dispA (
        .clk(cnt_display[13]),
        .bcd_dig('{latch_units, latch_tens, latch_hundreds, latch_thousands}),
        .blank_dig(4'b0000),
        .seg(D0_SEG),
        .dig_en(D0_AN)
    );

endmodule
