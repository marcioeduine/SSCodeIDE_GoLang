# SSCode IDE

**SSCode IDE** is a responsive, terminal-based Integrated Development Environment (TUI) built in 100% native Go using [Charm Bubbletea](https://github.com/charmbracelet/bubbletea), [Lipgloss](https://github.com/charmbracelet/lipgloss), and [Bubbles](https://github.com/charmbracelet/bubbles), featuring local AI LLM integration via [Ollama](https://ollama.ai).

---

## Key Features

- **100% Native Go & Bubbletea**: Built on the Elm architecture for terminal UIs.
- **Direct Editor Tab Switching**: Switch tabs instantly using `Alt+1..9`, `Ctrl+Tab`, `Ctrl+Shift+Tab`, or `Ctrl+N`.
- **F4 Automated File Header Generator**: Instantly insert or update standard author header blocks customized per file extension (`.go`, `.py`, `.c`, `.cpp`, `.cs`, `.js`, `.ts`, `.sh`, `.html`, etc.).
- **VS Code Style Git Source Control Panel**: Press `Ctrl+4` or `Ctrl+Shift+G` to view repository status (` branch`), stage/unstage files (`Space`), write commit messages (`Ctrl+Enter`), push (`P`), and pull (`U`).
- **Command Palette Workspace Management**: Press `Ctrl+P` and type `:workspace <path>` or `:cd <path>` to open and load entire directory workspaces.
- **Universal Portability**: Zero hardcoded local environment paths; builds seamlessly across Linux, macOS, and WSL.
- **Local Ollama AI Integration**: Connect to local coding models (`deepseek-coder`, `qwen2.5-coder`, `llama3.2`, etc.) via `OLLAMA_HOST` or default localhost.

---

## Directory Structure

```
.
├── docs/          # Technical documentation directory
│   ├── DEV_DOC.md # Developer architecture documentation
│   └── USER_DOC.md# User manual and shortcut guide
├── go.mod         # Go module dependencies
├── go.sum         # Dependency checksums
├── LICENSE        # GNU General Public License v3
├── Makefile       # Universal compilation build file
├── README.md      # Project overview documentation
├── src/           # Application source code
│   ├── ai.go      # Asynchronous Ollama HTTP client and shell executor
│   ├── icons.go   # File type icon mappings (Nerd Fonts)
│   ├── main.go    # Program entry point and Bubbletea launcher
│   ├── model.go   # State model, update logic, and view renderer
│   └── ui.go      # Styling tokens, theme definitions, and box rendering
└── sscode         # Executable launcher script
```

---

## Quick Start

### Build and Execution

To compile and launch SSCode IDE:

```bash
make run
```

Alternatively, compile using the Makefile and execute:

```bash
make
./sscode
```

---

## Configuration & Environment Variables

- `GO`: Override Go compiler binary path (defaults to standard `go` in PATH).
- `OLLAMA_HOST`: Override Ollama API host address (defaults to `http://localhost:11434`).

---

## Documentation

- **[User Documentation](docs/USER_DOC.md)**: Features, shortcuts, and command reference.
- **[Developer Documentation](docs/DEV_DOC.md)**: Architectural design, TUI layout rules, and component specs.

---

## License

Distributed under the terms of the GNU General Public License v3. See [LICENSE](LICENSE) for full details.
