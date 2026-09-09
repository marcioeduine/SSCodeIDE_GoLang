# SSCode IDE — User Documentation

SSCode IDE is a terminal-based text editor, Git source control manager, and local AI assistant built using Go, Bubbletea, and Ollama.

---

## Key Features

- **Real-Time Editor Syntax Highlighting**: Automatic keyword, type, string, comment, number, and preprocessor directive highlighting for C, C++, Go, Python, JavaScript, TypeScript, Shell, HTML, CSS, JSON, Markdown, etc.
- **Common IDE Shortcuts & Text Manipulation**: Full support for `Ctrl+C` (copy line/selection), `Ctrl+V` (paste), `Ctrl+X` (cut line/selection), `Ctrl+D` (duplicate line), `Ctrl+Backspace`/`Alt+Backspace` (delete word left), `Ctrl+Delete`/`Alt+Delete` (delete word right), `Ctrl+Left`/`Alt+Left` (jump word left), `Ctrl+Right`/`Alt+Right` (jump word right).
- **Threaded Syntax Autocomplete**: Non-blocking asynchronous command palette and chat autocomplete running in background threads (`tea.Cmd`). Shows floating syntax candidate popups without slowing down TUI rendering.
- **SSHeader Git User Configuration Modal**: Floating dialog window (`F5`, `Ctrl+G`, `:user`, `:config`, `:ssheader`) to configure `git config user.name` and `git config user.email` directly from the IDE. Updates author credentials used by `F4` header generator.
- **Dynamic Git Branch Status**: View the active Git branch (` main`, ` feature/...`) in real-time on the status bar.
- **Automated F4 Header Generator**: Press `F4` to insert or update standard 42/SSHeader file headers. Automatically retrieves author name and email from `git config user.name` and `git config user.email`, and formats lines to fit strictly within the exact character length boundary of the header delimiter.
- **Interactive Workspace Directory Picker**: Type `:workspace`, `:cd`, or `:folder` in the Command Palette (`Ctrl+P`) to open an interactive modal directory browser. List all system directories and navigate seamlessly (`..`, subfolders, `Space`/`Enter` to select workspace).
- **VS Code Style Git Control**: Press `Ctrl+4` or `Ctrl+Shift+G` to open the Git Source Control panel. View changed files, stage/unstage changes, write commit messages, and push/pull to remote GitHub repositories.
- **AI CLI Assistant**: Integrated chat panel connected directly to local Ollama LLMs (`deepseek-coder`, `qwen2.5-coder`, `llama3.2`, etc.).
- **Command Palette**: Press `Ctrl+P` to open a floating command widget for quick navigation, workspace selection, and settings.
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
| `F4` | Insert or update Git-configured author file header |
| `F5` or `Ctrl+G` | Open SSHeader Git User Configuration modal |
| `Ctrl+C` | Copy line or selection to clipboard (in Editor) |
| `Ctrl+X` | Cut line or selection to clipboard (in Editor) |
| `Ctrl+V` | Paste clipboard content (in Editor) |
| `Ctrl+D` | Duplicate active line (in Editor) |
| `Ctrl+Backspace` / `Alt+Backspace` | Delete previous word (in Editor) |
| `Ctrl+Delete` / `Alt+Delete` | Delete next word (in Editor) |
| `Ctrl+Left` / `Alt+Left` | Move cursor word left (in Editor) |
| `Ctrl+Right` / `Alt+Right` | Move cursor word right (in Editor) |
| `Tab` | Cycle focus between Explorer, Editor, Chat, and Git Source Control |
| `Ctrl+P` | Toggle Command Palette |
| `Ctrl+B` | Toggle Explorer sidebar visibility |
| `Ctrl+S` | Save current active file |
| `Ctrl+W` | Close current active editor tab |
| `Ctrl+K` | Open Ollama Model Manager modal |
| `Ctrl+Left` / `Ctrl+Right` | Adjust Explorer width ratio |
| `Alt+Left` / `Alt+Right` | Adjust Chat width ratio |

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

- `:user` or `:config` or `:ssheader` — Open floating SSHeader Git User configuration modal.
- `:workspace` or `:cd` or `:folder` — Open interactive modal directory picker to choose a workspace.
- `:workspace <path>` or `:cd <path>` — Open modal browser starting at specified directory path.
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
