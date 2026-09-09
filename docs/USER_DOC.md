# SSCode IDE — User Documentation

SSCode IDE is a terminal-based text editor, Git source control manager, and local AI assistant built using Go, Bubbletea, and Ollama.

---

## Key Features

- **File Explorer**: Interactive tree navigation with folder expansion and file icon indicators.
- **Multi-Tab Editor**: Edit multiple files simultaneously with line numbers, status indicators, and keyboard navigation.
- **Direct Tab Switching**: Press `Alt+1..9`, `Ctrl+Tab`, `Ctrl+Shift+Tab`, or `Ctrl+N` to jump directly between active editor tabs.
- **Automated F4 Header Generator**: Press `F4` to automatically insert or update standard author file headers matching the file extension (`.go`, `.py`, `.c`, `.cpp`, `.cs`, `.js`, `.ts`, `.sh`, `.html`, etc.).
- **VS Code Style Git Control**: Press `Ctrl+4` or `Ctrl+Shift+G` to open the Git Source Control panel. View changed files, stage/unstage changes, write commit messages, and push/pull to remote GitHub repositories.
- **Workspace Opening**: Open entire folder workspaces via Command Palette (`Ctrl+P` -> `:workspace <path>` or `:cd <path>`).
- **AI CLI Assistant**: Integrated chat panel connected directly to local Ollama LLMs (`deepseek-coder`, `qwen2.5-coder`, `llama3.2`, etc.).
- **Command Palette**: Press `Ctrl+P` to open a floating command widget for quick navigation and settings.
- **Custom Themes**: Instant theme switching (`catppuccin`, `dracula`, `nord`).

---

## Keyboard Shortcuts

| Shortcut | Action |
| :--- | :--- |
| `Ctrl+1` | Focus File Explorer |
| `Ctrl+2` | Focus Editor |
| `Ctrl+3` | Focus AI Chat |
| `Ctrl+4` or `Ctrl+Shift+G` | Focus Git Source Control |
| `Alt+1` .. `Alt+9` | Switch directly to Editor Tab 1..9 |
| `Ctrl+Tab` / `Ctrl+N` | Switch to Next Editor Tab |
| `Ctrl+Shift+Tab` | Switch to Previous Editor Tab |
| `F4` | Insert or update author file header |
| `Tab` | Cycle focus between Explorer, Editor, Chat, and Git Source Control |
| `Ctrl+P` | Toggle Command Palette |
| `Ctrl+B` | Toggle Explorer sidebar visibility |
| `Ctrl+S` | Save current active file |
| `Ctrl+W` | Close current active editor tab |
| `Ctrl+K` | Open Ollama Model Manager modal |
| `Ctrl+Left` / `Ctrl+Right` | Adjust Explorer width ratio |
| `Alt+Left` / `Alt+Right` | Adjust Chat width ratio |
| `Ctrl+C` | Quit SSCode IDE |

---

## Git Source Control Shortcuts (in Git Panel)

| Shortcut | Action |
| :--- | :--- |
| `Space` or `S` | Stage / Unstage selected file |
| `C` or `Ctrl+Enter` | Commit staged changes with commit message |
| `P` | Push commits to remote GitHub repository (`git push`) |
| `U` | Pull updates from remote GitHub repository (`git pull`) |
| `R` | Refresh Git repository status |
| `Up` / `Down` | Navigate Git files list |

---

## Command Palette & Chat Commands

You can execute commands in the Command Palette (`Ctrl+P`) or directly in the Chat input box:

- `:workspace <path>` or `:cd <path>` — Change working directory and load workspace folder.
- `:git` or `:status` — Focus Git Source Control panel.
- `:models` or `/models` — Open the Ollama model manager dialog.
- `:theme <name>` — Change theme (`catppuccin`, `dracula`, `nord`).
- `:open <path>` or `/open <path>` — Open a file in a new tab.
- `:w` or `/save` — Save active file.
- `:q` or `/quit` — Exit application.
- `!<command>` — Run an asynchronous shell command (e.g. `!ls -la` or `!git status`).
- `/clear` — Clear chat history.
- `/help` — Print command help details.

---

## System Requirements

- **Linux / macOS / WSL**
- **Terminal Emulator** with UTF-8 and TrueColour support (e.g., Alacritty, Kitty, WezTerm, iTerm2, GNOME Terminal).
- **Git** CLI installed for Source Control capabilities.
- **Ollama Service** running at `http://localhost:11434` (or `OLLAMA_HOST`) for local AI features.
