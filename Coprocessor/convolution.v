module convolution (
    input [199:0] pixel,         // Entrada contendo até 25 pixels (unsigned, 8 bits cada)
    input [199:0] kernel,        // Kernel de convolução (signed, 8 bits cada)
    input [1:0] matrix_size,     // Tamanho da matriz: 00=2x2, 01=3x3, 10=4x4, 11=5x5
    output [199:0] result_out    // Saída: resultado da convolução com extensão para 200 bits
);

wire signed [15:0] conv_result; // Resultado intermediário da operação de convolução

// Função que extrai um pixel de 8 bits a partir do índice na matriz de entrada (unsigned)
function [7:0] get_pixel;
    input [199:0] matrix;
    input [4:0] index;
    begin
        get_pixel = matrix[(index*8) +: 8];
    end
endfunction

// Função que extrai um valor de 8 bits do kernel, interpretando como número com sinal (signed)
function signed [7:0] get_kernel;
    input [199:0] matrix;
    input [4:0] index;
    begin
        get_kernel = matrix[(index*8) +: 8];
    end
endfunction

// Função que converte coordenadas 2D (linha, coluna) para índice linear (0 a 24) na matriz 5x5
function [4:0] get_index;
    input [2:0] row;
    input [2:0] col;
    begin
        get_index = row * 5 + col;
    end
endfunction

// Função que verifica se uma coordenada (linha, coluna) está dentro dos limites da matriz usada
function is_valid_coord;
    input [2:0] row;
    input [2:0] col;
    input [1:0] size;
    begin
        case (size)
            2'b00: is_valid_coord = (row < 2) && (col < 2);  // 2x2
            2'b01: is_valid_coord = (row < 3) && (col < 3);  // 3x3
            2'b10: is_valid_coord = (row < 4) && (col < 4);  // 4x4
            2'b11: is_valid_coord = (row < 5) && (col < 5);  // 5x5
            default: is_valid_coord = 0;
        endcase
    end
endfunction

// Função principal que realiza a convolução: soma dos produtos pixel * kernel
function signed [15:0] image_convolution;
    input [199:0] pixels;
    input [199:0] kernel;
    input [1:0] size;
    reg signed [15:0] sum;
    reg [7:0] pixel_val;
    reg signed [7:0] kernel_val;
    reg [2:0] row, col;
    reg [4:0] idx;
    begin
        sum = 16'b0;

        // Percorre a matriz 5x5, considerando apenas os elementos válidos com base no tamanho
        for (row = 0; row < 5; row = row + 1) begin
            for (col = 0; col < 5; col = col + 1) begin
                if (is_valid_coord(row, col, size)) begin
                    idx = get_index(row, col);
                    pixel_val = get_pixel(pixels, idx);
                    kernel_val = get_kernel(kernel, idx);

                    // Converte pixel (unsigned) para signed com 9 bits e realiza a multiplicação
                    sum = sum + ($signed({1'b0, pixel_val}) * kernel_val);
                end
            end
        end

        image_convolution = sum;
    end
endfunction

// Aplica a convolução e armazena o resultado
assign conv_result = image_convolution(pixel, kernel, matrix_size);

// Saída de 200 bits, com o resultado nos 16 bits menos significativos
assign result_out = {184'b0, conv_result};

endmodule
