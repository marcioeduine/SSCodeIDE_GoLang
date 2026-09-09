# SSCode IDE

**SSCode IDE** is a responsive, terminal-based Integrated Development Environment (TUI) built in 100% native Go using [Charm Bubbletea](https://github.com/charmbracelet/bubbletea), [Lipgloss](https://github.com/charmbracelet/lipgloss), and [Bubbles](https://github.com/charmbracelet/bubbles), featuring local AI LLM integration via [Ollama](https://ollama.ai).

---

## Key Features

- **100% Native Go & Bubbletea**: Built on the Elm architecture for terminal UIs.
- **Real-Time Editor Syntax Highlighting**: Real-time keyword, string, comment, type, and preprocessor color highlighting for C, C++, Go, Python, JavaScript, TypeScript, Shell, HTML, CSS, etc.
- **Common IDE Editing Shortcuts**: Full support for `Ctrl+C` (copy line/selection), `Ctrl+V` (paste), `Ctrl+X` (cut), `Ctrl+D` (duplicate line), `Ctrl+Backspace`/`Alt+Backspace` (delete word left), `Ctrl+Delete`/`Alt+Delete` (delete word right), `Ctrl+Left`/`Alt+Left` (word left), `Ctrl+Right`/`Alt+Right` (word right).
- **Threaded Syntax Autocomplete**: Non-blocking asynchronous command palette and chat suggestions powered by background goroutines (`tea.Cmd`).
- **Floating SSHeader Git User Modal**: Interactive floating dialog window (`F5`, `Ctrl+G`, `:user`, `:config`, `:ssheader`) to configure `git config user.name` and `git config user.email` directly inside the IDE.
- **Interactive Workspace Directory Browser**: Type `:workspace`, `:cd`, or `:folder` in Command Palette (`Ctrl+P`) to open an interactive modal directory picker.
- **Direct Editor Tab Switching**: Switch tabs instantly using `Alt+1..9`, `Ctrl+Tab`, `Ctrl+Shift+Tab`, or `Ctrl+N`.
- **F4 Automated File Header Generator**: Instantly insert or update standard 42/SSHeader file headers. Formatted to strict 80-character line bounds.
- **VS Code Style Git Source Control Panel**: Press `Ctrl+4` or `Ctrl+Shift+G` to view repository status (` branch`), stage/unstage files (`Space`), write commit messages (`Ctrl+Enter`), push (`P`), and pull (`U`).
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
