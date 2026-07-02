# SSCodeIDE

An ultra-lightweight, terminal-based Integrated Development Environment (IDE) built from scratch using pure C++98 and POSIX standard APIs. Featuring manual memory management, explicit logical operator mappings (`and`, `or`, `not`), strict UTF-8 text synchronization, single-click efficiency philosophies, and an embedded local asynchronous Artificial Intelligence command bridge.

---

## 1. Project Philosophy & Design Goals

SSCodeIDE is engineered to eliminate development overhead by consolidating file system navigation, text editing, and automated predictive intelligence into a single-threaded terminal layer. 

- **Single-Click Efficiency**: Streamlined modal design allowing instantaneous transitions between code writing, system control, and prompt workflows.
- **Orthodox Structural Architecture**: Completely built under strict adherence to the ISO/IEC 14882:1998 (C++98) standard, ensuring compatibility with legacy compiler toolchains, zero standard library overhead, and deterministic manual execution pipelines.
- **Visual Consolidation**: Maximising character density through raw ANSI escape sequence rendering pipelines, preventing panel overlap or complex multiplexer layers.

---

## 2. Technical Architecture & Constraints

The codebase implements an internal Model-View-Controller (MVC) pattern adapted for POSIX pseudo-terminal execution:

```
										  +--------------------------------+
										  |      TerminalManager           |
										  |  (POSIX raw mode / ReadKey)    |
										  +---------------+----------------+
														  |
														  | Keypress Event
														  v
+------------------+      +-------+--------+      +----------------+
|    FileBuffer    | <----+  EditorContext | <----+ RendererEngine |
| (Data Structure) |      | (Engine State) |      | (ANSI 24-bit)  |
+------------------+      +-------+--------+      +----------------+
|
| Pipe Execution
v
+---------------+----------------+
|        ss_ai_bridge.py         |
|     (Local Ollama Pipeline)    |
+--------------------------------+

```

### Language Compliance & Dialect Rules
- **Standard**: C++98 (`-std=c++98`).
- **Strict Quality Constraints**: Compiled with `-Wall -Wextra -Werror` to assure absolute programmatic memory safety and clean compilation.
- **Readability Paradigm**: Explicit usage of literal textual operators (`not`, `and`, `or`) configured via native compiler mappings or `<iso646.h>` bridges to prevent symbolic syntax fragmentation.
- **Memory Optimization**: Minimal vector allocation structures containing flat explicit UTF-8 boundaries to track multi-byte indexing safely without dynamic allocation metadata bloating.

---

## 3. Detailed Component Decomposition

### 3.1. Editor Core (`EditorContext`)
Manages global lifecycle state machines, active buffers, active file indexes, and interactive contextual variables:
- **`FileBuffer` Struct**: Houses multi-line data stores (`t_vector`), explicit tracking arrays for granular character position arrays (`cursor_x`, `cursor_y`), scroll offset controls (`row_offset`), state mutation flags (`is_dirty`), and deep snapshots of nested layout matrices forming an explicit internal Undo Stack (`undo_stack`).
- **`OpenFileBuffer`**: Sanitizes string input paths, filters target operating prefixes (`ICON_FILE`, `ICON_FOLDER`), scans operational files to avoid duplicated allocation records, and stream-loads contents line-by-line via `std::ifstream`.

### 3.2. Subsystem Interface (`TerminalManager`)
Responsible for isolating the active Linux Virtual Console profile and reconfiguring standard output streams:
- Modifies `termios` flags, explicitly shifting `tcgetattr` attributes to deactivate standard echo mechanisms (`ECHO`), line canonical buffers (`ICANON`), structural signaller signals (`ISIG`), and software flow mechanisms (`IXON`).
- Manages sequential multi-byte input sequences via byte escape arrays to reconstruct raw terminal properties (`ARROW_UP`, `ARROW_DOWN`, `HOME_KEY`, `END_KEY`, `DEL_KEY`, mouse-scroll definitions).

### 3.3. Layout Assembly (`RendererEngine`)
An autonomous 24-bit RGB ANSI escape renderer optimized to paint state components sequentially into standard output stream regions:
- **`WrapText`**: A robust layout calculation method that parses raw text structures, scans string width metrics under arbitrary terminal columns (`screen_cols`), and generates safe vector wraps.
- **`RefreshScreen`**: Employs structural double-buffering or dynamic character cursor positional jumps (`\x1b[H`, `\x1b[K`) to update context interfaces instantly, mitigating redraw visual tearing.

### 3.4. Local Predictive Bridge (`ss_ai_bridge.py`)
Provides an asynchronous processing system wrapped via native OS pipeline descriptors (`popen` / `pclose`):
- Accepts system prompt structures, processes safe runtime serialization to avoid token string splitting, and transfers instructions directly into the local intelligence server engine (Ollama pipeline).

---

## 4. Modal Operational Layout

SSCodeIDE handles runtime orchestration using three dedicated system modes (`e_editorMode`):

### 1. `MODE_EDITOR`
Standard operations for text modification, editing, and code production.
- **Visual Design**: The screen presents full structural text line records using full layout bounds.
- **Keybindings**:
  - `Ctrl + H`: Cycle back through opened active buffers.
  - `Ctrl + L`: Cycle forward through opened active buffers.
  - `Ctrl + C`: Copy the exact active text row into the global clipboard array.
  - `Ctrl + V`: Inject lines from the internal clipboard stack instantly below the cursor coordinate.
  - `Ctrl + Z`: Pop the top element of the active buffer's undo history array and restore state.
  - `/`: Immediately transition control into the AI command prompt.

### 2. `MODE_AI_PROMPT`
An organic interface to query built-in workflows or pipe text transformations.
- **Visual Design**: Opens an highlighted predictive banner across the bottom layout area (`\x1b[48;2;235;160;0m`), locking keyboard attention entirely into the prompt string stack.
- **Operational Interface**: Supports complete backspace rewriting structures, native multi-byte character insertion support, and absolute cancellation via `Esc`.

### 3. `MODE_FILE_MANAGER`
An integrated file browser that occupies the screen area to maximize efficiency.
- **Visual Design**: Replaces the primary editor buffer lines with a structured layout tracking local assets, automatically listing directories with an explicit `[FOLDER]` glyph and files with a `[FILE]` glyph.
- **Navigation Controls**:
  - `j` or `ARROW_DOWN`: Move selection down.
  - `k` or `ARROW_UP`: Move selection up.
  - `Enter`: Parse the line under the selection cursor, open the file through `OpenFileBuffer`, and switch back to `MODE_EDITOR`.
  - `Esc` or `q`: Exit file management mode immediately without swapping target buffers.

---

## 5. Shell Command Architecture

While in `MODE_AI_PROMPT`, any input string starting with a forward-slash (`/`) bypasses the local intelligence engine and executes internal editor macro commands directly:

| Command | Arguments | Structural Functional Description |
| :--- | :--- | :--- |
| `/editor` | *None* | Force-swaps the core terminal focus back into text production layout. |
| `/clear` | *None* | Flushes the active context dictionary payload on the AI server and closes the response panel view. |
| `/files` | *None* | Triggers a local path directory read (`opendir`) and injects the asset list into the buffer stack. |
| `/save` | *None* | Validates buffer status flags and writes active text elements into the current output track. |
| `/open` | `<filepath>` | Direct programmatic path loader that reads data structures or instantiates new buffers. |
| `/quit` | *None* | Signals lifecycle shutdown routines, cleanly deallocates vectors, and disables raw terminal profiles. |
| `/exit` | *None* | Identical behavior mapping to the execution of the standard `/quit` directive. |

---

## 6. Compilation & Dependency Requirements

The build process relies strictly on native compilation frameworks available on Unix systems.

### Dependencies
- **Operating System**: Linux / POSIX Compliant Environment.
- **Compiler**: GCC or Clang providing standard compliance for ISO C++98.
- **Environment**: Access to a standard Python 3.x interpreter with the Ollama pipeline framework active (for operational AI bridge processing).

### Directory Tree Layout

```

.
├── include/
│   └── sscode.hpp
├── src/
│   ├── EditorContext.cpp
│   ├── EditorContextUtils.cpp
│   ├── RendererEngine.cpp
│   ├── TerminalManager.cpp
│   └── main.cpp
├── ss_ai_bridge.py
└── Makefile

```

### Build Specifications
Compiling the environment requires executing standard GNU Makefile tasks:

```bash
# Clean previous object files and compile the production binary
make re

# Perform precise component-by-component step compilation
make

# Clean transitional compilation object traces
make clean

# Execute the binary directly with multi-buffer file streams loaded
./SSCodeIDE src/main.cpp include/sscode.hpp

```

---

## 7. Error Mitigation & Verification Systems

The structural codebase includes resilient error tracking strategies:

* **I/O File Protections**: Avoids crashes when accessing restricted files by gracefully falling back to a clean single empty buffer state.
* **Bounds Checking**: Restores cursors gracefully via clamp checks (`static_cast<int>(lines.size())`) when unexpected frame size shifts happen.
* **UTF-8 Multi-byte Safe Slicing**: Validates lead bits via bitmask logic (`0xC0`, `0x80`) to ensure multi-byte sequence changes always treat a character as a single unit, preventing screen fragmentation.
