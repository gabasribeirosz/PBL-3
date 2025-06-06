module coprocessor (
    input [2:0] op_code,                   		// Código da operação a ser executada (adição, subtração, etc.)
    input [1:0] matrix_size,               		// Define o tamanho da matriz (2x2, 3x3, 4x4, 5x5)
    input [199:0] matrix_a,            			// Matriz A de entrada
    input [199:0] matrix_b,              			// Matriz B de entrada
    output reg process_Done,                    // Sinaliza que o processamento foi concluído
    output reg [199:0] result_final     			// Resultado final da operação                          
);

    // Resultados intermediários para cada operação
    wire [199:0] result_convolution; 

   // Instância dos módulos combinacionais que realizam as operações de matrix
    convolution convolution (
        .pixel(matrix_a),                  // Pixels da região
        .matrix_b(matrix_b),                  // Kernel/filtro
        .matrix_size(matrix_size),            // Tamanho do kernel
        .result_out(result_convolution)       // Resultado da convolução   
    );

	// Lógica de controle para selecionar a operação a ser realizada com base no código de operação (op_code)
	always @(*) begin
		case (op_code)
			3'b111: begin 
				result_final = result_convolution;    // Resultado da convolução
				process_Done = 1;                     
			end
			default: begin 
				result_final = 0;                       // Caso padrão, limpa o resultado
				process_Done = 0;                       // Processamento não concluído
			end
		endcase
	end

endmodule