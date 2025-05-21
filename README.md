# IoT - Desenvolvimento de um webserver para automação residencial

Este projeto implementa um sistema de automação residencial utilizando o Raspberry Pi Pico W, com controle de iluminação, cortina e leitura de temperatura através de uma interface web acessível via Wi-Fi.

## Funcionalidades

* Controle de iluminação por ambiente (quarto, sala, área externa) com seleção de cor.
* Simulação de controle de cortina utilizando um buzzer.
* Leitura e exibição da temperatura interna do microcontrolador.
* Interface web simples para interação com o sistema.
* Comunicação via servidor TCP integrado (sem necessidade de bibliotecas externas de servidor web).

## Interface Web

A interface é acessada via o IP local do dispositivo, exibido após a conexão com o Wi-Fi.

### Ações disponíveis na interface:

* Seleção de cor e acionamento da iluminação para:

  * Quarto
  * Sala
  * Área externa
* Acionamento da cortina (simulada com buzzer)
* Exibição da temperatura interna do RP2040

## Instruções de Uso

1. Configure o ambiente de desenvolvimento com o Pico SDK.
2. Edite o código para inserir seu SSID e senha do Wi-Fi (`WIFI_SSID` e `WIFI_PASSWORD`).
3. Compile o projeto com `cmake` e grave o binário no Pico W.
4. Monitore o terminal serial para verificar a conexão e o IP atribuído.
5. Acesse o IP informado no navegador para interagir com a interface.

## Demonstração em Vídeo

*Insira aqui o link do vídeo demonstrativo do projeto.*

Exemplo:

```
https:
```
