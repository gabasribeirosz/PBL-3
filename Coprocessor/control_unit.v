module control_unit (
    input  wire        clk,        // Clock principal do sistema
    input  wire [31:0] data_in,    // Dados de entrada vindos do HPS
    output reg  [31:0] data_out    // Dados de saída para o HPS
);

// Definição dos estados da máquina de estados finita (FSM)
localparam IDLE      = 2'b00,  // Estado ocioso: aguardando início
           RECEIVING  = 2'b01, // Recebendo dados do HPS
           PROCESSING = 2'b10, // Processando os dados com o coprocessador
           SENDING    = 2'b11; // Enviando os resultados de volta para o HPS

reg [2:0] state;           // Estado atual da FSM
reg       fpga_wait;       // Flag de espera: ativa quando o FPGA está ocupado
reg [2:0] hps_ready_sync;  // Registradores para sincronizar o sinal hps_ready com o clock
reg       hps_ready_prev;  // Estado anterior de hps_ready para detecção de borda

reg [4:0] index;           // Índice para acessar posições das matrizes (0 a 24)
reg [2:0] op_code;         // Código da operação a ser realizada
reg [1:0] matrix_size;     // Tamanho da matriz/kernel
reg       start_flag;      // Flag de início (não utilizada no momento)

// Matrizes internas utilizadas no processamento
reg [7:0] pixel [0:24];                 // Matriz de pixels (unsigned)
reg signed [7:0] kernel [0:24];         // Matriz do kernel/filtro (signed)
reg signed [7:0] matrix_c [0:24];       // Matriz opcional (pode ser usada para futuras operações)
reg signed [7:0] matrix_result [0:24];  // Resultado do processamento

// Representações lineares (flattened) das matrizes - 25 elementos × 8 bits = 200 bits
wire [199:0] pixel_flat, kernel_flat, matrix_c_flat;
wire [199:0] matrix_out;   // Resultado do coprocessador em formato linear
wire         done_signal;  // Sinal de término do processamento do coprocessador

// Decodificação dos campos do barramento de entrada (data_in)
wire [7:0]  val_a      = data_in[7:0];     // Pixel da imagem
wire [7:0]  val_b      = data_in[15:8];    // Valor do kernel
wire [2:0]  opcode_in  = data_in[18:16];   // Código da operação
wire [1:0]  size_in    = data_in[20:19];   // Tamanho da matriz
wire [7:0]  val_c      = data_in[28:21];   // Valor opcional (matrix_c)
wire        reset      = data_in[29];      // Sinal de reset
wire        start_in   = data_in[30];      // Comando de início da operação

integer i; // Usado para loops

// Sincronização do sinal de prontidão vindo do HPS (bit 31 de data_in)
// Detecta borda de subida usando registradores deslocadores
always @(posedge clk or posedge reset) begin
    if (reset) begin
        hps_ready_sync <= 3'b000;
        hps_ready_prev <= 1'b0;
    end else begin
        hps_ready_sync <= {hps_ready_sync[1:0], data_in[31]}; // Shift register para detectar borda
        hps_ready_prev <= hps_ready_sync[2];
    end
end

// Detecta a borda de subida do sinal hps_ready (1 → 0 → 1)
wire hps_ready_edge = hps_ready_sync[2] && !hps_ready_prev;

// FSM principal responsável por controlar o fluxo de dados
always @(posedge clk or posedge reset) begin : main_fsm
    if (reset) begin
        state <= IDLE;
        index <= 0;
        fpga_wait <= 0;
        for (i = 0; i < 25; i = i + 1) begin
            pixel[i]  <= 8'b0;
            kernel[i] <= 8'b0;
        end
    end else begin
        case (state)
            // Estado ocioso, aguardando o sinal de start_in do HPS
            IDLE: begin
                if (start_in) begin
                    index <= 0;
                    state <= RECEIVING;
                end
            end

            // Recebendo dados de pixel, kernel e matriz C do HPS
            RECEIVING: begin
                if (hps_ready_edge) begin
                    op_code     <= opcode_in;
                    matrix_size <= size_in;
                    pixel[index]    <= val_a;  // Recebe pixel
                    kernel[index]   <= val_b;  // Recebe kernel
                    matrix_c[index] <= val_c;  // Recebe valor extra
                    index <= index + 1;
                    if (index == 24) begin
                        index <= 0;
                        state <= PROCESSING;
                    end
                end
            end

            // Aguarda o sinal de término do coprocessador
            PROCESSING: begin
                if (done_signal) begin
                    for (i = 0; i < 25; i = i + 1) begin
                        matrix_result[i] <= matrix_out[(i*8) +: 8]; // Extrai os bytes do resultado
                    end
                    index <= 0;
                    state <= SENDING;
                end
            end

            // Enviando os resultados da matriz processada para o HPS
            SENDING: begin
                if (hps_ready_edge) begin
                    if (index == 25) begin
                        state <= IDLE; // Envio completo
                    end
                    index <= index + 1;
                end
            end
        endcase

        // Indica se o FPGA está esperando o HPS para enviar/receber dados
        fpga_wait <= ((state == RECEIVING) || (state == SENDING)) && hps_ready_sync[2];
    end
end

// Lógica de saída: envia os dados da matriz processada ou mantém zero
always @(posedge clk or posedge reset) begin
    if (reset) begin
        data_out <= 32'b0;
    end else begin
        data_out <= {
            fpga_wait,                 // Bit 31: FPGA está esperando
            23'b0,                     // Bits 30 a 8: reservados/zerados
            (state == SENDING) ? matrix_result[index-1] : 8'b0 // Bits 7 a 0: resultado atual
        };
    end
end

// Conversão das matrizes multidimensionais para vetores lineares de 200 bits
generate
    genvar j;
    for (j = 0; j < 25; j = j + 1) begin : matrix_flatten
        assign pixel_flat[(j*8) +: 8]     = pixel[j];
        assign kernel_flat[(j*8) +: 8]    = kernel[j];
        assign matrix_c_flat[(j*8) +: 8]  = matrix_c[j];
    end
endgenerate

// Instanciação do módulo coprocessor, que realiza a operação selecionada
coprocessor coprocessor (
    .op_code(op_code),              // Código da operação (ex: convolução)
    .matrix_size(matrix_size),      // Tamanho da matriz (2x2 a 5x5)
    .pixel_data(pixel_flat),        // Matriz de pixels linearizada
    .kernel_data(kernel_flat),      // Kernel linearizado
    .process_Done(done_signal),     // Indica que o processamento foi concluído
    .result_final(matrix_out)       // Resultado linearizado da operação
);

endmodule
