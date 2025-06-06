# 📑 Problema 3: Filtro Detector de Bordas com FPGA (DE1-SoC)

<div align="center">
  Doscente: Wild Freitas
</div>

<div align="center">
  Discentes: Gabriel Ribeiro Souza & Lyrton Marcell & Israel Oliveira
</div>

<div align="center">
  Universidade Estadual de Feira de Santana (UEFS) - Bahia
</div>

<div align="center">
  Endereço: Av. Transnordestina, S/N - Bairro: Novo Horizonte - CEP: 44036-900
</div>

## 1. Introdução

Filtros detectores de borda são técnicas fundamentais no processamento digital de imagens, permitindo identificar transições abruptas de intensidade que correspondem aos contornos e limites dos objetos presentes em uma imagem. Este projeto tem como objetivo aplicar filtros de borda sobre imagens utilizando a plataforma DE1-SoC, com o processamento das operações de convolução acelerado via FPGA, a partir de um coprocessador especializado desenvolvido previamente.

---

## 2. Objetivos

### 2.1 Objetivo Geral

Implementar um sistema embarcado que aplica filtros detectores de bordas a imagens no formato 320×240 pixels, utilizando a DE1-SoC, com a execução das convoluções realizada por um coprocessador de multiplicação matricial na FPGA.

### 2.2 Objetivos Específicos

- ✅ Ler imagens em RGB e convertê-las para escala de cinza;
- ✅ Implementar os filtros de borda: Sobel 3×3, Sobel 5×5, Prewitt 3×3, Roberts 2×2 e Laplaciano 5×5;
- ✅ Desenvolver uma interface de comunicação entre o HPS (ARM) e a FPGA via memória mapeada;
- ✅ Criar scripts em C e Assembly para envio e recebimento de dados entre processador e FPGA;
- ✅ Exibir e salvar as imagens processadas para validação visual.

---

## 3 Fundamentação Teórica

### 3.1 Convolução

A **convolução** é uma operação fundamental no processamento digital de imagens. Trata-se de um processo matemático que aplica uma matriz de pesos (chamada **máscara** ou **kernel**) sobre a imagem para realçar certas características, como **bordas**, **texturas** e **detalhes**.

O funcionamento é o seguinte:

- Uma **janela de pixels** (normalmente 3×3, 5×5 ou outro tamanho ímpar) é movida sobre toda a imagem.
- Em cada posição, os valores dos pixels dentro da janela são multiplicados pelos valores correspondentes no kernel.
- Os produtos obtidos são somados, e o resultado é atribuído ao pixel central da janela na imagem de saída.
- Esse procedimento é repetido para todos os pixels da imagem.

**Matematicamente:**

![Texto alternativo](convolucao.png)

**Onde:**

- `R(x, y)` é o valor do pixel na posição (x, y) da imagem resultante.
- `I(x+i, y+j)` é o valor do pixel da imagem original na posição (x+i, y+j).
- `K(i, j)` é o valor da máscara na posição (i, j).
- `k` depende do tamanho do kernel (ex.: 1 para 3×3, 2 para 5×5).

A convolução é extremamente útil para operações como **detecção de bordas**, **suavização**, **realce de detalhes** e **remoção de ruído**.

---

### 3.2 Filtros de Detecção de Bordas

Filtros detectores de bordas são operadores baseados em convolução que identificam regiões de transição abrupta de intensidade na imagem, locais onde há uma variação rápida entre tons claros e escuros. Cada filtro possui suas particularidades no cálculo dos gradientes e sensibilidade a ruídos e detalhes.

Abaixo, detalhamos os filtros utilizados neste projeto:

| Filtro        | Descrição Detalhada |
|:--------------|:--------------------|
| **Sobel 3×3**  | Filtro derivativo de primeira ordem que utiliza dois kernels, um para o gradiente na direção horizontal (**Gx**) e outro na vertical (**Gy**). Os pesos são maiores nas posições centrais das bordas, conferindo maior ênfase a essas regiões. Por incluir suavização, é mais robusto a ruídos do que o Prewitt. |
| **Sobel 5×5**  | Versão expandida do Sobel tradicional, onde os kernels passam de 3×3 para 5×5. Isso permite analisar uma vizinhança maior, tornando o filtro mais eficaz na detecção de bordas em imagens com muitos detalhes ou com ruídos leves, pois suaviza variações locais indesejadas. |
| **Prewitt 3×3**| Assim como o Sobel, é um filtro de primeira derivada, mas utiliza pesos uniformes no cálculo do gradiente. Por ser mais simples, é mais rápido de calcular, porém mais suscetível a ruído. Sua aplicação é semelhante à do Sobel, mas com menor precisão em imagens de baixa qualidade. |
| **Roberts 2×2**| Filtro compacto que calcula a diferença entre pixels adjacentes na diagonal. Ideal para destacar detalhes finos e contornos abruptos, mas muito sensível a ruídos. Por usar janelas 2×2, oferece alta precisão local, mas menor robustez geral. |
| **Laplaciano 5×5** | Diferentemente dos demais, é um operador de segunda derivada, ou seja, calcula a variação da variação de intensidade. Detecta regiões onde há mudanças rápidas em todas as direções. Como pode amplificar ruídos, é frequentemente combinado com pré-filtros suavizantes. A versão 5×5 permite melhor controle sobre regiões de maior variação espacial. |

---

### 3.2.1 Comparativo dos Filtros

| Filtro        | Tamanho | Tipo de Derivada | Direção Sensível       | Robustez a Ruído | Uso Ideal                                        |
|:---------------|:-----------|:----------------|:----------------------|:----------------|:-------------------------------------------------|
| **Sobel 3×3**   | 3×3        | Primeira         | Horizontal e Vertical  | Boa             | Bordas gerais com ruído moderado                 |
| **Sobel 5×5**   | 5×5        | Primeira         | Horizontal e Vertical  | Muito boa       | Bordas finas com detalhes e ruído                |
| **Prewitt 3×3** | 3×3        | Primeira         | Horizontal e Vertical  | Média           | Bordas simples em imagens limpas                 |
| **Roberts 2×2** | 2×2        | Primeira         | Diagonal               | Baixa           | Detalhes finos e transições abruptas             |
| **Laplaciano 5×5** | 5×5     | Segunda          | Todas as direções      | Baixa a Média   | Detectar regiões de alta variação geral          |

---

## 4 Metodologia

O projeto foi desenvolvido na plataforma **DE1-SoC**, combinando um processador **ARM Cortex-A9** (HPS) e uma **FPGA Cyclone V**. A metodologia foi organizada em quatro etapas principais:

### 4.1 Linguagens Utilizadas

- **C**  
  Responsável por toda a aplicação de alto nível, incluindo:
  - Leitura e redimensionamento das imagens.
  - Conversão para escala de cinza.
  - Controle do protocolo de envio e recepção de dados para a FPGA.
  - Aplicação dos filtros de detecção de borda.
  - Salvamento das imagens processadas.

- **Assembly ARM (ARMv7)**  
  Utilizado para a implementação de rotinas de comunicação de baixo nível com a FPGA, incluindo:
  - Mapeamento de memória (`/dev/mem`).
  - Escrita e leitura nos registradores da FPGA.
  - Implementação de handshakes síncronos via registradores de controle.

---

### 4.2 Bibliotecas Externas

- **stb_image**  
  Biblioteca em C para leitura de imagens nos formatos **PNG**, **JPEG** e **BMP**. Utilizada para carregar imagens RGB e obter suas dimensões.

- **stb_image_write**  
  Complementa a stb_image, permitindo salvar imagens processadas em **formato PNG** após a aplicação dos filtros.

---

### 4.3 Coprocessador de Multiplicação Matricial (FPGA)

A FPGA contém um coprocessador dedicado à multiplicação elemento a elemento de duas matrizes 5×5 (janela da imagem e kernel do filtro). Esse módulo:
- Recebe dados via registradores mapeados.
- Realiza o produto e soma acumulada.
- Retorna o resultado convolucional.
- Opera sob controle de sinais de `start`, `reset` e `ack` (acknowledge) via bits de controle.

---

### 4.4 Protocolo de Comunicação HPS ↔ FPGA

Para comunicação entre o processador ARM e a FPGA, foi implementado um protocolo baseado em:

- **Memória mapeada** via `/dev/mem` para acesso aos registradores da FPGA diretamente pelo espaço de endereçamento virtual do Linux embarcado.
- **Handshakes síncronos**, controlados por bits específicos:
  - **Bit 31 (start / ack)**: sinaliza início de envio ou leitura de dados.
  - **Bits 0–28**: utilizados para enviar os valores dos pixels, pesos dos kernels e parâmetros de operação (opcode e tamanho).

O protocolo garante integridade e sincronização entre as etapas de envio de dados, execução na FPGA e leitura dos resultados.

---

### 4.5 Fluxo Operacional

1. O usuário seleciona o filtro desejado.
2. O HPS percorre todas as imagens no diretório `input/`.
3. Para cada pixel da imagem:
   - Extrai uma janela de pixels.
   - Envia a janela e o kernel para a FPGA.
   - Solicita a operação de convolução.
   - Recebe o resultado processado.
4. Armazena os resultados na imagem de saída.
5. Salva a imagem final no diretório `output/` com identificação do filtro aplicado.

---

## 5 Desenvolvimento

Este projeto realiza processamento de imagens com filtros de detecção de bordas utilizando um coprocessador na FPGA da plataforma DE1-SoC, com interação via HPS (ARM Cortex-A9) através de memória mapeada.

### 5.1 `main.c`

Responsável pelo fluxo completo de processamento das imagens:

- Leitura e redimensionamento de imagens de entrada no diretório `input/`, utilizando `stb_image`;
- Conversão de imagens RGB para escala de cinza com base na fórmula de luminância: (0.299*R + 0.587*G + 0.114*B);
- Extração de janelas de vizinhança (2×2, 3×3 ou 5×5) ao redor de cada pixel, com tratamento de bordas (padding zero);
- Envio das janelas e kernels para a FPGA, através da função `transfer_data_to_fpga()`;
- Recepção dos resultados processados via `retrieve_fpga_results()` e reconstrução do valor final;
- Cálculo da magnitude do gradiente (`√(Gx² + Gy²)`) para filtros com duas direções (Gx/Gy), ou valor absoluto para Laplace (unidirecional);
- Saturação dos valores convoluídos para a faixa [0, 255];
- Salvamento da imagem resultante no diretório `output/` como PNG com nome indicativo do filtro utilizado.

O código também permite **seleção dinâmica do filtro desejado via terminal**:
- Sobel 3x3 / 5x5  
- Prewitt 3x3  
- Roberts 2x2  
- Laplace 5x5

---

### 5.2 `hps_0.h`

Arquivo de cabeçalho contendo:

#### 5.2.1 Definições de constantes:
- `MATRIX_SIZE` (25): representa o tamanho linear de uma matriz 5×5;
- `HW_SUCCESS` e `HW_SEND_FAIL`: códigos de retorno para interface FPGA.

### Estrutura de dados `Params` usada para empacotar os argumentos enviados à FPGA:
```c
struct Params {
  const uint8_t* a;   // Ponteiro para janela de pixels
  const int8_t* b;    // Ponteiro para kernel do filtro
  uint32_t opcode;    // Código de operação (e.g., 7 = convolução)
  uint32_t size;      // Tamanho do kernel (interpretação FPGA)
};

