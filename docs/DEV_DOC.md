# SSCode IDE — Developer Documentation

SSCode IDE is an open-source, lightweight, responsive terminal integrated development environment written in 100% native Go using the **Charm Bubbletea** TUI framework (`github.com/charmbracelet/bubbletea`), **Bubbles** widgets (`textinput`), and **Lipgloss** for layout and styling.

---

## Architectural Design

The application follows the Model-View-Update (Elm Architecture) pattern:

```
                  +-----------------------+
                  |  tea.NewProgram()     |
                  +-----------+-----------+
                              |
                              v
                  +-----------------------+
                  |    mainModel.Init()   |
                  +-----------+-----------+
                              |
                              v
             +---------------------------------+
             |        Event Loop               |
             |  (Keyboard, Mouse, WindowSize)  |
             +----------------+----------------+
                              |
                              v
                  +-----------------------+
                  |  mainModel.Update()   |
                  +-----------+-----------+
                              |
                              v
                  +-----------------------+
                  |   mainModel.View()    |
                  +-----------------------+
```

---

## Source Directory Overview (`src/`)

All source files are located inside the `src/` directory to maintain a clean project root:

| File | Responsibility |
| :--- | :--- |
| `src/main.go` | Entry point. Initialises `tea.Program` with alternate screen buffer and mouse motion tracking. |
| `src/model.go` | State structures (`mainModel`, `EditorTab`, `FileItem`, `ChatMessage`, `GitStatusItem`, `WsModalItem`), event loop handling (`Update`), Git Source Control logic, F4 header generator, IDE text manipulation shortcuts (`Ctrl+C/V/X/D`, word movement/deletion), background thread autocomplete (`fetchAutocompletionsCmd`), and layout assembly (`View`). |
| `src/ui.go` | Style tokens (Catppuccin Mocha theme), syntax highlighter lexer (`highlightCodeLine`), panel box renderer (`renderPanel`), tab bar renderer (`renderTabBar`), autocomplete popup renderer (`renderSuggestPopup`), workspace modal renderer (`renderWorkspaceModal`), and Git user config modal renderer (`renderGitUserModal`). |
| `src/ai.go` | Asynchronous Ollama HTTP API client integration (`/api/tags`, `/api/generate`, `/api/pull`) with dynamic `OLLAMA_HOST` resolution and async shell execution. |
| `src/icons.go` | File type and directory Nerd Font icon mapping. |

---

## Component Systems

1. **Syntax Highlighting Engine**:
   - `highlightCodeLine()` parses line tokens (keywords, types, strings, comments, numbers, preprocessors) and applies Lipgloss color styles in real-time.

2. **Threaded Autocomplete Engine**:
   - `fetchAutocompletionsCmd()` runs completion queries asynchronously in background goroutines (`tea.Cmd`).
   - Renders floating autocomplete popups beneath Command Palette (`Ctrl+P`) and Chat (`Ctrl+3`) without blocking UI updates.

3. **SSHeader & Git Identity Modal**:
   - `openGitUserModal()` opens a floating configuration window (`F5`, `Ctrl+G`, `:user`, `:config`).
   - Saves settings to `.gitconfig` (`git config user.name` / `user.email`).
   - `generateHeader()` formats line bounds using `padOrTruncateRune()` to ensure exact 80-character header box width.

4. **Git Source Control Subsystem**:
   - Executes `git status --porcelain` and `git branch --show-current` asynchronously.
   - Supports staging (`git add`), unstaging (`git restore --staged`), committing (`git commit`), pushing (`git push`), and pulling (`git pull`).

5. **Interactive Workspace Picker**:
   - `openWorkspaceModal()` opens a interactive directory browser for selecting workspace folders.

---

## Building and Compiling

Execute the build target via `make`:

```bash
make re
```

To run the IDE directly:

```bash
./sscode
```
