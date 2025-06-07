#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image/stb_image.h"
#include "stb_image/stb_image_write.h"
#include <stdint.h>   // Para tipos inteiros de tamanho fixo (uint8_t, int16_t, etc.).
#include <stdio.h>    // Para funções de entrada/saída padrão (printf, scanf, fopen, etc.).
#include <stdlib.h>   // Para funções utilitárias gerais (malloc, free, exit, atoi, etc.).
#include <math.h>     // Para funções matemáticas (sqrt).
#include <string.h>   // Para funções de manipulação de strings (strcpy, strcmp, memset, etc.).
#include <dirent.h>   // Para operações de diretório (opendir, readdir, closedir).
#include <sys/stat.h> // Para obter informações sobre arquivos e criar diretórios (mkdir).
#include <errno.h>    // Para lidar com códigos de erro do sistema (errno).
#include "hps_0.h"

// --- Constantes Globais ---

// Define o tamanho linear da matriz/janela usada nas operações (5x5 = 25).
#define TAMANHO_MATRIZ_LINEAR 25
// Define a largura padrão das imagens processadas (em pixels).
#define LARGURA_PADRAO_IMG 320
// Define a altura padrão das imagens processadas (em pixels).
#define ALTURA_PADRAO_IMG 240

// --- Typedefs Globais ---

// Define um tipo `tipo_pixel_imagem` como `uint8_t` (inteiro sem sinal de 8 bits) para representar os valores dos pixels da imagem de entrada (0-255).
// Usado para a janela de pixels extraída da imagem.
// Aumenta a clareza e facilita a modificação futura do tipo de pixel.
typedef uint8_t tipo_pixel_imagem;
// Define um tipo `tipo_resultado_conv` como `int16_t` (inteiro com sinal de 16 bits) para representar os resultados das operações de convolução.
// O resultado pode ser negativo ou exceder 255, necessitando de maior alcance e sinal.
typedef int16_t tipo_resultado_conv;

// --- Variáveis Globais ---

// Matriz 2D global para armazenar a versão em escala de cinza da imagem carregada.
// Dimensões definidas por ALTURA_PADRAO_IMG e LARGURA_PADRAO_IMG.
unsigned char imagem_global_cinza[ALTURA_PADRAO_IMG][LARGURA_PADRAO_IMG];
// Vetor global para armazenar a janela de pixels (5x5) extraída da imagem em escala de cinza.
// Usado como entrada para as operações de convolução na FPGA.
// O tipo `tipo_pixel_imagem` (uint8_t) é usado para os elementos.
tipo_pixel_imagem janela_global_pixels[TAMANHO_MATRIZ_LINEAR];

/* ========== DEFINIÇÕES DOS KERNELS DOS FILTROS ========== */
// Kernels pré-definidos para diferentes filtros de detecção de borda.
// Todos são representados como matrizes lineares de `TAMANHO_MATRIZ_LINEAR` (25) elementos `int8_t`,
// mesmo que o kernel original seja menor (e.g., 3x3 ou 2x2). Os elementos não utilizados
// são preenchidos com zero para compatibilidade com a interface da FPGA que espera uma matriz 5x5.
// **NOTA:** Os nomes dos kernels foram mantidos para possível compatibilidade externa ou convenção.

// --- Kernel Sobel 3x3 --- 
int8_t sobel_gx_3x3[TAMANHO_MATRIZ_LINEAR] = {
     0,  0,  0,  0,  0,   // linha 0 (padding)
     0, -1,  0,  1,  0,   // linha 1 (kernel original)
     0, -2,  0,  2,  0,   // linha 2 (kernel original)
     0, -1,  0,  1,  0,   // linha 3 (kernel original)
     0,  0,  0,  0,  0    // linha 4 (padding)
};
int8_t sobel_gy_3x3[TAMANHO_MATRIZ_LINEAR] = {
     0,  0,  0,  0,  0,   // linha 0 (padding)
     0, -1, -2, -1,  0,   // linha 1 (kernel original)
     0,  0,  0,  0,  0,   // linha 2 (kernel original)
     0,  1,  2,  1,  0,   // linha 3 (kernel original)
     0,  0,  0,  0,  0    // linha 4 (padding)
};

// --- Kernel Sobel 5x5 --- 
int8_t sobel_gx_5x5[TAMANHO_MATRIZ_LINEAR] = {
     2,  2,  4,  2,  2,   // linha 0
     1,  1,  2,  1,  1,   // linha 1
     0,  0,  0,  0,  0,   // linha 2
    -1, -1, -2, -1, -1,   // linha 3
    -2, -2, -4, -2, -2    // linha 4
};
int8_t sobel_gy_5x5[TAMANHO_MATRIZ_LINEAR] = {
     2,  1,  0, -1, -2,   // linha 0
     2,  1,  0, -1, -2,   // linha 1
     4,  2,  0, -2, -4,   // linha 2
     2,  1,  0, -1, -2,   // linha 3
     2,  1,  0, -1, -2,   // linha 4
};

// --- Kernel Prewitt 3x3 --- 
int8_t prewitt_gx_3x3[TAMANHO_MATRIZ_LINEAR] = {
     0,  0,  0,  0,  0,   // linha 0 (padding)
     0, -1,  0,  1,  0,   // linha 1 (kernel original)
     0, -1,  0,  1,  0,   // linha 2 (kernel original)
     0, -1,  0,  1,  0,   // linha 3 (kernel original)
     0,  0,  0,  0,  0    // linha 4 (padding)
};
int8_t prewitt_gy_3x3[TAMANHO_MATRIZ_LINEAR] = {
     0,  0,  0,  0,  0,   // linha 0 (padding)
     0, -1, -1, -1,  0,   // linha 1 (kernel original)
     0,  0,  0,  0,  0,   // linha 2 (kernel original)
     0,  1,  1,  1,  0,   // linha 3 (kernel original)
     0,  0,  0,  0,  0    // linha 4 (padding)
};

// --- Kernel Roberts 2x2 --- 
int8_t roberts_gx_2x2[TAMANHO_MATRIZ_LINEAR] = {
     1,  0,  0,  0,  0,   // linha 0 (kernel original)
     0, -1,  0,  0,  0,   // linha 1 (kernel original)
     0,  0,  0,  0,  0,   // linha 2 (padding)
     0,  0,  0,  0,  0,   // linha 3 (padding)
     0,  0,  0,  0,  0    // linha 4 (padding)
};
int8_t roberts_gy_2x2[TAMANHO_MATRIZ_LINEAR] = {
     0,  1,  0,  0,  0,   // linha 0 (kernel original)
    -1,  0,  0,  0,  0,   // linha 1 (kernel original)
     0,  0,  0,  0,  0,   // linha 2 (padding)
     0,  0,  0,  0,  0,   // linha 3 (padding)
     0,  0,  0,  0,  0    // linha 4 (padding)
};

// --- Kernel Laplaciano 5x5 --- 
int8_t laplace_5x5[TAMANHO_MATRIZ_LINEAR] = {
     0,  0, -1,  0,  0,   // linha 0
     0, -1, -2, -1,  0,   // linha 1
    -1, -2, 16, -2, -1,   // linha 2
     0, -1, -2, -1,  0,   // linha 3
     0,  0, -1,  0,  0    // linha 4
};

/**
 * @brief Carrega uma imagem de um arquivo e a redimensiona para as dimensões LARGURA_PADRAO_IMG x ALTURA_PADRAO_IMG.
 * 
 * Utiliza a biblioteca stb_image para carregar a imagem. Se a imagem já tiver as dimensões
 * corretas, ela é copiada diretamente. Caso contrário, um redimensionamento simples por 
 * vizinho mais próximo (nearest neighbor) é aplicado.
 * 
 * @param nome_arquivo O caminho para o arquivo de imagem a ser carregado.
 * @param buffer_destino_rgb Matriz 3D (ALTURA_PADRAO_IMG x LARGURA_PADRAO_IMG x 3) onde a imagem RGB redimensionada será armazenada.
 * @return 0 em caso de sucesso, -1 se ocorrer erro ao carregar a imagem.
 */
int carregar_e_redimensionar_imagem(const char* nome_arquivo, unsigned char buffer_destino_rgb[ALTURA_PADRAO_IMG][LARGURA_PADRAO_IMG][3]) {
    int largura_original, altura_original, canais_originais; // Variáveis para armazenar dimensões e canais da imagem original.
    int coord_y, coord_x; // Variáveis de iteração para loops.
    
    // Tenta carregar a imagem usando stbi_load.
    // Força a carga de 3 canais (RGB), descartando o alfa se existir.
    unsigned char* dados_imagem_bruta = stbi_load(nome_arquivo, &largura_original, &altura_original, &canais_originais, 3);
    
    // Verifica se o carregamento falhou.
    if (!dados_imagem_bruta) {
        printf("Erro ao carregar a imagem: %s\n", nome_arquivo);
        return -1; // Retorna erro.
    }
    
    printf("Imagem carregada: %s (%dx%d pixels, %d canais)\n", nome_arquivo, largura_original, altura_original, canais_originais);
    
    // Verifica se a imagem já possui as dimensões desejadas.
    if (largura_original == LARGURA_PADRAO_IMG && altura_original == ALTURA_PADRAO_IMG) {
        printf("Dimensões da imagem correspondem ao alvo. Copiando diretamente.\n");
        // Copia os dados pixel a pixel para a matriz de saída `buffer_destino_rgb`.
        for (coord_y = 0; coord_y < ALTURA_PADRAO_IMG; coord_y++) {
            for (coord_x = 0; coord_x < LARGURA_PADRAO_IMG; coord_x++) {
                int indice_pixel_origem = (coord_y * LARGURA_PADRAO_IMG + coord_x) * 3; // Calcula o índice linear no buffer de entrada.
                buffer_destino_rgb[coord_y][coord_x][0] = dados_imagem_bruta[indice_pixel_origem + 0]; // Copia componente R.
                buffer_destino_rgb[coord_y][coord_x][1] = dados_imagem_bruta[indice_pixel_origem + 1]; // Copia componente G.
                buffer_destino_rgb[coord_y][coord_x][2] = dados_imagem_bruta[indice_pixel_origem + 2]; // Copia componente B.
            }
        }
    } else {
        // A imagem precisa ser redimensionada.
        printf("Redimensionando de %dx%d para %dx%d usando vizinho mais próximo...\n", largura_original, altura_original, LARGURA_PADRAO_IMG, ALTURA_PADRAO_IMG);
        
        // Itera sobre cada pixel da imagem de destino (LARGURA_PADRAO_IMG x ALTURA_PADRAO_IMG).
        for (coord_y = 0; coord_y < ALTURA_PADRAO_IMG; coord_y++) {
            for (coord_x = 0; coord_x < LARGURA_PADRAO_IMG; coord_x++) {
                // Calcula as coordenadas correspondentes na imagem original usando interpolação por vizinho mais próximo.
                int coord_x_origem = (coord_x * largura_original) / LARGURA_PADRAO_IMG;
                int coord_y_origem = (coord_y * altura_original) / ALTURA_PADRAO_IMG;
                
                // Garante que as coordenadas calculadas não excedam os limites da imagem original.
                if (coord_x_origem >= largura_original) coord_x_origem = largura_original - 1;
                if (coord_y_origem >= altura_original) coord_y_origem = altura_original - 1;
                
                // Calcula o índice linear do pixel correspondente na imagem original.
                int indice_pixel_origem = (coord_y_origem * largura_original + coord_x_origem) * 3;
                // Copia os componentes RGB do pixel original para o pixel de destino.
                buffer_destino_rgb[coord_y][coord_x][0] = dados_imagem_bruta[indice_pixel_origem + 0];
                buffer_destino_rgb[coord_y][coord_x][1] = dados_imagem_bruta[indice_pixel_origem + 1];
                buffer_destino_rgb[coord_y][coord_x][2] = dados_imagem_bruta[indice_pixel_origem + 2];
            }
        }
    }
    
    // Libera a memória alocada por stbi_load para os dados da imagem original.
    stbi_image_free(dados_imagem_bruta);
    return 0; // Retorna sucesso.
}

/**
 * @brief Salva uma imagem em escala de cinza em um arquivo PNG.
 * 
 * Utiliza a biblioteca stb_image_write. Tenta criar o diretório de saída
 * se ele não existir.
 * 
 * @param nome_arquivo_saida O caminho completo (incluindo nome do arquivo) onde salvar a imagem PNG.
 * @param dados_imagem_cinza Matriz 2D (ALTURA_PADRAO_IMG x LARGURA_PADRAO_IMG) contendo os dados da imagem em escala de cinza.
 */
void salvar_imagem_cinza_png(const char* nome_arquivo_saida, unsigned char dados_imagem_cinza[ALTURA_PADRAO_IMG][LARGURA_PADRAO_IMG]) {
    // Extrai o caminho do diretório a partir do nome completo do arquivo.
    char caminho_diretorio[256];
    strncpy(caminho_diretorio, nome_arquivo_saida, sizeof(caminho_diretorio) - 1); // Copia o nome do arquivo para um buffer temporário.
    caminho_diretorio[sizeof(caminho_diretorio) - 1] = '\0'; // Garante terminação nula.
    
    // Encontra a última barra ('/') no caminho.
    char *ultima_barra = strrchr(caminho_diretorio, '/');
    if (ultima_barra != NULL) {
        // Se encontrou uma barra, termina a string ali para obter apenas o caminho do diretório.
        *ultima_barra = '\0'; 
        // Tenta criar o diretório. A flag 0777 define as permissões.
        // Ignora o erro se o diretório já existir (errno == EEXIST).
        if (mkdir(caminho_diretorio, 0777) == -1 && errno != EEXIST) {
            perror("Erro ao criar diretório de saída");
            // Não retorna erro aqui, tenta salvar mesmo assim, pode funcionar se o diretório base existir.
        }
    }

    // Tenta salvar a imagem em escala de cinza como PNG usando stbi_write_png.
    // Parâmetros: nome do arquivo, largura, altura, número de canais (1 para grayscale),
    // ponteiro para os dados, e stride (número de bytes por linha, que é a largura).
    if (!stbi_write_png(nome_arquivo_saida, LARGURA_PADRAO_IMG, ALTURA_PADRAO_IMG, 1, dados_imagem_cinza, LARGURA_PADRAO_IMG)) {
        printf("Erro ao salvar PNG: %s\n", nome_arquivo_saida);
    } else {
        printf("PNG salvo com sucesso: %s\n", nome_arquivo_saida);
    }
}

/**
 * @brief Converte uma imagem RGB para escala de cinza.
 * 
 * Utiliza a fórmula de luminância padrão (0.299*R + 0.587*G + 0.114*B).
 * 
 * @param buffer_entrada_rgb Matriz 3D (ALTURA_PADRAO_IMG x LARGURA_PADRAO_IMG x 3) contendo a imagem RGB de entrada.
 * @param buffer_saida_cinza Matriz 2D (ALTURA_PADRAO_IMG x LARGURA_PADRAO_IMG) onde a imagem em escala de cinza resultante será armazenada.
 */
void converter_rgb_para_cinza(unsigned char buffer_entrada_rgb[ALTURA_PADRAO_IMG][LARGURA_PADRAO_IMG][3], unsigned char buffer_saida_cinza[ALTURA_PADRAO_IMG][LARGURA_PADRAO_IMG]) {
    int coord_y, coord_x; // Variáveis de iteração.
    // Itera sobre cada pixel da imagem.
    for (coord_y = 0; coord_y < ALTURA_PADRAO_IMG; coord_y++) {
        for (coord_x = 0; coord_x < LARGURA_PADRAO_IMG; coord_x++) {
            // Obtém os componentes R, G, B do pixel atual.
            unsigned char comp_r = buffer_entrada_rgb[coord_y][coord_x][0];
            unsigned char comp_g = buffer_entrada_rgb[coord_y][coord_x][1];
            unsigned char comp_b = buffer_entrada_rgb[coord_y][coord_x][2];
            // Calcula o valor em escala de cinza usando a fórmula de luminância.
            // O resultado é convertido para `unsigned char` (0-255).
            buffer_saida_cinza[coord_y][coord_x] = (unsigned char)(0.299*comp_r + 0.587*comp_g + 0.114*comp_b);
        }
    }
}

/**
 * @brief Extrai uma janela de pixels (vizinhaça) de uma imagem em escala de cinza.
 * 
 * A janela extraída é sempre armazenada no buffer linear global `janela_global_pixels` de tamanho 5x5 (TAMANHO_MATRIZ_LINEAR).
 * O tamanho real da janela a ser extraída (2x2, 3x3 ou 5x5) é determinado pelo `codigo_tamanho_kernel`.
 * A janela é posicionada corretamente dentro do buffer 5x5, com padding de zeros se necessário.
 * Trata o padding nas bordas da imagem atribuindo 0 aos pixels fora dos limites.
 * 
 * @param imagem_cinza Matriz 2D (ALTURA_PADRAO_IMG x LARGURA_PADRAO_IMG) da imagem em escala de cinza.
 * @param centro_x Coordenada X do pixel central da janela na imagem.
 * @param centro_y Coordenada Y do pixel central da janela na imagem.
 * @param codigo_tamanho_kernel Código que indica o tamanho da janela a ser extraída:
 *                  0: Roberts 2x2 (mapeado para canto superior esquerdo do 5x5)
 *                  1: Sobel/Prewitt 3x3 (mapeado para centro do 5x5)
 *                  3: Sobel/Laplace 5x5 (usa todo o 5x5)
 */
void extrair_janela_vizinhanca_linear(unsigned char imagem_cinza[ALTURA_PADRAO_IMG][LARGURA_PADRAO_IMG], int centro_x, int centro_y, uint32_t codigo_tamanho_kernel) {
    int desloc_y, desloc_x, indice_janela; // Variáveis de iteração e índice.
    int pixel_x_img, pixel_y_img; // Coordenadas do pixel na imagem original.
    
    // Zera completamente o buffer global `janela_global_pixels` (5x5) antes de preenchê-lo.
    // Isso garante que áreas não preenchidas (padding) contenham zero.
    memset(janela_global_pixels, 0, TAMANHO_MATRIZ_LINEAR * sizeof(tipo_pixel_imagem));
    
    // --- Caso Especial: Roberts 2x2 (codigo_tamanho_kernel == 0) ---
    if (codigo_tamanho_kernel == 0) {
        // Itera sobre a janela 2x2.
        for (desloc_y = 0; desloc_y < 2; desloc_y++) { // Linhas da janela 2x2 (0 a 1)
            for (desloc_x = 0; desloc_x < 2; desloc_x++) { // Colunas da janela 2x2 (0 a 1)
                // Calcula as coordenadas do pixel correspondente na imagem original.
                // O canto superior esquerdo da janela 2x2 corresponde ao pixel (centro_x, centro_y) da imagem.
                pixel_x_img = centro_x + desloc_x;
                pixel_y_img = centro_y + desloc_y;
                
                // Mapeia a posição (desloc_y, desloc_x) da janela 2x2 para o índice linear `indice_janela`
                // dentro do buffer global 5x5 (`janela_global_pixels`).
                // A janela 2x2 é colocada no canto superior esquerdo do buffer 5x5.
                int linha_no_buffer_5x5 = desloc_y; // Linha 0 ou 1 no buffer 5x5.
                int coluna_no_buffer_5x5 = desloc_x; // Coluna 0 ou 1 no buffer 5x5.
                indice_janela = linha_no_buffer_5x5 * 5 + coluna_no_buffer_5x5; // Índice linear (0, 1, 5, 6).
                
                // Verifica se as coordenadas (pixel_x_img, pixel_y_img) estão dentro dos limites da imagem.
                if (pixel_x_img >= 0 && pixel_x_img < LARGURA_PADRAO_IMG && pixel_y_img >= 0 && pixel_y_img < ALTURA_PADRAO_IMG) {
                    // Se dentro dos limites, copia o valor do pixel da imagem para a janela.
                    janela_global_pixels[indice_janela] = (tipo_pixel_imagem)imagem_cinza[pixel_y_img][pixel_x_img];
                } else {
                    // Se fora dos limites (borda da imagem), aplica padding com zero.
                    // (Já foi feito pelo memset, mas explícito aqui por clareza).
                    janela_global_pixels[indice_janela] = 0;
                }
            }
        }
    } 
    // --- Caso Geral: Janelas 3x3 e 5x5 (codigo_tamanho_kernel == 1 ou 3) ---
    else {
        int tamanho_real_janela; // Tamanho da janela (3 ou 5).
        // Determina o tamanho da janela com base no codigo_tamanho_kernel.
        switch(codigo_tamanho_kernel) {
            case 1: tamanho_real_janela = 3; break; // Sobel/Prewitt 3x3
            case 3: tamanho_real_janela = 5; break; // Sobel 5x5/Laplace 5x5
            default: tamanho_real_janela = 3; break; // Comportamento padrão (assume 3x3)
        }
        
        // Calcula a metade do tamanho da janela para facilitar a iteração em torno do centro.
        int metade_tamanho_janela = tamanho_real_janela / 2; // 1 para 3x3, 2 para 5x5.
        
        // Itera sobre a vizinhança definida pela janela (de -metade_tamanho_janela a +metade_tamanho_janela em relação ao centro).
        for (desloc_y = -metade_tamanho_janela; desloc_y <= metade_tamanho_janela; desloc_y++) { // Deslocamento vertical relativo a centro_y.
            for (desloc_x = -metade_tamanho_janela; desloc_x <= metade_tamanho_janela; desloc_x++) { // Deslocamento horizontal relativo a centro_x.
                // Calcula as coordenadas absolutas (pixel_x_img, pixel_y_img) do pixel na imagem original.
                pixel_x_img = centro_x + desloc_x;
                pixel_y_img = centro_y + desloc_y;
                
                // Calcula a posição (linha, coluna) correspondente dentro do buffer 5x5 (`janela_global_pixels`).
                // O centro da janela (desloc_y=0, desloc_x=0) corresponde ao centro do buffer 5x5 (linha 2, coluna 2).
                int linha_no_buffer_5x5 = desloc_y + 2; // Mapeia -1..1 (3x3) ou -2..2 (5x5) para 1..3 ou 0..4.
                int coluna_no_buffer_5x5 = desloc_x + 2; // Mapeia -1..1 (3x3) ou -2..2 (5x5) para 1..3 ou 0..4.
                indice_janela = linha_no_buffer_5x5 * 5 + coluna_no_buffer_5x5; // Calcula o índice linear no buffer 5x5.
                
                // Verifica se as coordenadas (pixel_x_img, pixel_y_img) estão dentro dos limites da imagem.
                if (pixel_x_img >= 0 && pixel_x_img < LARGURA_PADRAO_IMG && pixel_y_img >= 0 && pixel_y_img < ALTURA_PADRAO_IMG) {
                    // Se dentro dos limites, copia o valor do pixel da imagem para a janela.
                    janela_global_pixels[indice_janela] = (tipo_pixel_imagem)imagem_cinza[pixel_y_img][pixel_x_img];
                } else {
                    // Se fora dos limites (borda da imagem), aplica padding com zero.
                    // (Já foi feito pelo memset).
                    janela_global_pixels[indice_janela] = 0;
                }
            }
        }
    }
}

/**
 * @brief Calcula a convolução entre uma janela de imagem e um kernel de filtro usando a FPGA.
 * 
 * Prepara os parâmetros (ponteiros para a janela e kernel, opcode, tamanho) e os envia
 * para a FPGA através da função `transfer_data_to_fpga` (definida em hps_0.h/lib.s).
 * Em seguida, recupera o resultado da FPGA usando `retrieve_fpga_results` (definida em hps_0.h/lib.s).
 * O resultado da FPGA é esperado como um vetor de bytes, onde os dois primeiros bytes
 * representam o resultado final de 16 bits (signed).
 * 
 * @param ponteiro_janela_pixels Ponteiro para o buffer linear (TAMANHO_MATRIZ_LINEAR) contendo a janela de pixels (tipo_pixel_imagem/uint8_t).
 * @param ponteiro_kernel_filtro Ponteiro para o buffer linear (TAMANHO_MATRIZ_LINEAR) contendo o kernel do filtro (int8_t).
 * @param codigo_tamanho_kernel Código de tamanho do kernel (embora pareça não ser usado diretamente na chamada FPGA aqui, 
 *                  a FPGA pode usá-lo internamente se `params.size` for passado corretamente).
 *                  NOTA: O código atual passa `params.size = 3` fixo. Isso pode ser um bug ou simplificação.
 *                  Se a FPGA espera o `codigo_tamanho_kernel` correto (0, 1, 3), deveria ser `params.size = codigo_tamanho_kernel;`.
 * @return O resultado da convolução como um valor `tipo_resultado_conv` (int16_t). Retorna 0 em caso de falha na comunicação com a FPGA.
 */
int calcular_convolucao_fpga(tipo_pixel_imagem* ponteiro_janela_pixels, int8_t* ponteiro_kernel_filtro, uint32_t codigo_tamanho_kernel) {
    // Buffer temporário para receber o resultado bruto da FPGA (esperado como bytes).
    // Usa tipo_pixel_imagem (uint8_t) para compatibilidade com a assinatura de retrieve_fpga_results.
    tipo_pixel_imagem buffer_resultado_fpga[TAMANHO_MATRIZ_LINEAR]; 
   
    // Prepara a estrutura de parâmetros para enviar à FPGA (struct Params definida em hps_0.h).
    struct Params parametros_fpga = {
        .a = ponteiro_janela_pixels,      // Ponteiro para a janela de pixels (matriz A na FPGA).
        .b = ponteiro_kernel_filtro,     // Ponteiro para o kernel do filtro (matriz B na FPGA).
        .opcode = 7,            // Código de operação para convolução (assumindo 7 = convolução na FPGA).
        .size = 3               // Código de tamanho. **ATENÇÃO:** Está fixo em 3 (interpretado como 5x5 pela FPGA?).
                                // Se a FPGA espera o código real (0, 1, 3), isto deveria ser: `.size = codigo_tamanho_kernel`.
                                // Mantido como 3 para preservar comportamento original, mas verificar especificação da FPGA.
    };
    
    // Envia os dados (ponteiros e parâmetros) para a FPGA usando a função externa.
    if (transfer_data_to_fpga(&parametros_fpga) != HW_SUCCESS) {
        fprintf(stderr, "Falha no envio de dados para a FPGA\n");
        return 0; // Retorna 0 em caso de erro.
    }
    
    // Recupera os resultados do processamento da FPGA usando a função externa.
    if (retrieve_fpga_results(buffer_resultado_fpga) != HW_SUCCESS) {
        fprintf(stderr, "Falha na leitura dos resultados da FPGA\n");
        return 0; // Retorna 0 em caso de erro.
    }
    
    // Reconstrói o resultado final de 16 bits (tipo_resultado_conv) a partir dos dois primeiros bytes recebidos.
    // Assume que buffer_resultado_fpga[1] é o byte mais significativo (MSB) e buffer_resultado_fpga[0] é o menos significativo (LSB).
    // O casting para (int16_t) garante a interpretação correta do sinal.
    tipo_resultado_conv resultado_final_conv = (tipo_resultado_conv)((buffer_resultado_fpga[1] << 8) | buffer_resultado_fpga[0]);
    
    return resultado_final_conv; // Retorna o resultado da convolução.
}

/**
 * @brief Satura um valor de resultado de convolução para a faixa válida de pixel (0 a 255).
 * 
 * Se o valor for menor que 0, retorna 0.
 * Se o valor for maior que 255, retorna 255.
 * Caso contrário, retorna o próprio valor convertido para `unsigned char`.
 * 
 * @param valor_entrada O valor `tipo_resultado_conv` (int16_t) a ser saturado.
 * @return O valor saturado como `unsigned char`.
 */
unsigned char saturar_valor_pixel(tipo_resultado_conv valor_entrada) {
    if (valor_entrada < 0) return 0;
    if (valor_entrada > 255) return 255;
    return (unsigned char)valor_entrada;
}

/**
 * @brief Aplica um filtro de detecção de borda (como Sobel, Prewitt, Roberts ou Laplace) a uma imagem em escala de cinza.
 * 
 * A função opera em fases:
 * 1. Calcula o gradiente na direção X (Gx) para toda a imagem, armazenando em `buffer_gradiente_x`.
 * 2. Se um filtro Gy for fornecido (ponteiro_kernel_gy != NULL), calcula o gradiente na direção Y (Gy), armazenando em `buffer_gradiente_y`.
 * 3. Se ambos Gx e Gy foram calculados, calcula a magnitude do gradiente (sqrt(Gx^2 + Gy^2)) para cada pixel.
 * 4. Se apenas Gx foi calculado (caso do Laplace, onde ponteiro_kernel_gy é NULL), usa o valor absoluto de Gx.
 * 5. Satura o resultado (magnitude ou |Gx|) para a faixa 0-255 e armazena na matriz `buffer_resultado_final`.
 * 
 * Utiliza buffers estáticos (`buffer_gradiente_x`, `buffer_gradiente_y`) para armazenar os resultados intermediários dos gradientes.
 * Chama `extrair_janela_vizinhanca_linear` para obter a vizinhança de cada pixel e `calcular_convolucao_fpga` para calcular a convolução na FPGA.
 * 
 * @param ponteiro_kernel_gx Ponteiro para o kernel do filtro Gx (ou o único kernel, no caso do Laplace).
 * @param ponteiro_kernel_gy Ponteiro para o kernel do filtro Gy. NULL se o filtro for unidirecional (Laplace).
 * @param codigo_tamanho_kernel Código que indica o tamanho do kernel (0, 1 ou 3), passado para `extrair_janela_vizinhanca_linear`.
 * @param buffer_resultado_final Matriz 2D (ALTURA_PADRAO_IMG x LARGURA_PADRAO_IMG) onde a imagem resultante do filtro de borda será armazenada.
 */
void aplicar_filtro_operacao(int8_t* ponteiro_kernel_gx, int8_t* ponteiro_kernel_gy, uint32_t codigo_tamanho_kernel, unsigned char buffer_resultado_final[ALTURA_PADRAO_IMG][LARGURA_PADRAO_IMG]) {
    // Buffers estáticos para armazenar os resultados intermediários dos gradientes Gx e Gy.
    // Usar `static` evita alocação na pilha, que poderia estourar para arrays grandes.
    // O tipo `tipo_resultado_conv` (int16_t) é usado para armazenar os resultados da convolução.
    static tipo_resultado_conv buffer_gradiente_x[ALTURA_PADRAO_IMG][LARGURA_PADRAO_IMG];
    static tipo_resultado_conv buffer_gradiente_y[ALTURA_PADRAO_IMG][LARGURA_PADRAO_IMG];
    
    int coord_x, coord_y; // Variáveis de iteração.
    
    printf("Processando imagem com filtro de borda...\n");
    
    // Inicializa os buffers intermediários com zero.
    memset(buffer_gradiente_x, 0, sizeof(buffer_gradiente_x));
    memset(buffer_gradiente_y, 0, sizeof(buffer_gradiente_y));
    
    // --- Fase 1: Calcular Gradiente Gx --- 
    // Itera sobre cada pixel da imagem.
    for (coord_y = 0; coord_y < ALTURA_PADRAO_IMG; coord_y++) {
        for (coord_x = 0; coord_x < LARGURA_PADRAO_IMG; coord_x++) {
            // Extrai a janela de pixels centrada em (coord_x, coord_y) da imagem `imagem_global_cinza` global.
            // O tamanho da janela é determinado por `codigo_tamanho_kernel`.
            extrair_janela_vizinhanca_linear(imagem_global_cinza, coord_x, coord_y, codigo_tamanho_kernel);
            // Calcula a convolução entre a janela (`janela_global_pixels` global) e o kernel `ponteiro_kernel_gx` usando a FPGA.
            // O resultado (int16_t) é armazenado no buffer Gx.
            buffer_gradiente_x[coord_y][coord_x] = calcular_convolucao_fpga(janela_global_pixels, ponteiro_kernel_gx, codigo_tamanho_kernel);
        }
    }
    
    // --- Fase 2: Calcular Gradiente Gy (se aplicável) --- 
    // Verifica se um kernel Gy foi fornecido.
    if (ponteiro_kernel_gy != NULL) {
        // Itera sobre cada pixel da imagem.
        for (coord_y = 0; coord_y < ALTURA_PADRAO_IMG; coord_y++) {
            for (coord_x = 0; coord_x < LARGURA_PADRAO_IMG; coord_x++) {
                // Extrai a janela novamente (poderia ser otimizado se a janela não mudasse entre Gx e Gy).
                extrair_janela_vizinhanca_linear(imagem_global_cinza, coord_x, coord_y, codigo_tamanho_kernel);
                // Calcula a convolução com o kernel `ponteiro_kernel_gy`.
                buffer_gradiente_y[coord_y][coord_x] = calcular_convolucao_fpga(janela_global_pixels, ponteiro_kernel_gy, codigo_tamanho_kernel);
            }
        }
        
        // --- Fase 3: Calcular Magnitude do Gradiente --- 
        // Itera sobre cada pixel.
        for (coord_y = 0; coord_y < ALTURA_PADRAO_IMG; coord_y++) {
            for (coord_x = 0; coord_x < LARGURA_PADRAO_IMG; coord_x++) {
                // Obtém os resultados Gx e Gy dos buffers.
                tipo_resultado_conv valor_gx = buffer_gradiente_x[coord_y][coord_x];
                tipo_resultado_conv valor_gy = buffer_gradiente_y[coord_y][coord_x];
                
                // Calcula a magnitude Euclidiana: sqrt(Gx*Gx + Gy*Gy).
                // Usa `double` para a conta intermediária para evitar overflow e perda de precisão.
                tipo_resultado_conv magnitude_gradiente = (tipo_resultado_conv)sqrt((double)(valor_gx * valor_gx + valor_gy * valor_gy));
                
                // Satura o valor da magnitude para a faixa 0-255 e armazena no buffer final.
                buffer_resultado_final[coord_y][coord_x] = saturar_valor_pixel(magnitude_gradiente);
            }
        }
        
    } 
    // --- Caso: Filtro Unidirecional (Laplace) --- 
    else {
        printf("Processando filtro unidirecional (Laplace)... usando |Gx|\n");
        // Itera sobre cada pixel.
        for (coord_y = 0; coord_y < ALTURA_PADRAO_IMG; coord_y++) {
            for (coord_x = 0; coord_x < LARGURA_PADRAO_IMG; coord_x++) {
                // Para filtros como Laplace, o resultado Gx já representa a resposta do filtro.
                // Calcula o valor absoluto do resultado da convolução Gx.
                tipo_resultado_conv valor_absoluto_gx = abs(buffer_gradiente_x[coord_y][coord_x]);
                // Satura o valor absoluto para a faixa 0-255 e armazena no buffer final.
                buffer_resultado_final[coord_y][coord_x] = saturar_valor_pixel(valor_absoluto_gx);
            }
        }
    }
    
    printf("Aplicação do filtro concluída.\n");
}

/**
 * @brief Valida a seleção de operação (filtro) feita pelo usuário.
 * 
 * Verifica se a seleção está dentro do intervalo válido (1 a 6).
 * 
 * @param opcao_escolhida A opção (número do filtro ou sair) selecionada pelo usuário.
 * @return 0 se a seleção for válida (1 a 6), -1 caso contrário.
 */
int validar_opcao_usuario(uint32_t opcao_escolhida) {
    // Verifica se a opção está fora do intervalo permitido (1 para Sobel 3x3 até 6 para Sair).
    if (opcao_escolhida < 1 || opcao_escolhida > 6) {
        fprintf(stderr, "Opção inválida: %u. Escolha um número entre 1 e 6.\n", opcao_escolhida);
        return -1; // Indica seleção inválida.
    }
    return 0; // Indica seleção válida.
}

/**
 * @brief Verifica se um nome de arquivo corresponde a um formato de imagem suportado.
 * 
 * Compara a extensão do arquivo (ignorando maiúsculas/minúsculas) com as extensões
 * suportadas (".png", ".jpg", ".jpeg", ".bmp").
 * 
 * @param nome_arquivo O nome do arquivo a ser verificado.
 * @return 1 se for um arquivo de imagem suportado, 0 caso contrário.
 */
int verificar_arquivo_imagem_valido(const char *nome_arquivo) {
    // Encontra a última ocorrência do caractere '.' no nome do arquivo.
    const char *posicao_ponto = strrchr(nome_arquivo, '.');
    // Se não houver ponto, ou se o ponto for o primeiro caractere, não é uma extensão válida.
    if (!posicao_ponto || posicao_ponto == nome_arquivo) return 0;
    // Obtém o ponteiro para a string da extensão (após o ponto).
    const char *extensao = posicao_ponto + 1;
    // Compara a extensão (ignorando caso) com as extensões suportadas.
    // `strcasecmp` retorna 0 se as strings forem iguais.
    return (strcasecmp(extensao, "png") == 0 || 
            strcasecmp(extensao, "jpg") == 0 || 
            strcasecmp(extensao, "jpeg") == 0 || 
            strcasecmp(extensao, "bmp") == 0);
}

/* ====================================================== */
/* ================= FUNÇÃO PRINCIPAL =================== */
/* ====================================================== */

int main() {
    // --- Variáveis Locais --- 
    char caminho_arquivo_entrada[256]; // Buffer para construir o caminho completo do arquivo de entrada.
    char caminho_arquivo_saida[256];   // Buffer para construir o caminho completo do arquivo de saída.
    char nome_base_arquivo_saida[100]; // Buffer para armazenar a parte base do nome do arquivo de saída (sem extensão).
    unsigned char buffer_imagem_rgb[ALTURA_PADRAO_IMG][LARGURA_PADRAO_IMG][3]; // Buffer para armazenar a imagem RGB carregada e redimensionada.
    // Buffer para armazenar o resultado final do filtro de borda (imagem em escala de cinza).
    // `static` para evitar estouro de pilha.
    static unsigned char buffer_resultado_filtro[ALTURA_PADRAO_IMG][LARGURA_PADRAO_IMG]; 
    uint32_t opcao_usuario;            // Armazena a opção de filtro selecionada pelo usuário.
    // int coord_y_main, coord_x_main; // Variáveis de iteração (não usadas diretamente no main, removidas para limpeza).
    DIR *ponteiro_diretorio;           // Ponteiro para a estrutura de diretório.
    struct dirent *entrada_diretorio;  // Ponteiro para a entrada de diretório (arquivo ou subdiretório).
    const char *nome_diretorio_entrada = "input";   // Nome do diretório de entrada padrão.
    const char *nome_diretorio_saida = "output"; // Nome do diretório de saída padrão.

    // --- Inicialização --- 

    // Tenta criar o diretório de saída. Ignora o erro se ele já existir.
    if (mkdir(nome_diretorio_saida, 0777) == -1 && errno != EEXIST) {
        perror("Erro ao criar diretório de saída");
        return EXIT_FAILURE; // Encerra se não puder criar o diretório.
    }
    
    // Inicializa a comunicação com o hardware/FPGA.
    // `initiate_hardware` é definida externamente (hps_0.h / lib.s).
    if (initiate_hardware() != HW_SUCCESS) { 
        fprintf(stderr, "Falha ao inicializar hardware (FPGA)\n");
        return EXIT_FAILURE; // Encerra se a inicialização falhar.
    }
    
    printf("\n========= PROCESSAMENTO DE IMAGENS COM FILTRO DE BORDA (FPGA + STB_IMAGE) =========\n");
    
    // Tenta abrir o diretório de entrada.
    ponteiro_diretorio = opendir(nome_diretorio_entrada);
    if (ponteiro_diretorio == NULL) {
        perror("Erro ao abrir diretório de entrada 'input'");
        fprintf(stderr, "Certifique-se de que o diretório 'input' existe no mesmo local do executável e contém as imagens.\n");
        terminate_hardware(); // Libera recursos de hardware antes de sair.
        return EXIT_FAILURE;
    }

    printf("Processando imagens encontradas no diretório '%s'...\n", nome_diretorio_entrada);

    // --- Loop Principal de Seleção de Filtro --- 
    // Permite ao usuário escolher um filtro e aplicá-lo a todas as imagens no diretório de entrada.
    while (1) {
        opcao_usuario = 0; // Reseta a seleção.
        
        // Exibe o menu de opções.
        printf("\n\n------------------------------------------\n");
        printf("Escolha o filtro de borda para aplicar a TODAS as imagens em '%s':\n", nome_diretorio_entrada);
        printf("  1 - Sobel (3x3)\n");
        printf("  2 - Sobel expandido (5x5)\n");
        printf("  3 - Prewitt (3x3)\n");
        printf("  4 - Roberts (2x2)\n");
        printf("  5 - Laplace (5x5)\n");
        printf("  6 - Sair\n");
        printf("------------------------------------------\n");
        printf("Opção: ");
        
        // Lê a seleção do usuário.
        if (scanf("%u", &opcao_usuario) != 1) {
            printf("Entrada inválida! Por favor, digite um número.\n");
            // Limpa o buffer de entrada para evitar loops infinitos em caso de entrada não numérica.
            while (getchar() != '\n'); 
            continue; // Volta ao início do loop while.
        }
                
        // Valida a seleção (1 a 5).
        if (validar_opcao_usuario(opcao_usuario) != 0) { 
            continue; // Se inválida, volta ao início do loop while.
        }

        // --- Configuração do Filtro Selecionado --- 
        const char *nome_filtro_selecionado; // Nome do filtro para usar no nome do arquivo de saída.
        int8_t *kernel_selecionado_gx = NULL; // Ponteiro para o kernel Gx selecionado.
        int8_t *kernel_selecionado_gy = NULL; // Ponteiro para o kernel Gy selecionado (NULL para Laplace).
        uint32_t codigo_tamanho_kernel_selecionado = 0; // Código de tamanho do kernel para passar às funções (0, 1 ou 3).

        // Define os ponteiros e o código de tamanho com base na seleção do usuário.
        switch (opcao_usuario) {
            case 1: 
                nome_filtro_selecionado = "sobel_3x3"; 
                kernel_selecionado_gx = sobel_gx_3x3; 
                kernel_selecionado_gy = sobel_gy_3x3; 
                codigo_tamanho_kernel_selecionado = 1; // Código para 3x3
                break;
            case 2: 
                nome_filtro_selecionado = "sobel_5x5"; 
                kernel_selecionado_gx = sobel_gx_5x5; 
                kernel_selecionado_gy = sobel_gy_5x5; 
                codigo_tamanho_kernel_selecionado = 3; // Código para 5x5
                break;
            case 3: 
                nome_filtro_selecionado = "prewitt_3x3"; 
                kernel_selecionado_gx = prewitt_gx_3x3; 
                kernel_selecionado_gy = prewitt_gy_3x3; 
                codigo_tamanho_kernel_selecionado = 1; // Código para 3x3
                break;
            case 4: 
                nome_filtro_selecionado = "roberts_2x2"; 
                kernel_selecionado_gx = roberts_gx_2x2; 
                kernel_selecionado_gy = roberts_gy_2x2; 
                codigo_tamanho_kernel_selecionado = 0; // Código para 2x2
                break;
            case 5: 
                nome_filtro_selecionado = "laplace_5x5"; 
                kernel_selecionado_gx = laplace_5x5; 
                kernel_selecionado_gy = NULL; // Laplace não tem Gy separado.
                codigo_tamanho_kernel_selecionado = 3; // Código para 5x5
                break;
             case 6:
                  printf("Encerrando o programa...\n");
                  break;
            // Não deve chegar aqui devido à validação anterior, mas é uma boa prática ter um default.
            default: 
                printf("Erro interno - seleção inválida (%u) após validação.\n", opcao_usuario);
                continue; 
        }

        printf("\nAplicando filtro '%s' a todas as imagens no diretório '%s'...\n", nome_filtro_selecionado, nome_diretorio_entrada);

        // --- Loop de Processamento de Arquivos --- 
        // Itera sobre todos os arquivos no diretório de entrada para aplicar o filtro selecionado.
        
        // Volta ao início do diretório para garantir que todos os arquivos sejam processados.
        rewinddir(ponteiro_diretorio);

        // Lê a próxima entrada no diretório.
        while ((entrada_diretorio = readdir(ponteiro_diretorio)) != NULL) {
            // Ignora as entradas especiais "." (diretório atual) e ".." (diretório pai).
            if (strcmp(entrada_diretorio->d_name, ".") == 0 || strcmp(entrada_diretorio->d_name, "..") == 0) {
                continue;
            }

            // Constrói o caminho completo para o arquivo de entrada.
            snprintf(caminho_arquivo_entrada, sizeof(caminho_arquivo_entrada), "%s/%s", nome_diretorio_entrada, entrada_diretorio->d_name);

            // Verifica se a entrada é um arquivo de imagem suportado.
            struct stat info_arquivo;
            stat(caminho_arquivo_entrada, &info_arquivo); // Obtém informações sobre o arquivo/diretório.
            // Verifica se é um arquivo regular (S_ISREG) e se tem uma extensão de imagem suportada.
            if (!S_ISREG(info_arquivo.st_mode) || !verificar_arquivo_imagem_valido(entrada_diretorio->d_name)) {
                // printf("Ignorando '%s' (não é um arquivo de imagem suportado).\n", entrada_diretorio->d_name); // Comentado para reduzir output
                continue; // Pula para a próxima entrada do diretório.
            }

            printf("\nProcessando arquivo: %s\n", caminho_arquivo_entrada);

            // 1. Carrega e redimensiona a imagem.
            if (carregar_e_redimensionar_imagem(caminho_arquivo_entrada, buffer_imagem_rgb) != 0) {
                fprintf(stderr, "Erro ao carregar ou redimensionar a imagem '%s'. Pulando para a próxima.\n", caminho_arquivo_entrada);
                continue; // Pula esta imagem se houver erro.
            }

            // 2. Converte a imagem RGB para escala de cinza.
            // A imagem em escala de cinza é armazenada na variável global `imagem_global_cinza`.
            converter_rgb_para_cinza(buffer_imagem_rgb, imagem_global_cinza);

            // 3. Aplica o filtro de borda selecionado.
            // A função `aplicar_filtro_operacao` usa a `imagem_global_cinza` global e armazena o resultado
            // no buffer local `buffer_resultado_filtro`.
            aplicar_filtro_operacao(kernel_selecionado_gx, kernel_selecionado_gy, codigo_tamanho_kernel_selecionado, buffer_resultado_filtro);

            // 4. Constrói o nome do arquivo de saída.
            // Remove a extensão do nome do arquivo original.
            strncpy(nome_base_arquivo_saida, entrada_diretorio->d_name, sizeof(nome_base_arquivo_saida) - 1);
            nome_base_arquivo_saida[sizeof(nome_base_arquivo_saida) - 1] = '\0';
            char *posicao_ponto_saida = strrchr(nome_base_arquivo_saida, '.');
            if (posicao_ponto_saida) {
                *posicao_ponto_saida = '\0'; // Termina a string no ponto para remover a extensão.
            }
            // Monta o caminho completo do arquivo de saída no diretório `nome_diretorio_saida`.
            snprintf(caminho_arquivo_saida, sizeof(caminho_arquivo_saida), "%s/%s_%s.png", 
                     nome_diretorio_saida, nome_base_arquivo_saida, nome_filtro_selecionado);

            // 5. Salva a imagem resultante (em escala de cinza) como PNG.
            salvar_imagem_cinza_png(caminho_arquivo_saida, buffer_resultado_filtro);
            
            printf("Processamento de '%s' concluído. Resultado salvo em '%s'.\n", entrada_diretorio->d_name, caminho_arquivo_saida);

        } // Fim do loop while (readdir)
        
        printf("\nProcessamento de todas as imagens para o filtro '%s' concluído.\n", nome_filtro_selecionado);
        // Volta para o menu de seleção de filtro.

    } // Fim do loop while (seleção de filtro)

    // --- Finalização ---
    
    // Fecha o diretório de entrada.
    closedir(ponteiro_diretorio);
    
    // Libera/desliga recursos de hardware/FPGA (função externa).
    terminate_hardware();
    
    printf("\nPrograma finalizado com sucesso.\n");
    return EXIT_SUCCESS; // Retorna sucesso.
}
