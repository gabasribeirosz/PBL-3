module coprocessor (
    input  [2:0]   op_code,         // Código que define qual operação será executada (ex: 111 para convolução)
    input  [1:0]   matrix_size,     // Tamanho do kernel/matriz (00=2x2, 01=3x3, 10=4x4, 11=5x5)
    input  [199:0] pixel_data,      // Dados da imagem (região de pixels a ser processada - unsigned)
    input  [199:0] kernel_data,     // Kernel/filtro convolucional a ser aplicado - valores com sinal (signed)
    output reg     processing_done, // Sinal que indica que a operação foi finalizada (ativo em '1')
    output reg [199:0] final_result // Resultado da operação solicitada (ex: resultado da convolução)
);

    // Saída intermediária gerada pelo módulo de convolução
    wire [199:0] convolution_result;

    // Instanciação do módulo de convolução.
    // Este módulo recebe os dados de imagem, o kernel e o tamanho da matriz,
    // realizando a multiplicação elemento a elemento e somando os resultados,
    // simulando uma operação de convolução comum em processamento de imagens.
    convolution convolution (
        .pixel(pixel_data),              // Dados da imagem de entrada
        .kernel(kernel_data),            // Dados do kernel/filtro convolucional
        .matrix_size(matrix_size),       // Tamanho do kernel (controla lógica interna da convolução)
        .result_out(convolution_result)  // Resultado da convolução (mesmo tamanho dos dados de entrada)
    );

    // Bloco sempre sensível a qualquer mudança nas entradas (combinacional)
    // Responsável por selecionar a operação a ser executada com base em 'op_code'.
    // Atualmente, apenas a operação de convolução está implementada.
    always @(*) begin
        case (op_code)
            3'b111: begin
                // Quando o código for 111, executa a operação de convolução
                final_result     = convolution_result; // Atribui resultado da convolução à saída final
                processing_done  = 1'b1;               // Sinaliza que o processamento foi concluído
            end
            default: begin
                // Para quaisquer outros códigos, nenhuma operação é realizada
                final_result     = 200'b0;             // Zera a saída final
                processing_done  = 1'b0;               // Indica que não houve processamento
            end
        endcase
    end

endmodule
