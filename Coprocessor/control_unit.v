module control_unit (
    input  wire        clk,
    input  wire [31:0] data_in,
    output reg  [31:0] data_out
);

localparam IDLE      = 2'b00,
           RECEIVING  = 2'b01,
           PROCESSING = 2'b10,
           SENDING    = 2'b11;

reg [2:0] state;
reg       fpga_wait;
reg [2:0] hps_ready_sync;
reg       hps_ready_prev;

reg [4:0] index;
reg [2:0] op_code;
reg [1:0] matrix_size;
reg       start_flag;

// Matrizes internas renomeadas
reg [7:0] pixel [0:24];               // Entrada de imagem (unsigned)
reg signed [7:0] kernel [0:24];       // Filtro/kernel (signed)
reg signed [7:0] matrix_c [0:24];     // Terceira matriz opcional
reg signed [7:0] matrix_result [0:24]; // Resultado

// Versões flatten (200 bits)
wire [199:0] pixel_flat, kernel_flat, matrix_c_flat;
wire [199:0] matrix_out;
wire         done_signal;

// Decodificação do barramento de entrada
wire [7:0]  val_a      = data_in[7:0];
wire [7:0]  val_b      = data_in[15:8];
wire [2:0]  opcode_in  = data_in[18:16];
wire [1:0]  size_in    = data_in[20:19];
wire [7:0]  val_c      = data_in[28:21];
wire        reset      = data_in[29];
wire        start_in   = data_in[30];

integer i;

// Sincronização de borda do sinal do HPS
always @(posedge clk or posedge reset) begin
    if (reset) begin
        hps_ready_sync <= 3'b000;
        hps_ready_prev <= 1'b0;
    end else begin
        hps_ready_sync <= {hps_ready_sync[1:0], data_in[31]};
        hps_ready_prev <= hps_ready_sync[2];
    end
end

wire hps_ready_edge = hps_ready_sync[2] && !hps_ready_prev;

// FSM principal
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
            IDLE: begin
                if (start_in) begin
                    index <= 0;
                    state <= RECEIVING;
                end
            end

            RECEIVING: begin
                if (hps_ready_edge) begin
                    op_code     <= opcode_in;
                    matrix_size <= size_in;
                    pixel[index]  <= val_a;
                    kernel[index] <= val_b;
                    matrix_c[index] <= val_c;
                    index <= index + 1;
                    if (index == 24) begin
                        index <= 0;
                        state <= PROCESSING;
                    end
                end
            end

            PROCESSING: begin
                if (done_signal) begin
                    for (i = 0; i < 25; i = i + 1) begin
                        matrix_result[i] <= matrix_out[(i*8) +: 8];
                    end
                    index <= 0;
                    state <= SENDING;
                end
            end

            SENDING: begin
                if (hps_ready_edge) begin
                    if (index == 25) begin
                        state <= IDLE;
                    end
                    index <= index + 1;
                end
            end
        endcase

        fpga_wait <= ((state == RECEIVING) || (state == SENDING)) && hps_ready_sync[2];
    end
end

// Saída do módulo para o HPS
always @(posedge clk or posedge reset) begin
    if (reset) begin
        data_out <= 32'b0;
    end else begin
        data_out <= {fpga_wait, 23'b0, (state == SENDING) ? matrix_result[index-1] : 8'b0};
    end
end

// Conversão das matrizes para formato linear
generate
    genvar j;
    for (j = 0; j < 25; j = j + 1) begin : matrix_flatten
        assign pixel_flat[(j*8) +: 8]  = pixel[j];
        assign kernel_flat[(j*8) +: 8] = kernel[j];
        assign matrix_c_flat[(j*8) +: 8] = matrix_c[j];
    end
endgenerate

// Instância do coprocessador com novos nomes
coprocessor coprocessor (
    .op_code(op_code),
    .matrix_size(matrix_size),
    .pixel_data(pixel_flat),       // pixel = entrada da imagem
    .kernel_data(kernel_flat),      // kernel = filtro/convolução
    .process_Done(done_signal)
    .result_final(matrix_out),
);

endmodule
