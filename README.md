# PDVPrinterSim - Emulador de Impressora Térmica

Um emulador visual de impressora térmica construído em C++ com a biblioteca gráfica SFML, ideal para acelerar o desenvolvimento e a depuração de sistemas de Ponto de Venda (PDV) sem a necessidade de uma impressora física.

O emulador atua como um servidor de impressão de rede (Raw TCP/IP na porta 9100), interpreta comandos ESC/POS e renderiza o recibo final em uma interface gráfica que simula o papel saindo de uma impressora real.

---

*A visual thermal printer emulator built with C++ and SFML, perfect for accelerating the development and debugging of Point of Sale (POS) systems without needing physical hardware.*

*The emulator acts as a network print server (Raw TCP/IP on port 9100), interprets ESC/POS commands, and renders the final receipt in a GUI that simulates paper feeding from a real printer.*

## Principais Funcionalidades / Key Features

- **Emulação de Impressora de Rede:** Escuta na porta 9100 (Raw TCP/IP), compatível com CUPS (AppSocket/JetDirect).
- **Parser ESC/POS:** Interpreta comandos de justificação, negrito, sublinhado, tamanho da fonte e corte de papel.
- **Renderização Visual:** Exibe o recibo com animação de rolagem de papel.
- **Botão de Teste Interno:** Permite disparar impressões de exemplo diretamente da interface.

## Tecnologias Utilizadas / Tech Stack

- **Linguagem:** C++
- **Interface Gráfica:** SFML
- **Rede:** Winsock2 (Windows)
