module subtraction ( 
    input [199:0] matrix_a,      		
    input [199:0] matrix_b,      		
    input [1:0] matrix_size,            		
    output reg [199:0] result_out, 		
    output reg overflow                 		
);

    wire [4:0] active_elements;

    assign active_elements = (matrix_size == 2'b00) ? 4 :
                             (matrix_size == 2'b01) ? 9 :
                             (matrix_size == 2'b10) ? 16 :
                             25;

    wire signed [8:0] subtraction [0:24];            			
    wire overflow_check [0:24];        					

    genvar i;
    generate
        for (i = 0; i < 25; i = i + 1) begin : subtraction_and_check
            assign subtraction[i] = matrix_a[i*8 +: 8] - matrix_b[i*8 +: 8];  

            assign overflow_check[i] = (matrix_a[i*8+7] != matrix_b[i*8+7]) &&
                                       (subtraction[i][8] != matrix_a[i*8+7]);
        end
    endgenerate

    integer j;
    always @(*) begin
        overflow = 0;           
        result_out = 0;         

        for (j = 0; j < 25; j = j + 1) begin
            if (j < active_elements) begin
                result_out[j*8 +: 8] = subtraction[j][7:0];     
                if (overflow_check[j]) overflow = 1;     
            end else begin
                result_out[j*8 +: 8] = 8'd0;            
            end
        end
    end

endmodule