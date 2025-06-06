module convolution (
    input [199:0] pixel, // Pixels da região (valores unsigned 0-255)
    input [199:0] matrix_b, // Kernel/filtro (valores signed, podem ser negativos)
    input [1:0] matrix_size, // 00=2x2, 01=3x3, 10=4x4, 11=5x5
    output [199:0] result_out
);

// Wire para armazenar o resultado da convolução
wire signed [15:0] conv_result;

// Função para extrair pixel (unsigned)
function [7:0] get_pixel;
    input [199:0] matrix;
    input [4:0] index;
    begin
        get_pixel = matrix[(index*8) +: 8];
    end
endfunction

// Função para extrair valor do kernel (signed)
function signed [7:0] get_kernel;
    input [199:0] matrix;
    input [4:0] index;
    begin
        get_kernel = matrix[(index*8) +: 8];
    end
endfunction

// Função para mapear coordenadas (row, col) para índice linear
function [4:0] get_index;
    input [2:0] row;
    input [2:0] col;
    begin
        get_index = row * 5 + col; // Sempre mapeia como 5x5
    end
endfunction

// Função para verificar se coordenada está dentro dos limites
function is_valid_coord;
    input [2:0] row;
    input [2:0] col;
    input [1:0] size;
    begin
        case (size)
            2'b00: is_valid_coord = (row < 2) && (col < 2); // 2x2
            2'b01: is_valid_coord = (row < 3) && (col < 3); // 3x3
            2'b10: is_valid_coord = (row < 4) && (col < 4); // 4x4
            2'b11: is_valid_coord = (row < 5) && (col < 5); // 5x5
            default: is_valid_coord = 0;
        endcase
    end
endfunction

// Função de convolução para processamento de imagem
function signed [15:0] image_convolution;
    input [199:0] pixels;
    input [199:0] kernel;
    input [1:0] size;
    reg signed [15:0] sum;
    reg [7:0] pixel_val; // Pixel sempre unsigned
    reg signed [7:0] kernel_val; // Kernel pode ser signed
    reg [2:0] row, col;
    reg [4:0] idx;
    begin
        sum = 16'b0;
        // Loop através de todas as posições possíveis (até 5x5)
        for (row = 0; row < 5; row = row + 1) begin
            for (col = 0; col < 5; col = col + 1) begin
                // Verifica se a coordenada está dentro dos limites do tamanho especificado
                if (is_valid_coord(row, col, size)) begin
                    idx = get_index(row, col);
                    pixel_val = get_pixel(pixels, idx);
                    kernel_val = get_kernel(kernel, idx);
                    // Converte pixel unsigned para signed e multiplica pelo kernel
                    // Pixel: 0-255 → Signed: 0-255 (sem sinal negativo)
                    sum = sum + ($signed({1'b0, pixel_val}) * kernel_val);
                end
            end
        end
        image_convolution = sum;
    end
endfunction

assign conv_result = image_convolution(pixel, matrix_b, matrix_size);

assign result_out = {184'b0, conv_result};

endmodule