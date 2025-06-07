module convolution (
    input [199:0] pixel,         // Entrada contendo até 25 pixels (unsigned, 8 bits cada)
    input [199:0] kernel,        // Kernel de convolução (signed, 8 bits cada)
    input [1:0] matrix_size,     // Tamanho da matriz: 00=2x2, 01=3x3, 10=4x4, 11=5x5
    output [199:0] result_out    // Saída: resultado da convolução com extensão para 200 bits
);

// Resultado final da convolução (signed, 16 bits)
wire signed [15:0] conv_result;

// Função auxiliar para extrair um pixel da matriz `pixel`.
// Cada pixel tem 8 bits, e o índice varia de 0 a 24.
// A matriz de entrada é tratada como um vetor linear de 200 bits.
function [7:0] get_pixel;
    input [199:0] matrix;
    input [4:0] index;
    begin
        get_pixel = matrix[(index*8) +: 8];
    end
endfunction

// Função auxiliar para extrair um valor do `kernel`, interpretando-o como signed.
// Os índices e estrutura são idênticos aos dos pixels.
function signed [7:0] get_kernel;
    input [199:0] matrix;
    input [4:0] index;
    begin
        get_kernel = matrix[(index*8) +: 8];
    end
endfunction

// Função que converte coordenadas bidimensionais (linha e coluna) 
// para um índice linear na matriz representada como vetor de 1 dimensão.
// A matriz sempre tem até 5 colunas (5x5).
function [4:0] get_index;
    input [2:0] row;
    input [2:0] col;
    begin
        get_index = row * 5 + col;
    end
endfunction

// Função que valida se as coordenadas estão dentro dos limites do kernel especificado.
// Essa verificação evita acessar posições inválidas dependendo do tamanho selecionado.
function is_valid_coord;
    input [2:0] row;
    input [2:0] col;
    input [1:0] size;
    begin
        case (size)
            2'b00: is_valid_coord = (row < 2) && (col < 2);  // Kernel 2x2
            2'b01: is_valid_coord = (row < 3) && (col < 3);  // Kernel 3x3
            2'b10: is_valid_coord = (row < 4) && (col < 4);  // Kernel 4x4
            2'b11: is_valid_coord = (row < 5) && (col < 5);  // Kernel 5x5
            default: is_valid_coord = 0;
        endcase
    end
endfunction

// Função principal que executa a operação de convolução:
// Para cada posição válida, multiplica o valor do pixel pelo valor do kernel e acumula.
function signed [15:0] image_convolution;
    input [199:0] pixels;
    input [199:0] kernel;
    input [1:0] size;
    reg signed [15:0] sum;         // Acumulador da soma dos produtos
    reg [7:0] pixel_val;           // Valor atual do pixel (unsigned)
    reg signed [7:0] kernel_val;   // Valor atual do kernel (signed)
    reg [2:0] row, col;            // Coordenadas da matriz
    reg [4:0] idx;                 // Índice linear (0 a 24)
    begin
        sum = 16'b0; // Inicializa a soma

        // Percorre a matriz 5x5 (máximo possível)
        for (row = 0; row < 5; row = row + 1) begin
            for (col = 0; col < 5; col = col + 1) begin
                if (is_valid_coord(row, col, size)) begin
                    idx = get_index(row, col);          // Converte coordenadas para índice
                    pixel_val = get_pixel(pixels, idx); // Extrai pixel
                    kernel_val = get_kernel(kernel, idx); // Extrai kernel

                    // Converte o pixel (unsigned) para signed de 9 bits e multiplica
                    sum = sum + ($signed({1'b0, pixel_val}) * kernel_val);
                end
            end
        end

        image_convolution = sum; // Retorna o resultado final
    end
endfunction

// Executa a convolução com os dados fornecidos
assign conv_result = image_convolution(pixel, kernel, matrix_size);

// A saída é de 200 bits, sendo que apenas os 16 bits menos significativos contêm o resultado.
// O restante é preenchido com zeros.
assign result_out = {184'b0, conv_result};

endmodule
