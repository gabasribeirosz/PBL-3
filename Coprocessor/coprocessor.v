module coprocessor (
    input  [2:0]   op_code,         // Código que define qual operação será executada
    input  [1:0]   matrix_size,     // Tamanho do kernel/matriz (00=2x2, 01=3x3, etc.)
    input  [199:0] pixel_data,      // Dados da imagem (região de pixels - unsigned)
    input  [199:0] kernel_data,     // Kernel/filtro convolucional (signed)
    output reg     processing_done, // Sinal que indica que a operação foi finalizada
    output reg [199:0] final_result // Resultado da operação selecionada
);

    // Saída intermediária do módulo de convolução
    wire [199:0] convolution_result;

    // Instância do módulo de convolução que realiza o cálculo pixel × kernel
    convolution convolution (
        .pixel(pixel_data),              // Dados da imagem
        .kernel(kernel_data),            // Filtro/kernel
        .matrix_size(matrix_size),       // Tamanho do kernel (2x2 a 5x5)
        .result_out(convolution_result)  // Resultado da operação de convolução
    );

    // Seleção da operação a partir do código fornecido (por enquanto, apenas convolução)
    always @(*) begin
        case (op_code)
            3'b111: begin
                final_result     = convolution_result; // Operação: convolução
                processing_done  = 1'b1;               // Marca como concluído
            end
            default: begin
                final_result     = 200'b0;             // Resultado zerado
                processing_done  = 1'b0;               // Processamento inativo
            end
        endcase
    end

endmodule
