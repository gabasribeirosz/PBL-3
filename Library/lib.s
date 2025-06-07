.equ DELAY_CYCLES, 10              @ Define um valor de delay entre sinais para sincronização

.section .data
devmem_path: .asciz "/dev/mem"    @ Caminho do dispositivo para acessar memória física
LW_BRIDGE_BASE: .word 0xff200     @ Endereço base da ponte LW (Lightweight Bridge)
LW_BRIDGE_SPAN: .word 0x1000      @ Tamanho da região de mapeamento da ponte LW

.global data_in_ptr
data_in_ptr: .word 0              @ Ponteiro para a interface de entrada da FPGA

.global data_out_ptr
data_out_ptr: .word 0             @ Ponteiro para a interface de saída da FPGA

.global fd_mem 
fd_mem: .space 4                  @ Espaço para armazenar o descritor de arquivo de /dev/mem

.section .text  

.global initiate_hardware
.type initiate_hardware, %function
initiate_hardware:
    PUSH {r1-r7, lr}                     @ Salva registradores temporários na pilha

    @ --- Abre /dev/mem ---
    MOV r7, #5                           @ Código da syscall open
    LDR r0, =devmem_path                @ Primeiro argumento: caminho para o arquivo
    MOV r1, #2                          @ Segundo argumento: modo de abertura (O_RDWR)
    MOV r2, #0                          @ Terceiro argumento: flags (não usadas aqui)
    SVC 0                               @ Chamada de sistema

    CMP r0, #0                          @ Verifica se a abertura foi bem sucedida
    BLT fail_open                       @ Se r0 < 0, falha → encerra

    LDR r1, =fd_mem                     @ Salva file descriptor retornado
    STR r0, [r1]
    MOV r4, r0                          @ Armazena fd em r4 para reuso

    @ --- Mapeia memória da ponte LW ---
    MOV r7, #192                        @ Código da syscall mmap2
    MOV r0, #0                          @ Endereço sugerido = NULL (deixa o SO decidir)
    LDR r1, =LW_BRIDGE_SPAN
    LDR r1, [r1]                        @ Tamanho a mapear
    MOV r2, #3                          @ PROT_READ | PROT_WRITE
    MOV r3, #1                          @ MAP_SHARED
    LDR r5, =LW_BRIDGE_BASE
    LDR r5, [r5]                        @ Offset dentro de /dev/mem
    SVC 0

    CMP r0, #-1                         @ Verifica se o mapeamento falhou
    BEQ fail_mmap

    @ Configura ponteiros para interfaces de entrada e saída da FPGA
    LDR r1, =data_in_ptr
    STR r0, [r1]                        @ Salva ponteiro base (data_in)

    ADD r1, r0, #0x10                   @ Offset para data_out
    LDR r2, =data_out_ptr
    STR r1, [r2]

    MOV r0, #0                          @ Sucesso
    B end_init

fail_open:
    MOV r7, #1                          @ Código da syscall exit
    MOV r0, #1                          @ Código de erro 1
    SVC #0
    B end_init

fail_mmap:
    MOV r7, #1                          @ exit
    MOV r0, #2                          @ Código de erro 2
    SVC #0

end_init:
    POP {r1-r7, lr}                     @ Restaura registradores
    BX lr                               @ Retorna da função

@ Libera recursos usados por initiate_hardware
.global terminate_hardware
.type terminate_hardware, %function
terminate_hardware:
    PUSH {r4-r7, lr}

    @ Verifica se há ponteiro válido mapeado
    LDR r0, =data_in_ptr
    LDR r0, [r0]
    CMP r0, #0
    BEQ skip_munmap

    @ Desmapeia a memória
    MOV r7, #91                         @ syscall munmap
    LDR r1, =LW_BRIDGE_SPAN
    LDR r1, [r1]
    SVC 0

    @ Zera os ponteiros
    MOV r4, #0
    LDR r5, =data_in_ptr
    STR r4, [r5]
    LDR r5, =data_out_ptr
    STR r4, [r5]

skip_munmap:
    @ Fecha descritor de arquivo se válido
    LDR r0, =fd_mem
    LDR r0, [r0]
    CMP r0, #0
    BLE skip_close

    MOV r7, #6                          @ syscall close
    SVC 0

    MOV r4, #-1                         @ Marca fd como inválido
    LDR r5, =fd_mem
    STR r4, [r5]

skip_close:
    MOV r0, #0
    POP {r4-r7, lr}
    BX lr

@ Envia todos os dados à FPGA
.global transfer_data_to_fpga
.type transfer_data_to_fpga, %function
transfer_data_to_fpga:
    PUSH {r4-r11, lr}

    @ Carrega parâmetros (R0: ponteiro para struct)
    LDR r4, [r0]                        @ matriz A
    LDR r5, [r0, #4]                    @ matriz B
    LDR r6, [r0, #8]                    @ opcode
    LDR r7, [r0, #12]                   @ parâmetro extra (flags)

    @ Gera pulso de reset e start
    LDR r2, =data_in_ptr
    LDR r2, [r2]

    MOV r9, #1
    LSL r9, r9, #29                     @ Bit 29 = reset
    STR r9, [r2]
    MOV r0, #0
    STR r0, [r2]

    MOV r11, #DELAY_CYCLES             @ Delay
    BL delay_loop

    MOV r9, #1
    LSL r9, r9, #30                     @ Bit 30 = start
    STR r9, [r2]
    MOV r0, #0
    STR r0, [r2]

    MOV r9, #25                         @ Tamanho dos dados
    MOV r10, #0                         @ Índice

loop_send:
    CMP r10, r9
    BGE end_send

    @ Lê elementos de A e B
    LDRB r0, [r4, r10]
    LDRSB r1, [r5, r10]

    AND r0, r0, #0xFF
    AND r1, r1, #0xFF

    LSL r1, r1, #8
    ORR r0, r0, r1                      @ Combina A e B (B nos bits 8-15)

    ORR r0, r0, r6, LSL #16             @ Opcode (bits 16-18)
    ORR r0, r0, r7, LSL #19             @ Flags (bits 19+)

    PUSH {r0}
    MOV r1, #1
    BL handshake_send                   @ Envia com handshake
    POP {r0}

    ADD r10, r10, #1
    B loop_send

end_send:
    MOV r0, #0
    POP {r4-r11, lr}
    BX lr

@ Delay simples (espera ocupada)
delay_loop:
    SUBS r11, r11, #1
    BNE delay_loop
    BX lr

@ Lê todos os resultados da FPGA (result: uint8_t*)
.global retrieve_fpga_results
.type retrieve_fpga_results, %function
retrieve_fpga_results:
    PUSH {r4-r7, lr}

    MOV r4, r0                          @ Ponteiro de destino
    MOV r6, #25                         @ Tamanho da saída
    MOV r7, #0                          @ Índice

.loop_recv:
    CMP r7, r6
    BGE .done

    MOV r0, r4
    ADD r0, r0, r7                      @ Próxima posição de escrita
    BL handshake_receive

    CMP r0, #0
    BNE .error

    ADD r7, r7, #1
    B .loop_recv

.error:
    MOV r0, #1
    B .exit

.done:
    MOV r0, #0

.exit:
    POP {r4-r7, lr}
    BX lr

@ void handshake_send(uint32_t value)
handshake_send:
    PUSH {r1-r4, lr}
    LDR r1, =data_in_ptr
    LDR r1, [r1]
    LDR r2, =data_out_ptr
    LDR r2, [r2]

    @ Passo 1: Envia valor com bit 31 = 1 (ready)
    ORR r3, r0, #(1 << 31)
    STR r3, [r1]

.wait_ack_high_send:
    LDR r4, [r2]
    TST r4, #(1 << 31)
    BEQ .wait_ack_high_send            @ Aguarda ack da FPGA

    MOV r3, #0
    STR r3, [r1]                        @ Passo 3: Confirma recepção

.wait_ack_low_send:
    LDR r4, [r2]
    TST r4, #(1 << 31)
    BNE .wait_ack_low_send             @ Aguarda fim do ack

    POP {r1-r4, lr}
    BX lr

@ int handshake_receive(uint8_t* value_out)
handshake_receive:
    PUSH {r2-r5, lr}
    LDR r2, =data_in_ptr
    LDR r2, [r2]
    LDR r3, =data_out_ptr
    LDR r3, [r3]

    CMP r2, #0
    BEQ .handshake_error
    CMP r3, #0
    BEQ .handshake_error

    @ Envia ready para FPGA
    MOV r4, #(1 << 31)
    STR r4, [r2]

.wait_ack_high_recei:
    LDR r5, [r3]
    TST r5, #(1 << 31)
    BEQ .wait_ack_high_recei

    AND r4, r5, #0xFF                   @ Extrai os dados (bits 0-7)
    STRB r4, [r0]

    MOV r4, #0
    STR r4, [r2]

.wait_ack_low_recei:
    LDR r5, [r3]
    TST r5, #(1 << 31)
    BNE .wait_ack_low_recei

    MOV r0, #0
    B .handshake_exit

.handshake_error:
    MOV r0, #1

.handshake_exit:
    POP {r2-r5, lr}
    BX lr

