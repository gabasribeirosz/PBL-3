module coprocessor (
    input [2:0] op_code,                   
    input [1:0] matrix_size,               
    input [199:0] matrix_a,            			 
    input [199:0] matrix_b,              			
    input signed [7:0] scalar_integer,                  
    output reg overflow,                        
    output reg process_done,                    
    output reg [199:0] final_result     			                         
);

    wire [199:0] add_result, sub_result, scalar_result, mult_result, opposite_result;
    wire add_overflow, sub_overflow, scalar_overflow, mult_overflow;  

   addiction matrix_adder (
        .matrix_a(matrix_a),
        .matrix_b(matrix_b),
        .matrix_size(matrix_size),
        .result_out(add_result),             
        .overflow(add_overflow)              
    );

   subtraction matrix_subtractor (
        .matrix_a(matrix_a),
        .matrix_b(matrix_b),
        .matrix_size(matrix_size),
        .result_out(sub_result),             
        .overflow(sub_overflow)              
    );

    scalar matrix_scalar_multiplier (
        .matrix_a(matrix_a),   
        .integer_num(scalar_integer),              
        .matrix_size(matrix_size),                   
        .new_matrix(scalar_result)       
    );

    multiplication matrix_multiplier (                      
        .matrix_a(matrix_a),                  
        .matrix_b(matrix_b),                  
        .result_out(mult_result),   
        .overflow_flag(mult_overflow) 
    );
	 
	 opposite opposite_matrix (
        .matrix_a(matrix_a),                 
        .matrix_size(matrix_size),            
        .opposite_matrix(opposite_result)
    );

	always @(*) begin
		case (op_code)
			3'b000: begin 
				final_result = add_result;            
				overflow = add_overflow;              
				process_done = 1;                     
			end

			3'b001: begin 
				final_result = sub_result;            
				overflow = sub_overflow;             
				process_done = 1;                     
			end

            3'b010: begin 
				final_result = scalar_result;       
				overflow = scalar_overflow;                         
				process_done = 1;                    
			end
			
            3'b011: begin
				final_result = mult_result;   
				overflow = mult_overflow;     
				process_done = 1;
			end
			
            3'b100: begin
				final_result = opposite_result;     
				overflow = 0;                         
				process_done = 1;                    
			end
			
            default: begin 
				final_result = 0;                    
				overflow = 0;                      
				process_done = 0;                       
			end
		endcase
	end

endmodule
