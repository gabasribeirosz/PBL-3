.equ DELAY_CYCLES, 10

.section .data
devmem_path: .asciz "/dev/mem"
LW_BRIDGE_BASE: .word 0xff200
LW_BRIDGE_SPAN: .word 0x1000

.global data_in_ptr
data_in_ptr: .word 0         @ ponteiro para base do data_in

.global data_out_ptr
data_out_ptr: .word 0       @ ponteiro para base do data_out

.global fd_mem 
fd_mem: .space 4              @ file descriptor do open()

.section .text  

.global initiate_hardware
.type initiate_hardware, %function
initiate_hardware:
    @salva os valores dos registradores na pilha
    PUSH {r1-r7, lr}
    @ --- Abre /dev/mem ---
    MOV r7, #5      @ open
    LDR r0, =devmem_path
    MOV r1, #2
    MOV r2, #0
    SVC 0

    CMP r0, #0
    BLT fail_open
    
    @ --- Salva file descriptor ---
    LDR r1, =fd_mem
    str r0, [r1]
    mov r4, r0              @guarda em r4

    @ --- Mapeia memória ---
    MOV r7, #192    @ mmap2
    MOV r0, #0
    LDR r1, =LW_BRIDGE_SPAN
    LDR r1, [r1]
    MOV r2, #3
    MOV r3, #1
    LDR r5, =LW_BRIDGE_BASE
    LDR r5, [r5]
    SVC 0

    CMP r0, #-1
    BEQ fail_mmap

    LDR r1, =data_in_ptr    
    STR r0, [r1]
    ADD r1, r0, #0x10
    LDR r2, =data_out_ptr
    STR r1, [r2]

    MOV r0, #0
    B end_init

fail_open:
    mov r7, #1
    mov r0, #1
    svc #0
    B end_init

fail_mmap:
    mov r7, #1
    mov r0, #2
    svc #0

end_init:
    POP {r1-r7, lr}
    BX lr

@ Libera os recursos alocados por init_hw_access
.global terminate_hardware
.type terminate_hardware, %function
terminate_hardware:
    PUSH {r4-r7, lr}
    @ Verifica se o ponteiro mapeado é válido
    LDR r0, =data_in_ptr
    LDR r0, [r0]
    CMP r0, #0
    BEQ skip_munmap   
    @ Desmapeia a memória
    MOV r7, #91     
    LDR r1, =LW_BRIDGE_SPAN
    LDR r1, [r1]      
    SVC 0
    @ Limpa os ponteiros após o munmap
    MOV r4, #0
    LDR r5, =data_in_ptr
    STR r4, [r5]
    LDR r5, =data_out_ptr
    STR r4, [r5]

skip_munmap:
    @ Verifica se o descritor de arquivo é válido
    LDR r0, =fd_mem
    LDR r0, [r0]
    CMP r0, #0
    BLE skip_close   
    @ Fecha o descritor de arquivo
    MOV r7, #6        
    SVC 0
    @ Limpa o descritor de arquivo
    MOV r4, #-1
    LDR r5, =fd_mem
    STR r4, [r5]

skip_close:
    MOV r0, #0       
    POP {r4-r7, lr}
    BX lr

@ void send_all_data(*params)
.global transfer_data_to_fpga
.type transfer_data_to_fpga, %function
transfer_data_to_fpga:
    PUSH {r4-r11, lr}
    @ R0 = ponteiro para struct Params
    LDR r4, [r0]            
    LDR r5, [r0, #4]        
    LDR r6, [r0, #8]        
    LDR r7, [r0, #12]  

    @ Envia pulso de reset e de start para o módulo da FPGA
    LDR r2, =data_in_ptr
    LDR r2, [r2]

    MOV r9, #1
    LSL r9, r9, #29
    MOV r0, r9              @ r0 = reset bit (bit 29 = 1)
    STR r0, [r2]            
    MOV r0, #0
    STR r0, [r2]  

    @ --- Delay entre o sinal de reset e start          
    MOV r11, #DELAY_CYCLES              
    BL delay_loop  

    MOV r9, #1
    LSL r9, r9, #30
    MOV r0, r9              @ r0 = start bit (bit 30 = 1)
    STR r0, [r2]            
    MOV r0, #0
    STR r0, [r2] 

    @ Inicializa contadores
    MOV r9, #25             
    MOV r10, #0  

loop_send:
    CMP r10, r9
    BGE end_send  

    @ Carrega valores das matrizes
    LDRB r0, [r4, r10]     
    LDRSB r1, [r5, r10] 

    AND r0, r0, #0xFF       
    AND r1, r1, #0xFF   

    LSL r1, r1, #8          
    ORR r0, r0, r1   

    @ Adiciona os campos de controle
    ORR r0, r0, r6, LSL #16 
    ORR r0, r0, r7, LSL #19 
    
    PUSH {r0}              
    MOV r1, #1              
    BL handshake_send       
    POP {r0}                
    ADD r10, r10, #1
    B loop_send

end_send:
    MOV r0, #0              @ Retorna sucesso
    POP {r4-r11, lr}
    BX lr

delay_loop:
    SUBS r11, r11, #1
    BNE delay_loop
    BX lr

@ int read_all_results(uint8_t* result)
.global retrieve_fpga_results
.type retrieve_fpga_results, %function
retrieve_fpga_results:
    PUSH {r4-r7, lr}
    MOV r4, r0           
    MOV r6, #25          
    MOV r7, #0  

.loop_recv:
    CMP r7, r6
    BGE .done            
    MOV r0, r4
    ADD r0, r0, r7       
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
    @ Lê ponteiro para data_out
    LDR r2, =data_out_ptr
    LDR r2, [r2]          
    @ --- Etapa 1: Escreve valor com bit 31 ligado ---
    ORR r3, r0, #(1 << 31)     
    STR r3, [r1]               
    @ --- Etapa 2: Espera FPGA_ACK = 1 ---

.wait_ack_high_send:
    LDR r4, [r2]               
    TST r4, #(1 << 31)         
    BEQ .wait_ack_high_send         
    @ --- Etapa 3: Confirma recebimento → escreve 0 ---
    MOV r3, #0
    STR r3, [r1]               
    @ --- Etapa 4: Espera FPGA_ACK = 0 ---

.wait_ack_low_send:
    LDR r4, [r2]
    TST r4, #(1 << 31)
    BNE .wait_ack_low_send          
    @ --- Fim ---
    POP {r1-r4, lr}
    BX lr

@ int handshake_receive(uint8_t* value_out)
handshake_receive:
    PUSH {r2-r5, lr}
    @ Carrega endereços dos registradores de interface com a FPGA
    LDR r2, =data_in_ptr     
    LDR r2, [r2]
    LDR r3, =data_out_ptr    
    LDR r3, [r3]
    @ Verifica se os ponteiros são válidos
    CMP r2, #0
    BEQ .handshake_error     
    CMP r3, #0
    BEQ .handshake_error     
    @ Envia sinal de pronto para FPGA
    MOV r4, #(1 << 31)       
    STR r4, [r2]             
    @ Aguarda FPGA sinalizar que enviou dados (bit 31=1 em data_out)

.wait_ack_high_recei:
    LDR r5, [r3]             
    TST r5, #(1 << 31)       
    BEQ .wait_ack_high_recei 
    @ Extrai o valor da matriz (bits [7:0])
    AND r4, r5, #0xFF        
    STRB r4, [r0]            
    @ Confirma a leitura desativando o bit de controle
    MOV r4, #0
    STR r4, [r2]             
    @ Aguarda FPGA desativar seu sinal de pronto

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
 