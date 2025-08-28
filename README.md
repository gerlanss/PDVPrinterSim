# PDVPrinterSim - A Thermal Printer Emulator

<p align="center">
  <a href="https://isocpp.org/"><img src="https://img.shields.io/badge/Language-C++-blue.svg" alt="Language C++"></a>
  <a href="https://www.sfml-dev.org/"><img src="https://img.shields.io/badge/Made%20with-SFML-green.svg" alt="Made with SFML"></a>
  <a href="https://www.microsoft.com/windows/"><img src="https://img.shields.io/badge/Platform-Windows-0078D6.svg" alt="Platform Windows"></a>
  <a href="https://github.com/vhpontes/PDVPrinterSim/blob/main/LICENSE"><img src="https://img.shields.io/badge/License-MIT-yellow.svg" alt="License MIT"></a>
</p>

A visual thermal printer emulator built with C++ and SFML, perfect for accelerating the development and debugging of Point of Sale (POS) systems without needing physical hardware.

The emulator acts as a network print server (Raw TCP/IP on port 9100), interprets ESC/POS commands, and renders the final receipt in a GUI that simulates paper feeding from a real printer.

> ---
> ***(Tradução em Português)***
>
> *Um emulador visual de impressora térmica construído em C++ com a biblioteca gráfica SFML, ideal para acelerar o desenvolvimento e a depuração de sistemas de Ponto de Venda (PDV) sem a necessidade de uma impressora física.*
>
> *O emulador atua como um servidor de impressão de rede (Raw TCP/IP na porta 9100), interpreta comandos ESC/POS e renderiza o recibo final em uma interface gráfica que simula o papel saindo de uma impressora real.*

## ✨ Key Features / Principais Funcionalidades

- **Network Printer Emulation:** Listens on port 9100 (Raw TCP/IP), compatible with CUPS (AppSocket/JetDirect).
- **ESC/POS Parser:** Interprets commands for justification, bold, underline, font size, and paper cut.
- **Visual Rendering:** Displays the receipt with a continuous paper roll simulation.
- **Internal Test Button:** Allows triggering sample print jobs directly from the interface for quick tests.

## 🛠️ Tech Stack / Tecnologias Utilizadas

- **Language:** C++
- **GUI:** SFML (Simple and Fast Multimedia Library)
- **Networking:** Winsock2 (Windows)

## 🚀 Getting Started / Como Compilar e Usar

### Prerequisites / Pré-requisitos
- A C++ compiler (MinGW-w64 is recommended for Windows)
- The SFML library (v2.5.1 or newer) correctly installed and configured.

### Compilation / Compilação
Navigate to the project's root directory in your terminal and run the following command to compile the application:

```bash
g++ main.cpp -o PDVPrinterSim.exe -lsfml-graphics -lsfml-window -lsfml-system -lws2_32 -pthread -std=c++17

### Usage / Uso

1.  Execute o arquivo compilado: `./PDVPrinterSim.exe`.
2.  O emulador será iniciado e ficará aguardando por conexões na porta 9100.
3.  Envie os dados brutos em formato ESC/POS a partir da sua aplicação de Ponto de Venda para o endereço IP do emulador na porta 9100.

## 📝 License / Licença

This project is licensed under the MIT License. See the `LICENSE` file for details.
