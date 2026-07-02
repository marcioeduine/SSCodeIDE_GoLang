# SSCodeIDE - Ambiente de Desenvolvimento em Raylib com IA e Terminal

Bem-vindo ao **SSCodeIDE**, um ambiente de desenvolvimento integrado (IDE) minimalista, rápido e moderno, construído em C++ utilizando a biblioteca gráfica **Raylib**. Este projecto destaca-se pela sua performance optimizada, suporte a caracteres Unicode (UTF-8) e um assistente de inteligência artificial (IA) local incorporado.

---

## Características Principais

* **Alta Performance**: Algoritmos optimizados para cálculo de espaçamento e renderização de fontes monospace que eliminam o atraso (lag) de digitação, mantendo 60 fotogramas por segundo (FPS) mesmo com ficheiros extensos.
* **Assistente de IA Local Flutuante (F2)**: Um painel elegante e flutuante que hospeda um modelo de linguagem `Qwen-2.5-Coder` local (através do `llama.cpp` / `llama-completion`), permitindo explicações de código e conversas em tempo real sem expor os teus dados à Internet.
* **Transcrição de Voz (F3)**: Digitação inteligente por voz utilizando o modelo `whisper.cpp` (ggml-tiny), que transcreve a tua fala directamente para a caixa de texto da IA.
* **Síntese de Voz (TTS)**: A IA fala contigo em português de Portugal nativo através do utilitário `spd-say`.
* **Terminal Embutido (Ctrl+T)**: Um multiplexador que divide o ecrã a meio e insere uma sessão real do teu terminal Bash à esquerda do editor.
* **Ajuste Automático de Janelas (Super + Setas)**: Organiza a janela do editor de forma intuitiva arrastando-a com atalhos de teclado nativos.
* **Suporte Completo a UTF-8**: Mapeamento e renderização de todos os caracteres acentuados da língua portuguesa (á, é, í, ó, ú, ç, ã, õ, â, ê, ô e maiúsculas).
* **Gestão de Versão com Git LFS**: O repositório está pré-configurado para rastrear e gerir os modelos grandes binários de IA (`models/*.bin` e `models/*.gguf`) correctamente através de Git Large File Storage (LFS).

---

## Atalhos de Teclado e Navegação

| Atalho | Acção / Descrição |
| :--- | :--- |
| `F1` | Insere ou actualiza o teu cabeçalho personalizado de estilo padrão. |
| `F2` | Abre ou fecha o painel flutuante do Assistente de IA. |
| `F3` | Prime uma vez para iniciar a gravação de voz no microfone; prime novamente para transcrever com Whisper e inserir na caixa de texto. |
| `Ctrl + T` | Abre ou fecha a janela do terminal Bash embutido. |
| `Ctrl + S` | Grava o ficheiro actual no disco. |
| `Ctrl + Shift + S` | Grava o ficheiro actual como (Save As). |
| `Ctrl + O` | Abre um diálogo para carregar um ficheiro do disco. |
| `Ctrl + Z` | Desfazer a última alteração (Undo). |
| `Ctrl + Shift + Z` | Refazer a alteração desfeita (Redo). |
| `Ctrl + A` | Selecciona todo o texto presente no ecrã. |
| `Ctrl + C` | Copia a selecção para a área de transferência. |
| `Ctrl + X` | Corta a selecção de texto. |
| `Ctrl + V` | Cola o conteúdo da área de transferência. |
| `Escape` | Cancela e fecha a janela popup de sugestões de autocompletar. |
| `TAB` | Insere uma tabulação ou avança a selecção de linhas à direita. |
| `Shift + TAB` | Recua a indentação da selecção de linhas à esquerda. |

### Atalhos de Ajuste de Janela (Super / Windows + Setas)

Podes gerir o posicionamento do IDE no teu ecrã utilizando a tecla **Super** (tecla Windows) combinada com as setas:

* **`Super + Seta Esquerda`**: Encaixa a janela do IDE na metade esquerda do teu monitor.
* **`Super + Seta Direita`**: Encaixa a janela do IDE na metade direita do teu monitor.
* **`Super + Seta Cima`**: Maximiza a janela do IDE para ocupar o ecrã completo.
* **`Super + Seta Baixo`**: Repõe o tamanho normal anterior da janela ou minimiza-a se já estiver no tamanho normal.

---

## Instalação e Compilação

### Requisitos Prévios

Certifica-te de que possuis as ferramentas de compilação instaladas, bem como a biblioteca gráfica Raylib e os utilitários de áudio do PulseAudio e de síntese de voz:

```bash
sudo apt update
sudo apt install build-essential cmake libasound2-dev libx11-dev libxrandr-dev libxi-dev libxinerama-dev libxcursor-dev libgl1-mesa-dev libglu1-mesa-dev speech-dispatcher pulseaudio-utils
```

### Compilar o IDE

Para compilar todo o projecto a partir do código fonte:

```bash
make re
```

### Executar

Para iniciar o IDE passando um ficheiro a editar por argumento:

```bash
./SSCodeIDE src/main.cpp
```

---

## Como Funciona a Inteligência Artificial

O assistente local utiliza duas componentes instaladas na directoria `third_party/`:

1. **`llama.cpp` / `llama-completion`**: Executa o modelo quantizado `Qwen-2.5-Coder` de 0.5 mil milhões de parâmetros. O modelo é rápido, optimizado para execução no CPU com instruções AVX2 e responde em português correcto na antiga grafia portuguesa.
2. **`whisper.cpp`**: Transcreve o ficheiro de áudio `/tmp/voice.wav` gravado pelo comando `parecord` quando utilizas o atalho `F3`.

A síntese de voz (TTS) lê a resposta gerada de forma não bloqueante usando o utilitário `/usr/bin/spd-say` configurado para português de Portugal (`-l pt`).