# ChapiX - Divine Sandbox

## 🌟 Visão Geral do Projeto
ChapiX é um simulador de "God-Game" (Jogo de Deus) em estilo Sandbox construído inteiramente do zero. O jogador atua como uma divindade onipotente com o poder de moldar mundos limitados proceduralmente, interagir com o terreno, e observar o surgimento, evolução e até a destruição de múltiplas civilizações autônomas.

O foco central deste projeto não é apenas ser um "Pintor de Mapas", mas um **Ecossistema Vivo**: os cidadãos que habitam o mundo possuem Inteligência Artificial (IA) fundamentada em Máquina de Estados, tomando suas próprias decisões para cortar árvores, construir assentamentos, gerenciar recursos de comida e, no limite da diplomacia, declarar Guerras e se enfrentarem num "Campo de Batalha" unificado até a morte.

## ⚙️ Base Tecnológica (Como foi feito)
O projeto foi desenvolvido em Arquitetura modular buscando **Alta Performance**, escalabilidade e aprendizado e controle de Baixo e Médio Nível, utilizando as seguintes tecnologias e técnicas:

- **Linguagem Principal:** `C++ Moderno` (Foco em Orientação a Objetos, gerenciamento restrito de memória e paralelismo lógico).
- **Motor Gráfico (Engine):** `Raylib` - Uma framework focada em programação pura baseada em C, extremamente leve e célere. Possui acesso direto ao pipeline da GPU, o que permite a renderização fluida de uma malha com dezenas de milhares de tiles a 60 FPS com controles de Câmera 2D matemáticos.
- **Gerenciador de Dados (HD/I/O):** Sistema de serialização binária nativa via `nlohmann/json` (MessagePack/CBOR). Garantimos que os pesadíssimos Saves Games gigantes não virem arquivos de texto (JSON) monstruosos. Foram condensados pra vetores de Bytes Nativos extremamente leves salvos como `.sav` blindados com mecanismos de tratamento de erro (Try-Catch) Anti-Crash.
- **Arquitetura de Software:** Componentização do Projeto em Módulos Claros (ex: `SimulationManager` para gerir impérios, `UIManager` para os painéis Overlay e `WorldRenderer` fechado focado na Pintura Visual). Além do uso de algoritmos sofisticados, como Pesquisa Hash-Maps com tempo de complexidade **$O(1)$** para identificação instantânea das IA's.

## 🚀 Como Compilar e Executar (Guia de Instalação Inicial)

Para colocar o projeto para rodar numa máquina nova após ter baixado o repositório (seja via arquivo `.zip` ou terminal via `git pull / clone`), siga este fluxo prático:

**1. Abrindo a Central de Comando**
Abra o seu terminal do Windows (CMD Vanilla ou PowerShell) diretamente na Raiz (`C: \ ... \ProjetoGame`) onde este arquivo README se encontra hospedado.

**2. Compilando o Código-Fonte (Gerando o Executável)**
Para construir o projeto do zero usando a máxima potência do seu processador, nós utilizamos a arquitetura modular de Build do **CMake**. Isso permite compilarmos vários arquivos C++ em dezenas de Threads (Multi-Core) simultaneamente, poupando minutos na espera.

Basta rodar o nosso script facilitador no prompt:
```bat
build_cmake.bat
```
*(Ele irá invocar o MinGW para escanear a estrutura `CMakeLists.txt`, gerando as pastas de cache locais em `build/` e exportando o arquivo Final limpo pronto e otimizado direto na pasta `bin/`.)*

> **Nota Técnica Extra:** 
> - Caso algo trave num cache futuramente, você pode usar `build_cmake.bat clean` para esvaziar a pasta e recomeçar!
> - O script esconde nativamente as aborrecidas Janelas Pretas (Prompt do Windows) usando a flag Win32 `-mwindows` e empacota e costura o *"Resource File"* (A Logo e Identidade Visual `.ico`) nas veias binárias do Executável `ChapiX.exe`.

**3. Jogando a Obra de Arte**
Deu "Success" no compilador? Ótimo, rode o binário recém-gerado com este comando final no terminal:
```bat
bin\ChapiX.exe
```
*(Ou, se preferir caminhos orgânicos, saia do terminal, abra a pasta "bin" e dê um Duplo-Clique tradicional no arquivo `.exe`)*.

---
## 🎮 Super-Pautas para a sua Apresentação (Dicas do que Mostrar)
No momento em que você compartilhar a tela com a audiência, não se esqueça de exibir o peso do seu trabalho na prática:
1. **Pinte o Mundo:** Brinque com a Toolbelt infernal demonstrando a manipulação direta usando as ferramentas divinas para elevar e afundar terreno alterando os blocos da água/terra através da Câmera travada no Bounding Box.
2. **Ciclo de Vida:** Crie duas cidades (Cultura vs Cultura). Selecione o modo de visualização Humana e dê clique duplo num aldeão para mostrar as caixas da UI seguindo-o na Caçada / Trabalho dele pela tela.
3. **Morte e Pólvora (Guerra Tática):** Vá no poder Divino e mude o cursor da amizade pra Fogo cruzado (Relação para `-100`). Selecione as duas capitais na frente de todos e veja a IA alistando e convertendo civil para Soldado - logo após rastreando o Campo Matemático Centrado no mapa e todos se socando em Sangue (Vermelho/Hostile) até sobrar só um Reino Ativo!
