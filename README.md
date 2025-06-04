# Projeto de Controle de Iluminação Inteligente - Raspberry Pi Pico W - MQTT

## 📌 Sumário  
- [📹 Demonstração](#-demonstração)  
- [🎯 Objetivo](#-objetivo)  
- [🛠️ Funcionalidades Obrigatórias](#️-funcionalidades-obrigatórias)  
- [✨ Funcionalidades Adicionais](#-funcionalidades-adicionais)  
- [📦 Componentes Utilizados](#-componentes-utilizados)  
- [⚙️ Compilação e Gravação](#️-compilação-e-gravação)  
- [📂 Estrutura do Código](#-estrutura-do-código)  
- [👨‍💻 Autor](#-autor)  

## 📹 Demonstração  
[clique aqui para acessar o vídeo](https://youtu.be/NvOw4scISNc)
 
Conteúdo do vídeo:  
- Apresentação do projeto  
- Explicação sobre o controle de iluminação via interface web  
- Demonstração ao vivo do controle dos quadrantes da matriz de LEDs  


## 🎯 Objetivo  
Desenvolver um sistema embarcado para controle remoto de iluminação residencial utilizando o Raspberry Pi Pico W e uma matriz de LEDs WS2818B, comunicando-se através do protocolo MQTT. O sistema permite o controle individual de zonas de iluminação (quadrantes) e a visualização do seu estado e consumo de potência. 

## 🛠️ Funcionalidades Obrigatórias 
✅ Controle via MQTT: A placa atua como cliente MQTT, recebendo comandos para alternar os estados de 4 quadrantes da matriz de LEDs.
✅ Publicação de Estados: A placa publica o estado atual ("On"/"Off") de cada quadrante e a potência total consumida via MQTT.
✅ Conexão Wi-Fi: O Pico W conecta-se a uma rede Wi-Fi para estabelecer comunicação com o broker MQTT.
✅ Indicador de Status Online: Utiliza o recurso Last Will and Testament (LWT) do MQTT para indicar seu status online/offline.
✅ Feedback Visual: As mudanças nos estados dos quadrantes são refletidas instantaneamente na matriz de LEDs.
✅ Exibição de Potência: Um display OLED exibe a potência total calculada dos quadrantes ativos.

## 📦 Componentes Utilizados  
- Microcontrolador: Raspberry Pi Pico W  
- Matriz de LEDs: WS2818B 5x5  
- Conectividade: Wi-Fi via CYW43
- Broker MQTT: Mosquitto (executado em um Celular Android via Termux)
- Cliente MQTT para Visualização: IoT MQTT Panel (software para gestão e visualização em Android) 
- Ambiente de desenvolvimento: C/C++ com SDK do Pico e LWIP  

## ⚙️ Compilação e Gravação  
```bash
git clone https://github.com/Ronaldo8617/SmartLights
cd SmartLights
mkdir build
cd build
cmake ..
make
```

**Gravação:**  
Pelo ambiente do VScode compile e execute na placa de desnvovimento BitDogLab
Ou
Conecte o RP2040 no modo BOOTSEL e copie o `.uf2` gerado na pasta `build` para a unidade montada.

## 📂 Estrutura do Código  

```plaintext
SmartLights/  
├── lib/  
│   ├── font.h               # Fonte de caracteres do display  
│   ├── ssd1306.c, h         # Biblioteca do display OLED  
│   ├── buttons.c, h         # Configuração e debounce de botões  
│   ├── rgb.c, h             # Controle do LED RGB via PWM  
│   ├── display_init.c, h    # Inicialização e desenho no display  
│   ├── matrixws.c, h        # Controle da matriz WS2812B  
│   ├── ws2812b.pio.h        # Programa PIO da matriz WS2812B  
│   ├── buzzer.c, h          # Inicialização e controle do buzzer   
├── CMakeLists.txt           # Arquivo de configuração da build  
├── SmartLights.c            # Código principal (main)
├── mbedtls_config.h            
├── Lwipopts.h               
├── README.md                # Documento de descrição do projeto  
```

## 👨‍💻 Autor  
**Nome:** Ronaldo César Santos Rocha  
**GitHub:** [Ronaldo8617]
