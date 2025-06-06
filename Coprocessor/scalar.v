module scalar (
    input [199:0] matrix_a,           
    input signed [7:0] integer_num,          
    input [1:0] matrix_size,                 
    output reg [199:0] new_matrix, 
    output reg overflow_flag                 
);

    wire [4:0] active_elements;              
    assign active_elements = (matrix_size == 2'b00) ? 4 :
                             (matrix_size == 2'b01) ? 9 :
                             (matrix_size == 2'b10) ? 16 :
                             25;

    wire signed [15:0] result [0:24];     
    wire overflow [0:24];                    

    genvar i;
    generate
        for (i = 0; i < 25; i = i + 1) begin : scalar_multiplication
            wire signed [7:0] matrix_element = matrix_a[i*8 +: 8];
            assign result[i] = bit_mult(matrix_element, integer_num);

            assign overflow[i] = (result[i][15:8] != {8{result[i][7]}}) ? 1'b1 : 1'b0;
        end
    endgenerate

    integer j;
    always @(*) begin
        new_matrix = 0;
        overflow_flag = 0;

        for (j = 0; j < 25; j = j + 1) begin
            if (j < active_elements) begin
                new_matrix[j*8 +: 8] = result[j][7:0];
                if (overflow[j]) overflow_flag = 1;
            end else begin
                new_matrix[j*8 +: 8] = 8'd0;
            end
        end
    end

    function signed [15:0] bit_mult;
        input signed [7:0] a, b;
        integer k;
        reg signed [15:0] acc;
        begin
            acc = 0;
            for (k = 0; k < 7; k = k + 1)
                if (b[k]) acc = acc + (a <<< k);
            if (b[7]) acc = acc - (a <<< 7);  
            bit_mult = acc;
        end
    endfunction

endmodule