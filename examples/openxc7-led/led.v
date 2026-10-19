module led (
    input  clk,
    output reg led
);

reg [31:0] cnt;

always @(posedge clk) begin
    if (cnt > 50000000) begin
        cnt <= 0;
        led <= ~led;
    end else begin
        cnt <= cnt + 1;
    end
end

initial begin
    cnt <= 0;
    led <= 0;
end

endmodule
