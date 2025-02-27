# BitSound

BitSound é um projeto que utiliza o microcontrolador RP 2040 do Raspberry Pi Pico para criar um sistema de visualização de áudio com LEDs RGB, uma matriz de LEDs WS2812 e um display OLED SSD1306. O projeto também inclui funcionalidades de alarme e controle de volume usando um joystick.

## Estrutura do Projeto

```
.
├── .gitignore
├── .vscode/
│   ├── c_cpp_properties.json
│   ├── cmake-kits.json
│   ├── extensions.json
│   ├── launch.json
│   ├── settings.json
│   └── tasks.json
├── CMakeLists.txt
├── generated/
│   ├── ws2812.pio.h
│   ├── ws2812.py
├── include/
│   ├── button.h
│   ├── buzzer.h
│   ├── const.h
│   ├── led.h
│   ├── ssd1306_font.h
│   ├── ssd1306_i2c.h
│   └── ws2812.h
├── main.c
└── pico_sdk_import.cmake
```

## Funcionalidades

- **Visualização de Áudio**: O projeto possui diferentes modos de visualização de áudio, incluindo forma de onda, espectro de frequência, medidor VU e radar.
- **Controle de Volume**: O volume do som pode ser ajustado usando o joystick.
- **Alarme**: O sistema pode ser configurado para disparar um alarme baseado no nível de áudio capturado pelo microfone.
- **Controle de LEDs RGB**: Os LEDs RGB são controlados com base no nível de áudio.
- **Matriz de LEDs WS2812**: A matriz de LEDs WS2812 é atualizada progressivamente com base no nível de áudio.

## Configuração do Hardware

- **Microcontrolador**: Raspberry Pi Pico ou Raspberry Pi Pico W
- **Display OLED**: SSD1306
- **Matriz de LEDs**: WS2812
- **Joystick**
- **Microfone**
- **Buzzers**
- **Botões**

## Dependências

- [Pico SDK](https://github.com/raspberrypi/pico-sdk)

## Compilação e Execução

1. Clone o repositório:
    ```sh
    git clone https://github.com/wilssola/embarcatech-projeto-final.git
    cd embarcatech-projeto-final
    ```

2. Inicialize e atualize os submódulos:
    ```sh
    git submodule update --init
    ```

3. Configure o ambiente de compilação:
    ```sh
    mkdir build
    cd build
    cmake ..
    ```

4. Compile o projeto:
    ```sh
    make
    ```

5. Carregue o firmware no Raspberry Pi Pico.

## Uso

- **Menu**: Use os botões para navegar no menu e selecionar modos de visualização ou armar o alarme.
- **Visualização**: O modo de visualização exibe diferentes formas de visualização de áudio no display OLED e na matriz de LEDs.
- **Alarme**: O alarme é disparado quando o nível de áudio excede um limite especificado. Use o joystick para desativar o alarme.

## Contribuição

Contribuições são bem-vindas! Sinta-se à vontade para abrir issues e pull requests.

## Licença

Este projeto está licenciado sob a licença MIT. Veja o arquivo LICENSE para mais detalhes.

---

Desenvolvido por [Wilson Oliveira Lima](https://github.com/wilssola).