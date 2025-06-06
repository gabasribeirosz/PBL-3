#ifndef HPS_0_H
#define HPS_0_H
#include <stdint.h>

/* ========== CONSTANTES DE STATUS ========== */
#define MATRIX_SIZE 25
#define HW_SUCCESS      0
#define HW_SEND_FAIL   -1

/* ========== ESTRUTURAS DE DADOS ========== */
struct Params {
    const uint8_t* a;
    const int8_t* b;
    uint32_t opcode;
    uint32_t size; 
};

/* ========== DECLARAÇÕES DE FUNÇÕES ASSEMBLY ========== */
extern int initiate_hardware(void);
extern int terminate_hardware(void);
extern int transfer_data_to_fpga(const struct Params* p);
extern int retrieve_fpga_results(uint8_t* result);

/* ========== KERNELS DOS FILTROS DE BORDA ========== */

// Sobel 3x3 (mapeado para matriz 5x5 com zeros)
extern int8_t sobel_gx_3x3[MATRIX_SIZE];
extern int8_t sobel_gy_3x3[MATRIX_SIZE];

// Sobel 5x5
extern int8_t sobel_gx_5x5[MATRIX_SIZE];
extern int8_t sobel_gy_5x5[MATRIX_SIZE];

// Prewitt 3x3 (mapeado para matriz 5x5 com zeros)
extern int8_t prewitt_gx_3x3[MATRIX_SIZE];
extern int8_t prewitt_gy_3x3[MATRIX_SIZE];

// Roberts 2x2 (mapeado para matriz 5x5 com zeros)
extern int8_t roberts_gx_2x2[MATRIX_SIZE];
extern int8_t roberts_gy_2x2[MATRIX_SIZE];

// Laplace 5x5
extern int8_t laplace_5x5[MATRIX_SIZE];

#endif