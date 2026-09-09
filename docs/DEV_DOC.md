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
| `src/model.go` | State structures (`mainModel`, `EditorTab`, `FileItem`, `ChatMessage`, `GitStatusItem`), event loop handling (`Update`), Git Source Control logic, F4 header generator, keyboard/mouse event dispatching, and layout assembly (`View`). |
| `src/ui.go` | Style tokens (Catppuccin Mocha theme, Git status styles), panel box renderer (`renderPanel`), tab bar renderer (`renderTabBar`), text truncation (`truncateString`), and raw word wrapping (`wrapRawText`). |
| `src/ai.go` | Asynchronous Ollama HTTP API client integration (`/api/tags`, `/api/generate`, `/api/pull`) with dynamic `OLLAMA_HOST` resolution and async shell execution. |
| `src/icons.go` | File type and directory Nerd Font icon mapping. |

---

## Component Systems

1. **Git Source Control Subsystem**:
   - Executes `git status --porcelain` asynchronously to build `gitItems`.
   - Supports staging (`git add`), unstaging (`git restore --staged`), committing (`git commit`), pushing (`git push`), and pulling (`git pull`).

2. **Automated Header Generator (`F4`)**:
   - `generateHeader()` constructs language-specific header comments (`.go`, `.py`, `.c`, `.cpp`, `.cs`, `.js`, `.ts`, `.sh`, `.html`, etc.).
   - `insertOrUpdateHeader()` updates the `Updated:` timestamp if a header already exists, or prepends a new header to `tab.Lines`.

3. **Workspace Management**:
   - `executePaletteCommand()` handles workspace switching (`:workspace <path>`, `:cd <path>`).
   - Changes current working directory with `os.Chdir()`, reloads tree via `loadDirectory()`, and updates Git status.

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
