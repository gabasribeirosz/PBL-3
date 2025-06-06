module addiction (
    input [199:0] matrix_a,      			
    input [199:0] matrix_b,      			
    input [1:0] matrix_size,            	
    output reg [199:0] result_out, 			
    output reg overflow                 	
);

    wire signed [8:0] sum [0:24];               
    wire overflow_check [0:24];                 
    wire [4:0] active_elements;                 

    assign active_elements = (matrix_size == 2'b00) ? 4 :
                             (matrix_size == 2'b01) ? 9 :
                             (matrix_size == 2'b10) ? 16 :
                             25;

    genvar i;
    generate
        for (i = 0; i < 25; i = i + 1) begin : adder_loop
            wire signed [7:0] element_a = matrix_a[i*8 +: 8];
            wire signed [7:0] element_b = matrix_b[i*8 +: 8];

            assign sum[i] = element_a + element_b;

           assign overflow_check[i] = (element_a[7] == element_b[7]) && (sum[i][7] != element_a[7]);
        end
    endgenerate

    integer j;
    always @(*) begin
        overflow = 0;
        result_out = 0;

        for (j = 0; j < 25; j = j + 1) begin
            if (j < active_elements) begin
                result_out[j*8 +: 8] = sum[j][7:0];
                if (overflow_check[j]) overflow = 1;
            end else begin
                result_out[j*8 +: 8] = 8'd0;
            end
        end
    end
endmodule