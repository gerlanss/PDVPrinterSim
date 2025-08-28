// main.cpp
#include <SFML/Graphics.hpp>
#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <queue>
#include <mutex>

#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

#define DEFAULT_PORT "9100"
#define DEFAULT_BUFLEN 512

// Estrutura para guardar o estado atual da formatação da impressora
struct PrinterState {
    enum Justification { LEFT, CENTER, RIGHT };
    Justification justification = LEFT;
    bool isBold = false;
    bool isUnderlined = false;
    int widthMultiplier = 1;
    int heightMultiplier = 1;
};

// Estrutura para enviar para a fila: contém o texto e a formatação daquela linha
struct PrintLine {
    std::string text;
    std::string leftText;
    std::string rightText; // Usado para a segunda coluna (ex: valor)    
    PrinterState state; // Uma cópia do estado no momento da impressão da linha
};

// --- DADOS GLOBAIS PARA COMUNICAÇÃO ENTRE THREADS ---
std::queue<PrintLine> g_printQueue; 
std::mutex g_queueMutex;
// ----------------------------------------------------

// Adicione estas duas funções antes de run_server()

// Retorna os dados do primeiro recibo de teste
std::string getTestReceipt1() {
    std::string msg;
    // O PHP agora envia os dados separados por '|' para o C++ alinhar
    msg += "\x1B\x61\x01"; // Centralizar
    msg += "\x1D\x21\x11"; // Fonte Grande
    msg += "CAFE DO BOM\n";
    msg += "\x1B\x40"; // Resetar
    msg += "----------------------------------------\n";
    msg += "ITEM|VALOR (R$)\n";
    msg += "Aluguel Quadra - Hora|100,00\n";
    msg += "----------------------------------------\n";
    msg += "\x1B\x45\x01"; // Negrito ON
    msg += "TOTAL:|R$ 100,00\n";
    msg += "\x1B\x40"; // Resetar
    msg += "\x1B\x61\x01"; // Centralizar
    msg += "Prefeitura Municipal de Pocos de Caldas\n";
    msg += "18.722.710/0001-80\n\n\n";
    msg += "\x1D\x56\x01"; // Cortar Papel
    return msg;
}

// Retorna os dados do segundo recibo de teste
std::string getTestReceipt2() {
    std::string msg;
    msg += "PROXIMA VENDA\n";
    msg += "Agua Mineral          1    R$ 4,00\n";
    msg += "\n\n\n"; // Pula algumas linhas
    msg += "\x1D\x56\x01"; // GS V 1 (Cortar Papel)
    return msg;
}

// Adicione esta função também, ela será o nosso "PowerShell" interno
void sendDataToServer(const std::string& data) {
    SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (clientSocket == INVALID_SOCKET) {
        std::cerr << "[CLIENTE_TESTE] Erro ao criar socket." << std::endl;
        return;
    }

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(9100);
    inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr);

    if (connect(clientSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "[CLIENTE_TESTE] Nao foi possivel conectar ao servidor. O servidor esta rodando?" << std::endl;
        closesocket(clientSocket);
        return;
    }

    send(clientSocket, data.c_str(), data.length(), 0);
    
    closesocket(clientSocket);
}

void run_server() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "[SERVIDOR] WSAStartup falhou." << std::endl;
        return;
    }
    std::cout << "[SERVIDOR] Winsock inicializado." << std::endl;

    struct addrinfo *result = NULL, hints;
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    getaddrinfo(NULL, DEFAULT_PORT, &hints, &result);
    SOCKET listenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    bind(listenSocket, result->ai_addr, (int)result->ai_addrlen);
    freeaddrinfo(result);

    std::cout << "[SERVIDOR] Aguardando conexoes na porta " << DEFAULT_PORT << "..." << std::endl;
    while (true) {
        if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR) {
            std::cerr << "[SERVIDOR] Listen falhou." << std::endl;
            closesocket(listenSocket);
            WSACleanup();
            return;
        }

        SOCKET clientSocket = accept(listenSocket, NULL, NULL);
        if (clientSocket == INVALID_SOCKET) {
            std::cerr << "[SERVIDOR] Accept falhou." << std::endl;
            continue;
        }
        std::cout << "[SERVIDOR] Cliente conectado!" << std::endl;

        // --- LINHAS CORRIGIDAS ---
        char recvbuf[DEFAULT_BUFLEN];
        int bytesReceived;

        PrinterState currentState; // Estado da impressora para esta conexão
        std::string currentTextLine; // Acumulador de texto para a linha atual

        do {
            bytesReceived = recv(clientSocket, recvbuf, DEFAULT_BUFLEN, 0);
            if (bytesReceived > 0) {
                // Itera por cada byte recebido
                for (int i = 0; i < bytesReceived; ++i) {
                    unsigned char byte = recvbuf[i];

                    switch (byte) {
                        case 0x1B: // Comando ESC
                        {
                            if (i + 1 < bytesReceived) {
                                i++; 
                                unsigned char commandByte = recvbuf[i];
                                
                                if (commandByte == '@') { // ESC @ - Inicializar Impressora
                                    currentState = PrinterState(); // Reseta TUDO para o estado padrão
                                } 
                                else if (commandByte == 'a') { // ESC a - Justificação
                                    if (i + 1 < bytesReceived) {
                                        i++;
                                        unsigned char param = recvbuf[i];
                                        if (param == 0 || param == 48) currentState.justification = PrinterState::LEFT;
                                        else if (param == 1 || param == 49) currentState.justification = PrinterState::CENTER;
                                        else if (param == 2 || param == 50) currentState.justification = PrinterState::RIGHT;
                                    }
                                }
                                else if (commandByte == 'E') { // ESC E - Negrito
                                    if (i + 1 < bytesReceived) {
                                        i++;
                                        currentState.isBold = (recvbuf[i] == 1);
                                    }
                                }
                                else if (commandByte == '-') { // ESC - - Sublinhado
                                    if (i + 1 < bytesReceived) {
                                        i++;
                                        currentState.isUnderlined = (recvbuf[i] == 1);
                                    }
                                }
                            }
                            break;
                        }
                        case 0x1D: // Comando GS
                        {
                            if (i + 1 < bytesReceived) {
                                i++;
                                unsigned char commandByte = recvbuf[i];
                                if (commandByte == '!') { // GS ! - Tamanho do caractere
                                    if (i + 1 < bytesReceived) {
                                        i++;
                                        unsigned char param = recvbuf[i];
                                        currentState.widthMultiplier = ((param >> 0) & 0x0F) + 1;
                                        currentState.heightMultiplier = ((param >> 4) & 0x0F) + 1;
                                    }
                                }
                                // --- NOVO COMANDO ADICIONADO AQUI ---
                                else if (commandByte == 'V') { // GS V - Cortar Papel
                                    // Parâmetro (m) não é necessário para a simulação, então o ignoramos.
                                    // Apenas enviamos uma linha especial para a fila.
                                    PrintLine cutLine;
                                    cutLine.text = "--[CUT]--"; // Um "código secreto" para a parte gráfica
                                    cutLine.state = currentState; // O estado não importa, mas precisa ser enviado
                                    
                                    std::lock_guard<std::mutex> lock(g_queueMutex);
                                    g_printQueue.push(cutLine);
                                }
                                // ---------------------------------------
                            }
                            break;
                        }
                       case 0x0A: // Comando LF (Nova Linha) - LÓGICA ATUALIZADA
                        {
                            PrintLine line;
                            line.state = currentState;

                            // Verifica se a linha tem o delimitador de coluna '|'
                            size_t delimiterPos = currentTextLine.find('|');
                            if (delimiterPos != std::string::npos) {
                                line.leftText = currentTextLine.substr(0, delimiterPos);
                                line.rightText = currentTextLine.substr(delimiterPos + 1);
                            } else {
                                line.leftText = currentTextLine;
                                line.rightText = ""; // Sem texto à direita
                            }

                            std::lock_guard<std::mutex> lock(g_queueMutex);
                            g_printQueue.push(line);
                            
                            currentTextLine.clear();
                            break;
                        }
                        default:
                        {
                            // Caractere normal, adiciona ao texto da linha
                            if (byte != '\r') { // Ignora Carriage Return
                                currentTextLine += byte;
                            }
                            break;
                        }
                    }
                }
            }
        } while (bytesReceived > 0);


        std::cout << "[SERVIDOR] Cliente desconectado." << std::endl;
        shutdown(clientSocket, SD_SEND);
        closesocket(clientSocket);
    }
    closesocket(listenSocket);
    WSACleanup();
}


int main() {
    std::thread serverThread(run_server);
    serverThread.detach();

    // --- CONFIGURAÇÃO GRÁFICA ---
    sf::Font font;
    if (!font.loadFromFile("assets/consola.ttf")) {
        std::cerr << "[GRAFICOS] Erro: Nao foi possivel carregar a fonte." << std::endl;
        return -1;
    }
    std::cout << "[GRAFICOS] Fonte carregada com sucesso." << std::endl;

    sf::Texture printerTexture;
    if (!printerTexture.loadFromFile("assets/printer2.png")) {
        std::cerr << "[GRAFICOS] Erro: Nao foi possivel carregar a imagem 'printer2.png'." << std::endl;
        return -1;
    }
    sf::Sprite printerSprite(printerTexture);
    
    float larguraDesejada = 581.0f; // <-- MUDE A LARGURA AQUI
    float alturaDesejada = 521.0f;  // <-- MUDE A ALTURA AQUI

    float fatorEscalaX = larguraDesejada / printerTexture.getSize().x;
    float fatorEscalaY = alturaDesejada / printerTexture.getSize().y;

    printerSprite.setScale(fatorEscalaX, fatorEscalaY);

    sf::RenderWindow window(sf::VideoMode(larguraDesejada, alturaDesejada + 80), "PDVPrinterSim - Emulador de Impressora Termica");
    window.setFramerateLimit(60);

    //sf::RectangleShape paper(sf::Vector2f(300, 210));
    sf::RectangleShape paper(sf::Vector2f(384, 315));
    paper.setFillColor(sf::Color::Yellow);
    paper.setPosition(140, 79);

    std::vector<std::pair<sf::Text, sf::Text>> lines;
    unsigned int characterSize = 14;
    float paperLeftMargin = paper.getPosition().x + 5;
    float paperRightMargin = paper.getPosition().x + paper.getSize().x - 5;
    float paperTopMargin = paper.getPosition().y + 5;

    // --- BOTÃO DE TESTE ---
    sf::RectangleShape testButton(sf::Vector2f(200, 40));
    testButton.setFillColor(sf::Color(100, 100, 100));
    testButton.setPosition((window.getSize().x - testButton.getSize().x) / 2.0f, 535);

    sf::Text buttonText;
    buttonText.setFont(font);
    buttonText.setString("Imprimir Teste");
    buttonText.setCharacterSize(20);
    buttonText.setFillColor(sf::Color::White);
    buttonText.setPosition(
        testButton.getPosition().x + (testButton.getSize().x - buttonText.getLocalBounds().width) / 2.0f,
        testButton.getPosition().y + (testButton.getSize().y - buttonText.getLocalBounds().height) / 2.0f - 5
    );    

    // --- NOVAS VARIÁVEIS PARA ANIMAÇÃO ---
    sf::Clock deltaClock;
    float scrollTarget = 0.0f;
    float animationSpeed = 120.0f;
    const float printHeadY = paper.getPosition().y + paper.getSize().y - 60; 
    
    bool receiptHasBeenCut = false;
    
    // --- LOOP PRINCIPAL DA APLICAÇÃO ---
    while (window.isOpen()) {
        sf::Time dt = deltaClock.restart();
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();

            if (event.type == sf::Event::MouseButtonPressed) {
                if (event.mouseButton.button == sf::Mouse::Left) {
                    sf::Vector2f mousePos = window.mapPixelToCoords(sf::Mouse::getPosition(window));
                    if (testButton.getGlobalBounds().contains(mousePos)) {
                        std::cout << "[BOTAO] Botao de teste clicado!" << std::endl;
                        
                        std::thread testThread([]() {
                            sendDataToServer(getTestReceipt1());
                            std::this_thread::sleep_for(std::chrono::seconds(2));
                            //sendDataToServer(getTestReceipt2());
                        });
                        testThread.detach();
                    }
                }
            }
        }
        
        // --- LÓGICA DE PROCESSAMENTO DE DADOS (ATUALIZADA) ---
        PrintLine lineData;
        bool hasData = false;
        {
            std::lock_guard<std::mutex> lock(g_queueMutex);
            if (!g_printQueue.empty()) {
                lineData = g_printQueue.front();
                g_printQueue.pop();
                hasData = true;
            }
        }

        if (hasData) {
            if (receiptHasBeenCut) {
                lines.clear();
                receiptHasBeenCut = false;
            }

            // Lida com o comando de corte de forma visual
            if (lineData.text == "--[CUT]--") {
                receiptHasBeenCut = true;
                lineData.text = "--------------------------------"; // Substitui o comando por uma linha visual
                lineData.state.justification = PrinterState::CENTER; // Centraliza a linha de corte
            }
            else {
                sf::Text leftText, rightText;
                sf::Text newLine;
                float textWidth = newLine.getGlobalBounds().width;
                float xPos = paperLeftMargin;

                // Configura ambos os textos (esquerda e direita) com o mesmo estilo
                sf::Uint32 style = sf::Text::Regular;
                if (lineData.state.isBold) style |= sf::Text::Bold;
                if (lineData.state.isUnderlined) style |= sf::Text::Underlined;
                sf::Vector2f scale(lineData.state.widthMultiplier, lineData.state.heightMultiplier);                

                // Configura o texto da esquerda
                leftText.setFont(font);
                leftText.setString(lineData.leftText);
                leftText.setCharacterSize(characterSize);
                leftText.setFillColor(sf::Color::Black);
                leftText.setStyle(style);
                leftText.setScale(scale);

                // Configura o texto da direita, se existir
                if (!lineData.rightText.empty()) {
                    rightText.setFont(font);
                    rightText.setString(lineData.rightText);
                    rightText.setCharacterSize(characterSize);
                    rightText.setFillColor(sf::Color::Black);
                    rightText.setStyle(style);
                    rightText.setScale(scale);
                }

                // --- LÓGICA DE POSICIONAMENTO COM ALTURA FIXA E VARIÁVEL ---
                float yPos = paperTopMargin;
                float currentLineHeight = std::max(leftText.getGlobalBounds().height, rightText.getGlobalBounds().height);

                if (!lines.empty()) {
                    const auto& lastLine = lines.back().first;
                    float lastLineHeight = lastLine.getGlobalBounds().height;

                    // Lógica para adicionar espaço extra após linhas grandes
                    if (lastLine.getScale().y > 1.0f) {
                        lastLineHeight += 10.0f; // Adiciona um respiro
                    } else {
                        // Para linhas normais, garante uma altura mínima para evitar sobreposição
                        lastLineHeight = std::max(lastLineHeight, 20.0f);
                    }
                    yPos = lastLine.getPosition().y + lastLineHeight;
                }
                newLine.setPosition(xPos, yPos);

                // Posiciona a parte esquerda do texto
                float xPosLeft = paperLeftMargin;
                if (lineData.state.justification == PrinterState::CENTER) {
                    xPosLeft = paper.getPosition().x + (paper.getSize().x - leftText.getGlobalBounds().width) / 2.0f;
                } else if (lineData.state.justification == PrinterState::RIGHT) {
                    xPosLeft = paperRightMargin - leftText.getGlobalBounds().width;
                }
                leftText.setPosition(xPosLeft, yPos);

                // Posiciona a parte direita do texto (sempre alinhada à direita)
                if (!lineData.rightText.empty()) {
                    rightText.setPosition(paperRightMargin - rightText.getGlobalBounds().width, yPos);
                }

                // Lógica de rolagem instantânea
                float paperBottom = paper.getPosition().y + paper.getSize().y;
                if (yPos + currentLineHeight > paperBottom) {
                    float scrollOffset = (yPos + currentLineHeight) - paperBottom;
                    for (auto& linePair : lines) {
                        linePair.first.move(0, -scrollOffset);
                        linePair.second.move(0, -scrollOffset);
                    }
                    leftText.move(0, -scrollOffset);
                    rightText.move(0, -scrollOffset);
                }
                lines.push_back({leftText, rightText});
            }
        }

        // --- SEÇÃO DE DESENHO ATUALIZADA ---
        window.clear(sf::Color(50, 50, 50));
        window.draw(printerSprite);
        window.draw(paper);
        for (const auto& linePair : lines) {
            // Desenha as duas partes de cada linha
            if(linePair.first.getGlobalBounds().intersects(paper.getGlobalBounds())) window.draw(linePair.first);
            if(linePair.second.getGlobalBounds().intersects(paper.getGlobalBounds())) window.draw(linePair.second);
        }
        window.draw(testButton);
        window.draw(buttonText);
        window.display();
    }
    return 0;
}