#include <iostream> // Entrada e saída padrão
#include <string> // Manipulação de strings
#include <assert.h> // Assertivas para debug
#include <cstring> // Funções de manipulação de strings em C
using namespace std; // Uso do namespace padrão

#include <glad/glad.h> // Loader para funções OpenGL modernas
#include <GLFW/glfw3.h> // Biblioteca para criação de janelas e captura de input

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h" // Biblioteca para carregar imagens (texturas)

// Constantes de configuração do mapa e tiles
const int TILE_WIDTH = 64;
const int TILE_HEIGHT = 32;
const int MAP_WIDTH = 15;
const int MAP_HEIGHT = 15;
const int TOTAL_COINS = 4;

// Matriz que define o layout do mapa
int map[MAP_HEIGHT][MAP_WIDTH] = {
    //0 = Areia
    //1 = Grama
    //2 = Pedra
    //3 = Lava
    //4 = Agua raza
    //5 = Agua profunda
    //6 = Jogador
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 3, 3, 3},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 3, 3},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 3},
    {1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 2},
    {1, 1, 1, 1, 1, 1, 0, 4, 0, 1, 1, 1, 1, 1, 1},
    {1, 1, 1, 1, 1, 0, 4, 5, 4, 0, 1, 1, 1, 1, 1},
    {1, 1, 1, 1, 0, 4, 5, 5, 5, 4, 0, 1, 1, 1, 1},
    {1, 1, 1, 0, 4, 5, 5, 5, 5, 5, 4, 0, 1, 1, 1},
    {1, 1, 1, 1, 0, 4, 5, 5, 5, 4, 0, 1, 1, 1, 1},
    {1, 1, 1, 1, 1, 0, 4, 5, 4, 0, 1, 1, 1, 1, 1},
    {1, 1, 1, 1, 1, 1, 0, 4, 0, 1, 1, 1, 1, 1, 1},
    {1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
    {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}
};

// Matriz de posição das moedas (1 = moeda presente, 0 = vazio)
int coinMap[MAP_HEIGHT][MAP_WIDTH] = { 0 };
int coinCount = 0; // Contador de moedas coletadas

// Posição atual do personagem
int selectedX = 1, selectedY = 1;

// Texturas utilizadas no jogo
GLuint tilesetTexture, vampTexture, coinTexture;

// Animação do personagem
int currentFrame = 0;
float frameTime = 0.15f; // Tempo entre os quadros da animação
float frameTimer = 0.0f;
int frameCols = 5; // Colunas da sprite sheet
int frameRows = 4; // Linhas da sprite sheet (uma por direção)
int direction = 1; // Direção atual: 0=baixo, 1=cima, 2=esq, 3=dir
bool isMoving = false; // Indica se o personagem está se movendo

// Controle de movimentação na lava
int lastDirection = -1;
bool lavaIntent = false;

// Função que carrega uma textura de imagem para a GPU
bool loadTexture(const char* path, GLuint& outTexture) {
    int width, height, nrChannels;
    unsigned char* data = stbi_load(path, &width, &height, &nrChannels, 4);
    if (!data) {
        std::cerr << "Erro ao carregar imagem: " << path << std::endl;
        return false;
    }

    glGenTextures(1, &outTexture);
    glBindTexture(GL_TEXTURE_2D, outTexture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glBindTexture(GL_TEXTURE_2D, 0);
    stbi_image_free(data);
    return true;
}

// Reinicia o jogo e reposiciona moedas
void resetGame() {
    selectedX = 1;
    selectedY = 1;
    coinCount = 0;
    memset(coinMap, 0, sizeof(coinMap));
    coinMap[2][2] = 1; // Posição moeda 1
    coinMap[5][5] = 1; // Posição moeda 2
    coinMap[0][14] = 1; // Posição moeda 3
    coinMap[12][10] = 1; // Posição moeda 4
    std::cout << "Todas as moedas coletadas! Reiniciando jogo...\n";
}

// Desenha um tile isométrico com base na posição e textura
void drawTile(int i, int j, int tileIndex, bool isSelected) {
    int originX = 500;
    int originY = 400 + (MAP_HEIGHT * TILE_HEIGHT / 2) / 2;
    int screenX = originX + (i - j) * (TILE_WIDTH / 2);
    int screenY = originY - (i + j) * (TILE_HEIGHT / 2);
    float u0 = tileIndex / 7.0f;
    float u1 = (tileIndex + 1) / 7.0f;

    glBindTexture(GL_TEXTURE_2D, tilesetTexture);
    glBegin(GL_QUADS);
        glColor3f(isSelected ? 1.0f : 0.8f, 1.0f, 1.0f); // Tile selecionado fica mais claro
        glTexCoord2f((u0 + u1) / 2, 1.0f); glVertex2f(screenX, screenY + TILE_HEIGHT / 2);
        glTexCoord2f(u1, 0.5f); glVertex2f(screenX + TILE_WIDTH / 2, screenY);
        glTexCoord2f((u0 + u1) / 2, 0.0f); glVertex2f(screenX, screenY - TILE_HEIGHT / 2);
        glTexCoord2f(u0, 0.5f); glVertex2f(screenX - TILE_WIDTH / 2, screenY);
    glEnd();
}

// Desenha todas as moedas no mapa
void drawCoins() {
    int originX = 500;
    int originY = 400 + (MAP_HEIGHT * TILE_HEIGHT / 2) / 2;

    glBindTexture(GL_TEXTURE_2D, coinTexture);
    glColor3f(1.0f, 1.0f, 1.0f);

    for (int y = 0; y < MAP_HEIGHT; ++y) {
        for (int x = 0; x < MAP_WIDTH; ++x) {
            if (coinMap[y][x] == 1) {
                int screenX = originX + (x - y) * (TILE_WIDTH / 2);
                int screenY = originY - (x + y) * (TILE_HEIGHT / 2);
                int size = 32;
                int verticalOffset = 20; // Posicionamento fino da moeda no tile

                glBegin(GL_QUADS);
                    glTexCoord2f(0.0f, 1.0f); glVertex2f(screenX - size / 2, screenY + TILE_HEIGHT / 2 + size - verticalOffset);
                    glTexCoord2f(1.0f, 1.0f); glVertex2f(screenX + size / 2, screenY + TILE_HEIGHT / 2 + size - verticalOffset);
                    glTexCoord2f(1.0f, 0.0f); glVertex2f(screenX + size / 2, screenY + TILE_HEIGHT / 2 - verticalOffset);
                    glTexCoord2f(0.0f, 0.0f); glVertex2f(screenX - size / 2, screenY + TILE_HEIGHT / 2 - verticalOffset);
                glEnd();
            }
        }
    }
}

// Desenha o personagem animado baseado em direção e tempo
void drawVampirinhoAnimado(float deltaTime) {
    if (isMoving) {
        frameTimer += deltaTime;
        if (frameTimer >= frameTime) {
            frameTimer = 0.0f;
            currentFrame = (currentFrame + 1) % frameCols;
        }
    } else {
        currentFrame = 0;
    }

    float u0 = currentFrame / (float)frameCols;
    float v0 = direction / (float)frameRows;
    float u1 = (currentFrame + 1) / (float)frameCols;
    float v1 = (direction + 1) / (float)frameRows;

    int originX = 500;
    int originY = 400 + (MAP_HEIGHT * TILE_HEIGHT / 2) / 2;
    int screenX = originX + (selectedX - selectedY) * (TILE_WIDTH / 2);
    int screenY = originY - (selectedX + selectedY) * (TILE_HEIGHT / 2);

    int spriteSize = 64;
    int offsetY = 40; //Ajuste fino posição personagem vertical 
    int offsetX = -7; //Ajuste fino posição personagem horizontal

    glBindTexture(GL_TEXTURE_2D, vampTexture);
    if (map[selectedY][selectedX] == 3) // Se estiver na lava, personagem fica vermelho
        glColor3f(1.0f, 0.3f, 0.3f);
    else
        glColor3f(1.0f, 1.0f, 1.0f);

    glBegin(GL_QUADS);
        glTexCoord2f(u0, v0); glVertex2f(screenX - spriteSize / 2 + offsetX, screenY + TILE_HEIGHT / 2 + spriteSize - offsetY);
        glTexCoord2f(u1, v0); glVertex2f(screenX + spriteSize / 2 + offsetX, screenY + TILE_HEIGHT / 2 + spriteSize - offsetY);
        glTexCoord2f(u1, v1); glVertex2f(screenX + spriteSize / 2 + offsetX, screenY + TILE_HEIGHT / 2 - offsetY);
        glTexCoord2f(u0, v1); glVertex2f(screenX - spriteSize / 2 + offsetX, screenY + TILE_HEIGHT / 2 - offsetY);
    glEnd();
}

// Função de callback para tratar entradas do teclado
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (action != GLFW_PRESS) return; // Só reage ao pressionamento de tecla

    int newX = selectedX, newY = selectedY;

    // Movimentação baseada nas teclas WAXD + diagonais QEZC
    switch (key) {
        case GLFW_KEY_W: if (selectedY > 0) newY--; direction = 1; break; // cima
        case GLFW_KEY_X: if (selectedY < MAP_HEIGHT - 1) newY++; direction = 0; break; // baixo
        case GLFW_KEY_A: if (selectedX > 0) newX--; direction = 2; break; // esquerda
        case GLFW_KEY_D: if (selectedX < MAP_WIDTH - 1) newX++; direction = 3; break; // direita
        case GLFW_KEY_Q: if (selectedX > 0 && selectedY > 0) { newX--; newY--; direction = 1; } break; // cima-esquerda
        case GLFW_KEY_E: if (selectedX < MAP_WIDTH - 1 && selectedY > 0) { newX++; newY--; direction = 3; } break; // cima-direita
        case GLFW_KEY_Z: if (selectedX > 0 && selectedY < MAP_HEIGHT - 1) { newX--; newY++; direction = 2; } break; // baixo-esquerda
        case GLFW_KEY_C: if (selectedX < MAP_WIDTH - 1 && selectedY < MAP_HEIGHT - 1) { newX++; newY++; direction = 0; } break; // baixo-direita
    }

    // Impede movimentação para tiles de bloqueio (4 = parede, 5 = obstáculo)
    if (map[newY][newX] == 4 || map[newY][newX] == 5) return;
    // Controle especial para pisar na lava (tile 3)
    if (map[newY][newX] == 3) {
        // Só pode andar se pressionar 2 vezes na mesma direção
        if (lavaIntent && direction == lastDirection) {
            selectedX = newX;
            selectedY = newY;
            isMoving = true;
            lavaIntent = false;
        } else {
            lavaIntent = true;
            lastDirection = direction;
            std::cout << "Voce esta prestes a pisar na lava! Pressione novamente na mesma direcao.\n";
        }
    } else {
        // Movimento normal
        selectedX = newX;
        selectedY = newY;
        isMoving = true;
        lavaIntent = false;
    }

    // Atualiza a última direção pressionada
    lastDirection = direction;
}
// Função principal do programa
int main() {
    if (!glfwInit()) return -1; // Inicializa o GLFW

    // Cria a janela principal
    GLFWwindow* window = glfwCreateWindow(1000, 600, "Tilemap Isometrico com Moedas", NULL, NULL);
    if (!window) { glfwTerminate(); return -1; }

    glfwMakeContextCurrent(window); // Define o contexto atual
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) return -1; // Carrega as funções OpenGL

    glfwSetKeyCallback(window, key_callback); // Registra função de callback para teclado

    // Configuração da projeção ortográfica (2D)
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, 1000, 0, 600, -1, 1); // Tela de 1000x600
    glMatrixMode(GL_MODELVIEW);

    // Ativa texturas e transparência
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Carrega texturas
    if (!loadTexture("../assets/tilesets/tilesetIso.png", tilesetTexture)) return -1;
    if (!loadTexture("../assets/sprites/VampireWalk.png", vampTexture)) return -1;
    if (!loadTexture("../assets/sprites/Moeda.png", coinTexture)) return -1;

    // Inicializa o jogo (posição e moedas)
    resetGame();

    float lastTime = glfwGetTime(); // Marca o tempo inicial

    // Loop principal do jogo
    while (!glfwWindowShouldClose(window)) {
        float currentTime = glfwGetTime();
        float deltaTime = currentTime - lastTime;
        lastTime = currentTime;

        glClear(GL_COLOR_BUFFER_BIT); // Limpa a tela
        glLoadIdentity();

        // Desenha o mapa tile por tile
        for (int j = 0; j < MAP_HEIGHT; ++j)
            for (int i = 0; i < MAP_WIDTH; ++i)
                drawTile(i, j, (i == selectedX && j == selectedY) ? 6 : map[j][i], i == selectedX && j == selectedY);

        // Checa e coleta moeda se estiver no mesmo tile
        if (coinMap[selectedY][selectedX] == 1) {
            coinMap[selectedY][selectedX] = 0;
            coinCount++;
            std::cout << "Moedas coletadas: " << coinCount << std::endl;
            if (coinCount >= TOTAL_COINS) resetGame(); // Reinicia se coletar todas
        }

        drawCoins(); // Renderiza moedas
        drawVampirinhoAnimado(deltaTime); // Renderiza personagem com animação
        isMoving = false; // Reseta o estado de movimento

        glfwSwapBuffers(window); // Troca os buffers (renderização)
        glfwPollEvents(); // Processa eventos (teclado, mouse, etc.)
    }

    glfwDestroyWindow(window); // Destroi a janela
    glfwTerminate(); // Finaliza o GLFW
    return 0;
}
