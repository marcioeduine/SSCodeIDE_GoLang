package main

import (
	"fmt"
	"os"
	"os/exec"
	"path/filepath"
	"strconv"
	"strings"
	"time"

	"github.com/charmbracelet/bubbles/textinput"
	tea "github.com/charmbracelet/bubbletea"
	"github.com/charmbracelet/lipgloss"
)

type Focus int

const (
	FocusExplorer Focus = iota
	FocusEditor
	FocusChat
	FocusSourceControl
)

type FileItem struct {
	Path     string
	Name     string
	IsDir    bool
	Expanded bool
	Depth    int
	Children []*FileItem
}

type EditorTab struct {
	Path       string
	Name       string
	Lines      []string
	CursorLine int
	CursorCol  int
	ScrollTop  int
	IsModified bool
}

type ChatMessage struct {
	Sender string
	Text   string
}

type GitStatusItem struct {
	Path   string
	Status string
	Staged bool
}

type mainModel struct {
	width  int
	height int
	focus  Focus

	sidebarOpen bool
	expRatio    float64
	chatRatio   float64

	isDraggingSplit1 bool
	isDraggingSplit2 bool

	rootDir      *FileItem
	flatItems    []*FileItem
	explorerSel  int
	expScrollTop int

	tabs      []EditorTab
	activeTab int

	chatHistory  []ChatMessage
	chatInput    textinput.Model
	aiPending    bool
	currentModel string
	chatScroll   int

	showModal    bool
	modelsList   []ModelInfo
	modalSel     int
	modalLoading bool
	modalStatus  string

	showCmdPalette  bool
	cmdPaletteInput textinput.Model

	// Source Control (Git)
	gitItems       []GitStatusItem
	gitBranch      string
	gitCommitInput textinput.Model
	gitSel         int
	gitScrollTop   int

	statusMsg string
}

func initialModel() mainModel {
	ti := textinput.New()
	ti.Placeholder = "Send a message to SSBot (/help)..."
	ti.Focus()
	ti.CharLimit = 500

	cmdInput := textinput.New()
	cmdInput.Prompt = "󰘳 : "
	cmdInput.Placeholder = "workspace ./src | open main.go | models | theme dracula | w | q"
	cmdInput.CharLimit = 150
	cmdInput.Width = 50

	gcInput := textinput.New()
	gcInput.Prompt = "Commit: "
	gcInput.Placeholder = "Message (Press C or Ctrl+Enter to commit)..."
	gcInput.CharLimit = 100
	gcInput.Width = 30

	m := mainModel{
		focus:           FocusExplorer,
		sidebarOpen:     true,
		expRatio:        0.25,
		chatRatio:       0.25,
		chatInput:       ti,
		cmdPaletteInput: cmdInput,
		gitCommitInput:  gcInput,
		chatHistory:     make([]ChatMessage, 0),
		tabs:            make([]EditorTab, 0),
		activeTab:       -1,
		currentModel:    "deepseek-coder:1.3b",
	}

	m.chatHistory = append(m.chatHistory, ChatMessage{
		Sender: "SYSTEM",
		Text:   "SSCode IDE initialised.\nShortcuts: Ctrl+1 (Explorer) | Ctrl+2 (Editor) | Ctrl+3 (Chat) | Ctrl+4 (Git Source Control) | Alt+1..9 / Ctrl+Tab (Tabs) | F4 (Header)",
	})

	m.loadDirectory(".")
	m.refreshGitStatus()

	if _, err := os.Stat("src/main.go"); err == nil {
		m.openFile("src/main.go")
	} else if _, err := os.Stat("README.md"); err == nil {
		m.openFile("README.md")
	} else {
		m.tabs = append(m.tabs, EditorTab{
			Path:  "",
			Name:  "Untitled",
			Lines: []string{"// Welcome to SSCode IDE!", "// Open a file from Explorer, type /help in chat, or press Ctrl+P for Command Palette."},
		})
		m.activeTab = 0
	}

	return m
}

func (m *mainModel) loadDirectory(root string) {
	abs, err := filepath.Abs(root)
	if err != nil {
		abs = root
	}
	m.rootDir = buildFileTree(abs, 0)
	m.flattenTree()
}

func buildFileTree(path string, depth int) *FileItem {
	info, err := os.Stat(path)
	if err != nil {
		return nil
	}

	item := &FileItem{
		Path:  path,
		Name:  filepath.Base(path),
		IsDir: info.IsDir(),
		Depth: depth,
	}

	if info.IsDir() && depth <= 1 {
		entries, err := os.ReadDir(path)
		if err == nil {
			for _, entry := range entries {
				name := entry.Name()
				if name == ".git" || name == "node_modules" || name == "build" || name == "dist" || name == "SSCodeIDE" {
					continue
				}
				childPath := filepath.Join(path, name)
				child := buildFileTree(childPath, depth+1)
				if child != nil {
					item.Children = append(item.Children, child)
				}
			}
		}
		if depth == 0 {
			item.Expanded = true
		}
	}
	return item
}

func (m *mainModel) flattenTree() {
	var list []*FileItem
	var recurse func(item *FileItem)
	recurse = func(item *FileItem) {
		if item == nil {
			return
		}
		list = append(list, item)
		if item.IsDir && item.Expanded {
			for _, child := range item.Children {
				recurse(child)
			}
		}
	}
	if m.rootDir != nil {
		for _, child := range m.rootDir.Children {
			recurse(child)
		}
	}
	m.flatItems = list
	if m.explorerSel >= len(m.flatItems) {
		m.explorerSel = len(m.flatItems) - 1
	}
	if m.explorerSel < 0 {
		m.explorerSel = 0
	}
}

func (m *mainModel) activeTabPtr() *EditorTab {
	if m.activeTab >= 0 && m.activeTab < len(m.tabs) {
		return &m.tabs[m.activeTab]
	}
	return nil
}

func (m *mainModel) refreshGitStatus() {
	out, err := exec.Command("git", "status", "--porcelain").Output()
	if err != nil {
		m.gitItems = nil
		m.gitBranch = ""
		return
	}
	branchOut, _ := exec.Command("git", "branch", "--show-current").Output()
	m.gitBranch = strings.TrimSpace(string(branchOut))
	if m.gitBranch == "" {
		m.gitBranch = "main"
	}

	lines := strings.Split(strings.TrimSpace(string(out)), "\n")
	var items []GitStatusItem
	for _, l := range lines {
		if len(l) < 4 {
			continue
		}
		st := l[:2]
		p := strings.TrimSpace(l[3:])
		staged := (st[0] != ' ' && st[0] != '?')
		items = append(items, GitStatusItem{
			Path:   p,
			Status: strings.TrimSpace(st),
			Staged: staged,
		})
	}
	m.gitItems = items
	if m.gitSel >= len(m.gitItems) {
		m.gitSel = len(m.gitItems) - 1
	}
	if m.gitSel < 0 {
		m.gitSel = 0
	}
}

func generateHeader(filename string) []string {
	ext := strings.ToLower(filepath.Ext(filename))
	base := filepath.Base(filename)
	author := "By: Márcio Eduine (Ser Superior) <marcioeduine@gmail.com>"
	now := time.Now().Format("2006/01/02 15:04:05")

	switch ext {
	case ".go", ".c", ".cpp", ".cc", ".cxx", ".h", ".hpp", ".cs", ".java", ".js", ".ts", ".css", ".php":
		return []string{
			"/* ************************************************************************** */",
			"/*                                                                            */",
			"/*                                                       :::      ::::::::   */",
			fmt.Sprintf("/*   %-50s :+:      :+:    :+:   */", base),
			"/*                                                    +:+---+----+---+        */",
			fmt.Sprintf("/*   %-50s +#+------+------+       */", author),
			"/*                                                +#+------+------+       */",
			fmt.Sprintf("/*   Created: %s by Ser Superior     #+#    #+#             */", now),
			fmt.Sprintf("/*   Updated: %s by Ser Superior    ########   ########.fr   */", now),
			"/*                                                                            */",
			"/* ************************************************************************** */",
			"",
		}
	case ".py", ".sh", ".bash", ".zsh", ".yml", ".yaml", ".toml":
		return []string{
			"# **************************************************************************** #",
			"#                                                                              #",
			"#                                                        :::      ::::::::    #",
			fmt.Sprintf("#   %-50s :+:      :+:    :+:    #", base),
			"#                                                    +:+---+----+---+         #",
			fmt.Sprintf("#   %-50s +#+------+------+        #", author),
			"#                                                +#+------+------+        #",
			fmt.Sprintf("#   Created: %s by Ser Superior     #+#    #+#              #", now),
			fmt.Sprintf("#   Updated: %s by Ser Superior    ########   ########.fr    #", now),
			"#                                                                              #",
			"# **************************************************************************** #",
			"",
		}
	default:
		return []string{
			"<!-- ********************************************************************** -->",
			fmt.Sprintf("<!--   %-68s -->", base),
			fmt.Sprintf("<!--   %-68s -->", author),
			fmt.Sprintf("<!--   Created: %s | Updated: %s   -->", now, now),
			"<!-- ********************************************************************** -->",
			"",
		}
	}
}

func (m *mainModel) insertOrUpdateHeader() {
	tab := m.activeTabPtr()
	if tab == nil {
		m.statusMsg = "No active editor tab to insert header."
		return
	}

	headerLines := generateHeader(tab.Path)
	if len(tab.Lines) >= 11 && (strings.HasPrefix(tab.Lines[0], "/* ****") || strings.HasPrefix(tab.Lines[0], "# ****") || strings.HasPrefix(tab.Lines[0], "<!-- ****")) {
		newLines := append(headerLines, tab.Lines[11:]...)
		tab.Lines = newLines
		m.statusMsg = fmt.Sprintf("Header updated for %s", filepath.Base(tab.Path))
	} else {
		newLines := append(headerLines, tab.Lines...)
		tab.Lines = newLines
		m.statusMsg = fmt.Sprintf("Header inserted into %s", filepath.Base(tab.Path))
	}
	tab.IsModified = true
}

func (m *mainModel) openFile(path string) {
	for i, t := range m.tabs {
		if t.Path == path {
			m.activeTab = i
			m.statusMsg = fmt.Sprintf("Active: %s", filepath.Base(path))
			return
		}
	}

	content, err := os.ReadFile(path)
	if err != nil {
		m.statusMsg = fmt.Sprintf("Failed to open file: %v", err)
		return
	}

	lines := strings.Split(string(content), "\n")
	newTab := EditorTab{
		Path:       path,
		Name:       filepath.Base(path),
		Lines:      lines,
		CursorLine: 0,
		CursorCol:  0,
		ScrollTop:  0,
		IsModified: false,
	}

	m.tabs = append(m.tabs, newTab)
	m.activeTab = len(m.tabs) - 1
	m.statusMsg = fmt.Sprintf("Opened: %s", filepath.Base(path))
}

func (m *mainModel) closeActiveTab() {
	if m.activeTab >= 0 && m.activeTab < len(m.tabs) {
		m.tabs = append(m.tabs[:m.activeTab], m.tabs[m.activeTab+1:]...)
		if m.activeTab >= len(m.tabs) {
			m.activeTab = len(m.tabs) - 1
		}
		if len(m.tabs) == 0 {
			m.activeTab = -1
		}
	}
}

func (m *mainModel) saveActiveFile() {
	tab := m.activeTabPtr()
	if tab == nil || tab.Path == "" {
		m.statusMsg = "No active file to save."
		return
	}
	content := strings.Join(tab.Lines, "\n")
	err := os.WriteFile(tab.Path, []byte(content), 0644)
	if err != nil {
		m.statusMsg = fmt.Sprintf("Failed to save file: %v", err)
	} else {
		tab.IsModified = false
		m.statusMsg = fmt.Sprintf("File saved: %s", filepath.Base(tab.Path))
		m.refreshGitStatus()
	}
}

func (m mainModel) Init() tea.Cmd {
	return textinput.Blink
}

func (m mainModel) Update(msg tea.Msg) (tea.Model, tea.Cmd) {
	var cmds []tea.Cmd

	switch msg := msg.(type) {
	case tea.WindowSizeMsg:
		m.width = msg.Width
		m.height = msg.Height
		chatW := int(float64(msg.Width) * m.chatRatio)
		if chatW < 20 {
			chatW = 20
		}
		tiW := chatW - 6
		if tiW < 10 {
			tiW = 10
		}
		m.chatInput.Width = tiW
		m.gitCommitInput.Width = chatW - 6

	case tea.MouseMsg:
		var expWidth, edtWidth, chatWidth int
		if m.sidebarOpen {
			expWidth = int(float64(m.width) * m.expRatio)
			chatWidth = int(float64(m.width) * m.chatRatio)
			edtWidth = m.width - expWidth - chatWidth
		} else {
			expWidth = 0
			chatWidth = int(float64(m.width) * m.chatRatio)
			edtWidth = m.width - chatWidth
		}
		split1X := expWidth
		split2X := expWidth + edtWidth

		if m.isDraggingSplit1 && m.width > 0 {
			newRatio := float64(msg.X) / float64(m.width)
			if newRatio >= 0.10 && newRatio <= 0.45 {
				m.expRatio = newRatio
			}
		}
		if m.isDraggingSplit2 && m.width > 0 {
			newRatio := float64(m.width-msg.X) / float64(m.width)
			if newRatio >= 0.10 && newRatio <= 0.45 {
				m.chatRatio = newRatio
			}
		}

		switch msg.Action {
		case tea.MouseActionPress:
			if msg.Button == tea.MouseButtonLeft {
				if m.sidebarOpen && (msg.X >= split1X-1 && msg.X <= split1X+1) {
					m.isDraggingSplit1 = true
					return m, nil
				}
				if msg.X >= split2X-1 && msg.X <= split2X+1 {
					m.isDraggingSplit2 = true
					return m, nil
				}

				if m.sidebarOpen && msg.X < expWidth {
					if m.focus != FocusSourceControl {
						m.focus = FocusExplorer
					}
					m.chatInput.Blur()
					m.gitCommitInput.Blur()

					clickedRow := msg.Y - 2 + m.expScrollTop
					if clickedRow >= 0 && clickedRow < len(m.flatItems) {
						m.explorerSel = clickedRow
					}
				} else if msg.X >= expWidth && msg.X < split2X {
					m.focus = FocusEditor
					m.chatInput.Blur()
					m.gitCommitInput.Blur()

					tab := m.activeTabPtr()
					if tab != nil {
						clickedLine := msg.Y - 3 + tab.ScrollTop
						if clickedLine >= 0 && clickedLine < len(tab.Lines) {
							tab.CursorLine = clickedLine
							tab.CursorCol = 0
						}
					}
				} else if msg.X >= split2X {
					m.focus = FocusChat
					m.gitCommitInput.Blur()
					m.chatInput.Focus()
				}
			}

		case tea.MouseActionRelease:
			m.isDraggingSplit1 = false
			m.isDraggingSplit2 = false
		}

		if msg.Button == tea.MouseButtonWheelUp || msg.Button == tea.MouseButtonWheelDown {
			delta := -1
			if msg.Button == tea.MouseButtonWheelDown {
				delta = 1
			}

			if m.sidebarOpen && msg.X < expWidth {
				if m.focus == FocusSourceControl {
					m.gitScrollTop += delta
					if m.gitScrollTop < 0 {
						m.gitScrollTop = 0
					}
				} else {
					m.expScrollTop += delta
					if m.expScrollTop < 0 {
						m.expScrollTop = 0
					}
				}
			} else if msg.X >= expWidth && msg.X < split2X {
				tab := m.activeTabPtr()
				if tab != nil {
					tab.ScrollTop += delta * 3
					if tab.ScrollTop < 0 {
						tab.ScrollTop = 0
					}
				}
			} else if msg.X >= split2X {
				m.chatScroll += delta * 2
				if m.chatScroll < 0 {
					m.chatScroll = 0
				}
			}
			return m, nil
		}

	case tea.KeyMsg:
		key := msg.String()

		if key == "ctrl+c" {
			return m, tea.Quit
		}

		if key == "f4" {
			m.insertOrUpdateHeader()
			return m, nil
		}

		// Direct Editor Tab switching via Alt+1..9 or Ctrl+Tab / Ctrl+PageDown
		if (strings.HasPrefix(key, "alt+") && len(key) == 5) || (m.focus == FocusEditor && strings.HasPrefix(key, "ctrl+") && len(key) == 6) {
			numStr := key[len(key)-1:]
			if idx, err := strconv.Atoi(numStr); err == nil && idx >= 1 && idx <= 9 {
				targetIdx := idx - 1
				if targetIdx < len(m.tabs) {
					m.activeTab = targetIdx
					m.focus = FocusEditor
					m.statusMsg = fmt.Sprintf("Switched to Tab %d: %s", idx, m.tabs[targetIdx].Name)
					return m, nil
				}
			}
		}

		if key == "ctrl+tab" || key == "ctrl+pagedown" || key == "ctrl+n" {
			if len(m.tabs) > 0 {
				m.activeTab = (m.activeTab + 1) % len(m.tabs)
				m.focus = FocusEditor
				m.statusMsg = fmt.Sprintf("Tab: %s", m.tabs[m.activeTab].Name)
				return m, nil
			}
		}

		if key == "ctrl+shift+tab" || key == "ctrl+pageup" {
			if len(m.tabs) > 0 {
				m.activeTab = (m.activeTab - 1 + len(m.tabs)) % len(m.tabs)
				m.focus = FocusEditor
				m.statusMsg = fmt.Sprintf("Tab: %s", m.tabs[m.activeTab].Name)
				return m, nil
			}
		}

		if key == "ctrl+p" {
			m.showCmdPalette = !m.showCmdPalette
			if m.showCmdPalette {
				m.cmdPaletteInput.SetValue("")
				m.cmdPaletteInput.Focus()
			} else {
				m.cmdPaletteInput.Blur()
			}
			return m, nil
		}

		if m.showCmdPalette {
			switch key {
			case "esc":
				m.showCmdPalette = false
				m.cmdPaletteInput.Blur()
				return m, nil
			case "enter":
				cmdStr := strings.TrimSpace(m.cmdPaletteInput.Value())
				m.showCmdPalette = false
				m.cmdPaletteInput.Blur()
				if cmdStr != "" {
					if quit := m.executePaletteCommand(cmdStr, &cmds); quit {
						return m, tea.Quit
					}
				}
				return m, tea.Batch(cmds...)
			}

			var cCmd tea.Cmd
			m.cmdPaletteInput, cCmd = m.cmdPaletteInput.Update(msg)
			return m, cCmd
		}

		switch key {
		case "ctrl+b":
			m.sidebarOpen = !m.sidebarOpen
			return m, nil
		case "ctrl+w":
			m.closeActiveTab()
			return m, nil
		case "ctrl+s":
			m.saveActiveFile()
			return m, nil
		case "ctrl+left":
			if m.expRatio > 0.10 {
				m.expRatio -= 0.03
			}
			return m, nil
		case "ctrl+right":
			if m.expRatio < 0.45 {
				m.expRatio += 0.03
			}
			return m, nil
		case "alt+left":
			if m.chatRatio < 0.45 {
				m.chatRatio += 0.03
			}
			return m, nil
		case "alt+right":
			if m.chatRatio > 0.10 {
				m.chatRatio -= 0.03
			}
			return m, nil
		case "ctrl+1":
			m.focus = FocusExplorer
			m.sidebarOpen = true
			m.chatInput.Blur()
			m.gitCommitInput.Blur()
			return m, nil
		case "ctrl+2":
			m.focus = FocusEditor
			m.chatInput.Blur()
			m.gitCommitInput.Blur()
			return m, nil
		case "ctrl+3":
			m.focus = FocusChat
			m.gitCommitInput.Blur()
			m.chatInput.Focus()
			return m, nil
		case "ctrl+4", "ctrl+shift+g":
			m.focus = FocusSourceControl
			m.sidebarOpen = true
			m.refreshGitStatus()
			m.chatInput.Blur()
			m.gitCommitInput.Focus()
			return m, nil
		case "ctrl+k":
			m.showModal = !m.showModal
			if m.showModal {
				m.modalStatus = "Loading Ollama model list..."
				m.modalLoading = true
				return m, fetchModelsCmd()
			}
			return m, nil
		}

		if m.showModal {
			switch key {
			case "esc", "q":
				m.showModal = false
			case "up", "k":
				if m.modalSel > 0 {
					m.modalSel--
				}
			case "down", "j":
				if m.modalSel < len(m.modelsList)-1 {
					m.modalSel++
				}
			case "enter":
				if len(m.modelsList) > 0 && m.modalSel < len(m.modelsList) {
					selected := m.modelsList[m.modalSel]
					if selected.Installed {
						m.currentModel = selected.Name
						m.showModal = false
						m.chatHistory = append(m.chatHistory, ChatMessage{
							Sender: "SYSTEM",
							Text:   fmt.Sprintf("Active model changed to: %s", selected.Name),
						})
					} else {
						m.modalStatus = fmt.Sprintf("Downloading %s from Ollama...", selected.Name)
						m.modalLoading = true
						return m, pullModelCmd(selected.Name)
					}
				}
			}
			return m, nil
		}

		if key == "tab" {
			if m.sidebarOpen {
				m.focus = (m.focus + 1) % 4
			} else {
				if m.focus == FocusEditor {
					m.focus = FocusChat
					m.chatInput.Focus()
				} else {
					m.focus = FocusEditor
					m.chatInput.Blur()
				}
			}
			return m, nil
		}

		switch m.focus {
		case FocusSourceControl:
			switch key {
			case "up", "k":
				if m.gitSel > 0 {
					m.gitSel--
				}
			case "down", "j":
				if m.gitSel < len(m.gitItems)-1 {
					m.gitSel++
				}
			case "space", "s":
				if m.gitSel >= 0 && m.gitSel < len(m.gitItems) {
					item := m.gitItems[m.gitSel]
					if item.Staged {
						exec.Command("git", "restore", "--staged", item.Path).Run()
						m.statusMsg = fmt.Sprintf("Unstaged: %s", item.Path)
					} else {
						exec.Command("git", "add", item.Path).Run()
						m.statusMsg = fmt.Sprintf("Staged: %s", item.Path)
					}
					m.refreshGitStatus()
				}
			case "c", "ctrl+enter":
				msg := strings.TrimSpace(m.gitCommitInput.Value())
				if msg != "" {
					out, err := exec.Command("git", "commit", "-m", msg).CombinedOutput()
					if err != nil {
						m.statusMsg = fmt.Sprintf("Commit failed: %s", strings.TrimSpace(string(out)))
					} else {
						m.statusMsg = "Git Commit successful!"
						m.gitCommitInput.SetValue("")
					}
					m.refreshGitStatus()
				} else {
					m.statusMsg = "Commit message cannot be empty."
				}
			case "p":
				out, err := exec.Command("git", "push").CombinedOutput()
				if err != nil {
					m.statusMsg = fmt.Sprintf("Push failed: %s", strings.TrimSpace(string(out)))
				} else {
					m.statusMsg = "Git Push successful!"
				}
				m.refreshGitStatus()
			case "u":
				out, err := exec.Command("git", "pull").CombinedOutput()
				if err != nil {
					m.statusMsg = fmt.Sprintf("Pull failed: %s", strings.TrimSpace(string(out)))
				} else {
					m.statusMsg = "Git Pull successful!"
				}
				m.refreshGitStatus()
			case "r":
				m.refreshGitStatus()
				m.statusMsg = "Git status refreshed."
			}
			var gCmd tea.Cmd
			m.gitCommitInput, gCmd = m.gitCommitInput.Update(msg)
			cmds = append(cmds, gCmd)

		case FocusExplorer:
			switch key {
			case "up", "k":
				if m.explorerSel > 0 {
					m.explorerSel--
				}
			case "down", "j":
				if m.explorerSel < len(m.flatItems)-1 {
					m.explorerSel++
				}
			case "enter", "space":
				if m.explorerSel >= 0 && m.explorerSel < len(m.flatItems) {
					item := m.flatItems[m.explorerSel]
					if item.IsDir {
						item.Expanded = !item.Expanded
						if item.Expanded && len(item.Children) == 0 {
							entries, err := os.ReadDir(item.Path)
							if err == nil {
								for _, entry := range entries {
									name := entry.Name()
									if name == ".git" || name == "node_modules" || name == "build" || name == "dist" || name == "SSCodeIDE" {
										continue
									}
									child := buildFileTree(filepath.Join(item.Path, name), item.Depth+1)
									if child != nil {
										item.Children = append(item.Children, child)
									}
								}
							}
						}
						m.flattenTree()
					} else {
						m.openFile(item.Path)
						m.focus = FocusEditor
					}
				}
			case "h":
				if m.explorerSel >= 0 && m.explorerSel < len(m.flatItems) {
					item := m.flatItems[m.explorerSel]
					if item.IsDir && item.Expanded {
						item.Expanded = false
						m.flattenTree()
					}
				}
			case "l":
				if m.explorerSel >= 0 && m.explorerSel < len(m.flatItems) {
					item := m.flatItems[m.explorerSel]
					if item.IsDir && !item.Expanded {
						item.Expanded = true
						m.flattenTree()
					}
				}
			}

		case FocusEditor:
			tab := m.activeTabPtr()
			if tab != nil {
				visibleLines := m.height - 6
				if visibleLines < 3 {
					visibleLines = 3
				}

				switch key {
				case "up":
					if tab.CursorLine > 0 {
						tab.CursorLine--
						if tab.CursorCol > len(tab.Lines[tab.CursorLine]) {
							tab.CursorCol = len(tab.Lines[tab.CursorLine])
						}
					}
				case "down":
					if tab.CursorLine < len(tab.Lines)-1 {
						tab.CursorLine++
						if tab.CursorCol > len(tab.Lines[tab.CursorLine]) {
							tab.CursorCol = len(tab.Lines[tab.CursorLine])
						}
					}
				case "left":
					if tab.CursorCol > 0 {
						tab.CursorCol--
					}
				case "right":
					if tab.CursorLine < len(tab.Lines) && tab.CursorCol < len(tab.Lines[tab.CursorLine]) {
						tab.CursorCol++
					}
				case "enter":
					curLine := tab.Lines[tab.CursorLine]
					leftPart := curLine[:tab.CursorCol]
					rightPart := curLine[tab.CursorCol:]
					tab.Lines[tab.CursorLine] = leftPart

					newLines := make([]string, 0, len(tab.Lines)+1)
					newLines = append(newLines, tab.Lines[:tab.CursorLine+1]...)
					newLines = append(newLines, rightPart)
					newLines = append(newLines, tab.Lines[tab.CursorLine+1:]...)
					tab.Lines = newLines
					tab.CursorLine++
					tab.CursorCol = 0
					tab.IsModified = true

				case "backspace":
					if tab.CursorCol > 0 {
						curLine := tab.Lines[tab.CursorLine]
						tab.Lines[tab.CursorLine] = curLine[:tab.CursorCol-1] + curLine[tab.CursorCol:]
						tab.CursorCol--
						tab.IsModified = true
					} else if tab.CursorLine > 0 {
						prevLen := len(tab.Lines[tab.CursorLine-1])
						tab.Lines[tab.CursorLine-1] += tab.Lines[tab.CursorLine]
						tab.Lines = append(tab.Lines[:tab.CursorLine], tab.Lines[tab.CursorLine+1:]...)
						tab.CursorLine--
						tab.CursorCol = prevLen
						tab.IsModified = true
					}
				default:
					if len(key) == 1 && msg.Runes != nil && len(msg.Runes) > 0 {
						ch := string(msg.Runes[0])
						if tab.CursorLine < len(tab.Lines) {
							curLine := tab.Lines[tab.CursorLine]
							tab.Lines[tab.CursorLine] = curLine[:tab.CursorCol] + ch + curLine[tab.CursorCol:]
							tab.CursorCol++
							tab.IsModified = true
						}
					}
				}

				if tab.CursorLine < tab.ScrollTop {
					tab.ScrollTop = tab.CursorLine
				}
				if tab.CursorLine >= tab.ScrollTop+visibleLines {
					tab.ScrollTop = tab.CursorLine - visibleLines + 1
				}
			}

		case FocusChat:
			switch key {
			case "enter":
				inputVal := strings.TrimSpace(m.chatInput.Value())
				if inputVal != "" {
					m.chatInput.SetValue("")
					if quit := m.handleChatInput(inputVal, &cmds); quit {
						return m, tea.Quit
					}
				}
			}
			var cmd tea.Cmd
			m.chatInput, cmd = m.chatInput.Update(msg)
			cmds = append(cmds, cmd)
		}

	case modelsMsg:
		m.modalLoading = false
		if msg.err != nil {
			m.modalStatus = msg.err.Error()
		} else {
			m.modelsList = msg.models
			m.modalStatus = "Select a model to activate or download:"
		}

	case pullFinishedMsg:
		m.modalLoading = false
		if msg.err != nil {
			m.modalStatus = msg.err.Error()
		} else {
			m.modalStatus = fmt.Sprintf("Model %s installed successfully!", msg.model)
			m.currentModel = msg.model
			return m, fetchModelsCmd()
		}

	case setModelFinishedMsg:
		m.modalLoading = false
		if msg.err != nil {
			m.modalStatus = msg.err.Error()
		} else {
			m.currentModel = msg.model
			m.showModal = false
			m.chatHistory = append(m.chatHistory, ChatMessage{
				Sender: "SYSTEM",
				Text:   fmt.Sprintf("Active model changed to: %s", msg.model),
			})
		}

	case aiResponseMsg:
		m.aiPending = false
		if msg.err != nil {
			m.chatHistory = append(m.chatHistory, ChatMessage{
				Sender: "ERROR",
				Text:   msg.err.Error(),
			})
		} else {
			m.chatHistory = append(m.chatHistory, ChatMessage{
				Sender: "SSBot",
				Text:   msg.response,
			})
		}

	case shellResultMsg:
		m.chatHistory = append(m.chatHistory, ChatMessage{
			Sender: "SYSTEM",
			Text:   fmt.Sprintf("Executed: !%s\n%s", msg.cmd, msg.output),
		})
	}

	return m, tea.Batch(cmds...)
}

func (m *mainModel) executePaletteCommand(cmdStr string, cmds *[]tea.Cmd) bool {
	cmdStr = strings.TrimPrefix(cmdStr, ":")

	if cmdStr == "models" || cmdStr == "SSBotModels" {
		m.showModal = true
		m.modalStatus = "Loading model list..."
		m.modalLoading = true
		*cmds = append(*cmds, fetchModelsCmd())
		return false
	}

	if strings.HasPrefix(cmdStr, "workspace ") || strings.HasPrefix(cmdStr, "cd ") || strings.HasPrefix(cmdStr, "folder ") || strings.HasPrefix(cmdStr, "openfolder ") {
		parts := strings.SplitN(cmdStr, " ", 2)
		if len(parts) == 2 {
			targetDir := strings.TrimSpace(parts[1])
			if err := os.Chdir(targetDir); err == nil {
				m.loadDirectory(targetDir)
				m.refreshGitStatus()
				m.statusMsg = fmt.Sprintf("Workspace changed to: %s", targetDir)
			} else {
				m.statusMsg = fmt.Sprintf("Failed to change workspace: %v", err)
			}
		}
		return false
	}

	if cmdStr == "git" || cmdStr == "status" {
		m.focus = FocusSourceControl
		m.sidebarOpen = true
		m.refreshGitStatus()
		return false
	}

	if strings.HasPrefix(cmdStr, "theme ") {
		tName := strings.TrimSpace(cmdStr[6:])
		m.statusMsg = setTheme(tName)
		return false
	}

	if strings.HasPrefix(cmdStr, "open ") {
		path := strings.TrimSpace(cmdStr[5:])
		m.openFile(path)
		return false
	}

	if cmdStr == "w" || cmdStr == "save" {
		m.saveActiveFile()
		return false
	}

	if cmdStr == "q" || cmdStr == "quit" {
		return true
	}

	if strings.HasPrefix(cmdStr, "pull ") {
		modelName := strings.TrimSpace(cmdStr[5:])
		*cmds = append(*cmds, pullModelCmd(modelName))
		return false
	}

	if strings.HasPrefix(cmdStr, "set_model ") {
		modelName := strings.TrimSpace(cmdStr[10:])
		*cmds = append(*cmds, setModelCmd(modelName))
		return false
	}

	if strings.HasPrefix(cmdStr, "!") {
		shCmd := strings.TrimSpace(cmdStr[1:])
		*cmds = append(*cmds, runShellCmd(shCmd))
		return false
	}

	m.statusMsg = fmt.Sprintf("Unknown command: :%s", cmdStr)
	return false
}

func (m *mainModel) handleChatInput(inputVal string, cmds *[]tea.Cmd) bool {
	m.chatHistory = append(m.chatHistory, ChatMessage{
		Sender: "USER",
		Text:   inputVal,
	})

	if inputVal == ":SSBotModels" || inputVal == "/models" {
		m.showModal = true
		m.modalStatus = "Loading model list..."
		m.modalLoading = true
		*cmds = append(*cmds, fetchModelsCmd())
		return false
	}

	if inputVal == "/help" {
		m.chatHistory = append(m.chatHistory, ChatMessage{
			Sender: "SYSTEM",
			Text: `SSCode IDE Commands & Shortcuts:
Ctrl+P                        - Floating Command Palette (:workspace, :models, :theme, :w, :q)
Drag borders with Mouse       - Adjust panel split widths
Mouse Wheel                   - Scroll Explorer, Editor, and Chat
Ctrl+1, Ctrl+2, Ctrl+3, Ctrl+4- Focus Explorer (1), Editor (2), Chat (3), Git (4)
Alt+1..9 / Ctrl+Tab           - Direct Editor Tab switching
F4                            - Insert/Update Standard Author Header
Ctrl+B                        - Toggle Sidebar
Ctrl+W / Ctrl+S               - Close Tab / Save File
Ctrl+K                        - Open Ollama Models modal`,
		})
		return false
	}

	if inputVal == "/clear" {
		m.chatHistory = make([]ChatMessage, 0)
		return false
	}

	if inputVal == "/save" {
		m.saveActiveFile()
		return false
	}

	if inputVal == "/quit" || inputVal == ":q" {
		return true
	}

	if strings.HasPrefix(inputVal, "/open ") {
		path := strings.TrimSpace(inputVal[6:])
		m.openFile(path)
		return false
	}

	if strings.HasPrefix(inputVal, "/pull ") {
		modelName := strings.TrimSpace(inputVal[6:])
		*cmds = append(*cmds, pullModelCmd(modelName))
		return false
	}

	if strings.HasPrefix(inputVal, "/set_model ") {
		modelName := strings.TrimSpace(inputVal[11:])
		*cmds = append(*cmds, setModelCmd(modelName))
		return false
	}

	if strings.HasPrefix(inputVal, "!") {
		cmdStr := strings.TrimSpace(inputVal[1:])
		*cmds = append(*cmds, runShellCmd(cmdStr))
		return false
	}

	m.aiPending = true
	*cmds = append(*cmds, sendAIChatCmd(inputVal, m.currentModel))
	return false
}

func (m mainModel) View() string {
	if m.width <= 0 || m.height <= 0 {
		return "Loading SSCode IDE..."
	}

	statusHeight := 1
	mainHeight := m.height - statusHeight
	if mainHeight < 4 {
		mainHeight = 4
	}

	var expWidth, edtWidth, chatWidth int
	if m.sidebarOpen {
		expWidth = int(float64(m.width) * m.expRatio)
		chatWidth = int(float64(m.width) * m.chatRatio)
		if expWidth < 15 {
			expWidth = 15
		}
		if chatWidth < 20 {
			chatWidth = 20
		}
		edtWidth = m.width - expWidth - chatWidth
		if edtWidth < 20 {
			edtWidth = 20
			if m.width > 55 {
				chatWidth = m.width - expWidth - edtWidth
				if chatWidth < 20 {
					chatWidth = 20
					expWidth = m.width - edtWidth - chatWidth
				}
			}
		}
	} else {
		expWidth = 0
		chatWidth = int(float64(m.width) * m.chatRatio)
		if chatWidth < 20 {
			chatWidth = 20
		}
		edtWidth = m.width - chatWidth
		if edtWidth < 20 {
			edtWidth = 20
			if m.width > 40 {
				chatWidth = m.width - edtWidth
			}
		}
	}

	if m.sidebarOpen {
		edtWidth = m.width - expWidth - chatWidth
	} else {
		edtWidth = m.width - chatWidth
	}

	var expView string
	if m.sidebarOpen {
		if m.focus == FocusSourceControl {
			// Source Control (Git) View
			var gitSb strings.Builder
			gitSb.WriteString(m.gitCommitInput.View() + "\n\n")
			gitSb.WriteString(lipgloss.NewStyle().Foreground(mdTextMuted).Render("[Space]:Stage/Unstage [c]:Commit [p]:Push [u]:Pull") + "\n\n")

			if len(m.gitItems) == 0 {
				gitSb.WriteString(lipgloss.NewStyle().Foreground(mdGreen).Render("✔ Working tree clean") + "\n")
			} else {
				for i, gItem := range m.gitItems {
					prefix := " "
					if i == m.gitSel {
						prefix = ">"
					}
					tag := "[U]"
					style := gitUntrackedStyle
					if gItem.Staged {
						tag = "[S]"
						style = gitStagedStyle
					} else if gItem.Status == "M" {
						tag = "[M]"
						style = gitModifiedStyle
					}

					line := fmt.Sprintf("%s %s %-2s %s", prefix, tag, gItem.Status, gItem.Path)
					line = truncateString(line, expWidth-2)
					if i == m.gitSel {
						gitSb.WriteString(lipgloss.NewStyle().Bold(true).Foreground(mdPurple).Render(line) + "\n")
					} else {
						gitSb.WriteString(style.Render(line) + "\n")
					}
				}
			}
			gitTitle := fmt.Sprintf("[ GIT CONTROL (%s) ]", m.gitBranch)
			if m.focus == FocusSourceControl {
				gitTitle = fmt.Sprintf("[● GIT CONTROL (%s) ]", m.gitBranch)
			}
			expView = renderPanel(gitTitle, gitSb.String(), m.focus == FocusSourceControl, expWidth, mainHeight)
		} else {
			// File Explorer View
			expVisibleRows := mainHeight - 3
			if expVisibleRows < 1 {
				expVisibleRows = 1
			}

			if m.explorerSel < m.expScrollTop {
				m.expScrollTop = m.explorerSel
			}
			if m.explorerSel >= m.expScrollTop+expVisibleRows {
				m.expScrollTop = m.explorerSel - expVisibleRows + 1
			}
			if m.expScrollTop < 0 {
				m.expScrollTop = 0
			}

			endIdx := m.expScrollTop + expVisibleRows
			if endIdx > len(m.flatItems) {
				endIdx = len(m.flatItems)
			}

			var expSb strings.Builder
			if m.expScrollTop < len(m.flatItems) {
				for i := m.expScrollTop; i < endIdx; i++ {
					item := m.flatItems[i]
					indent := strings.Repeat("  ", item.Depth)
					icon := GetFileIcon(item.Path, item.IsDir, item.Expanded)

					prefix := " "
					if i == m.explorerSel && m.focus == FocusExplorer {
						prefix = ">"
					}

					line := fmt.Sprintf("%s%s %s %s", prefix, indent, icon, item.Name)
					line = truncateString(line, expWidth-2)

					if i == m.explorerSel && m.focus == FocusExplorer {
						expSb.WriteString(lipgloss.NewStyle().Bold(true).Foreground(mdPurple).Render(line) + "\n")
					} else {
						expSb.WriteString(line + "\n")
					}
				}
			}
			expTitle := "[ EXPLORER ]"
			if m.focus == FocusExplorer {
				expTitle = "[● EXPLORER ]"
			}
			expView = renderPanel(expTitle, expSb.String(), m.focus == FocusExplorer, expWidth, mainHeight)
		}
	}

	var edtSb strings.Builder
	tabBar := renderTabBar(m.tabs, m.activeTab, edtWidth-2)
	edtSb.WriteString(tabBar + "\n")

	tab := m.activeTabPtr()
	if tab != nil {
		visibleLines := mainHeight - 4
		if visibleLines < 1 {
			visibleLines = 1
		}

		if tab.ScrollTop < 0 {
			tab.ScrollTop = 0
		}
		endLine := tab.ScrollTop + visibleLines
		if endLine > len(tab.Lines) {
			endLine = len(tab.Lines)
		}

		for i := tab.ScrollTop; i < endLine; i++ {
			rawL := tab.Lines[i]
			lineNum := fmt.Sprintf("%3d │ ", i+1)
			lineNumWidth := 6
			maxContentWidth := edtWidth - 3 - lineNumWidth
			if maxContentWidth < 4 {
				maxContentWidth = 4
			}
			runesL := []rune(rawL)
			if len(runesL) > maxContentWidth {
				runesL = runesL[:maxContentWidth]
			}

			var lineContent string
			if i == tab.CursorLine && m.focus == FocusEditor {
				if tab.CursorCol < len(runesL) {
					left := string(runesL[:tab.CursorCol])
					mid := string(runesL[tab.CursorCol])
					right := string(runesL[tab.CursorCol+1:])
					lineContent = left + lipgloss.NewStyle().Reverse(true).Render(mid) + right
				} else {
					lineContent = string(runesL) + lipgloss.NewStyle().Reverse(true).Render(" ")
				}
				edtSb.WriteString(lipgloss.NewStyle().Foreground(mdPurple).Render(lineNum) + lineContent + "\n")
			} else {
				lineContent = string(runesL)
				edtSb.WriteString(lipgloss.NewStyle().Foreground(mdTextMuted).Render(lineNum) + lineContent + "\n")
			}
		}
	} else {
		edtSb.WriteString(lipgloss.NewStyle().Foreground(mdTextMuted).Render("No open file. Select from Explorer or run /open <file>.") + "\n")
	}

	fileName := "Untitled"
	if tab != nil && tab.Path != "" {
		fileName = filepath.Base(tab.Path)
		if tab.IsModified {
			fileName += " [*]"
		}
	}
	edtTitle := fmt.Sprintf("[ EDITOR: %s ]", fileName)
	if m.focus == FocusEditor {
		edtTitle = fmt.Sprintf("[● EDITOR: %s ]", fileName)
	}
	edtView := renderPanel(edtTitle, edtSb.String(), m.focus == FocusEditor, edtWidth, mainHeight)

	chatInnerWidth := chatWidth - 2
	if chatInnerWidth < 4 {
		chatInnerWidth = 4
	}

	var allChatVisualLines []string
	for _, cMsg := range m.chatHistory {
		var prefix string
		var pLen int
		var style lipgloss.Style

		switch cMsg.Sender {
		case "USER":
			prefix = "You: "
			pLen = 5
			style = userChatMsg
		case "SSBot":
			prefix = "SSBot: "
			pLen = 7
			style = botChatMsg
		case "SYSTEM":
			prefix = "System: "
			pLen = 8
			style = systemChatMsg
		case "ERROR":
			prefix = "Error: "
			pLen = 7
			style = errorChatMsg
		}

		maxTextWidth := chatInnerWidth - pLen
		if maxTextWidth < 4 {
			maxTextWidth = 4
		}

		rawWrapped := wrapRawText(cMsg.Text, maxTextWidth)
		indentStr := strings.Repeat(" ", pLen)

		for j, wLine := range rawWrapped {
			if j == 0 {
				allChatVisualLines = append(allChatVisualLines, style.Render(prefix)+wLine)
			} else {
				allChatVisualLines = append(allChatVisualLines, style.Render(indentStr)+wLine)
			}
		}
	}

	if m.aiPending {
		allChatVisualLines = append(allChatVisualLines, systemChatMsg.Render("SSBot thinking..."))
	}

	chatHistoryMax := mainHeight - 4
	if chatHistoryMax < 1 {
		chatHistoryMax = 1
	}

	cScrollTop := 0
	if len(allChatVisualLines) > chatHistoryMax {
		cScrollTop = len(allChatVisualLines) - chatHistoryMax
	}

	visibleChatLines := allChatVisualLines[cScrollTop:]

	var chatSb strings.Builder

	neededPadding := chatHistoryMax - len(visibleChatLines)
	if neededPadding > 0 {
		chatSb.WriteString(strings.Repeat("\n", neededPadding))
	}

	for _, l := range visibleChatLines {
		chatSb.WriteString(l + "\n")
	}

	tiW := chatInnerWidth - 4
	if tiW < 10 {
		tiW = 10
	}
	m.chatInput.Width = tiW
	chatSb.WriteString(m.chatInput.View())

	chatTitle := fmt.Sprintf("[ SSBOT CHAT (%s) ]", m.currentModel)
	if m.focus == FocusChat {
		chatTitle = fmt.Sprintf("[● SSBOT CHAT (%s) ]", m.currentModel)
	}
	chatView := renderPanel(chatTitle, chatSb.String(), m.focus == FocusChat, chatWidth, mainHeight)

	var mainView string
	if m.sidebarOpen {
		mainView = lipgloss.JoinHorizontal(lipgloss.Top, expView, edtView, chatView)
	} else {
		mainView = lipgloss.JoinHorizontal(lipgloss.Top, edtView, chatView)
	}

	focusName := "EXPLORER"
	if m.focus == FocusEditor {
		focusName = "EDITOR"
	} else if m.focus == FocusChat {
		focusName = "CHAT"
	} else if m.focus == FocusSourceControl {
		focusName = "GIT CONTROL"
	}

	curLine, curCol := 1, 1
	if tab != nil {
		curLine = tab.CursorLine + 1
		curCol = tab.CursorCol + 1
	}

	gitBranch := " " + m.gitBranch
	if m.gitBranch == "" {
		gitBranch = " main"
	}
	lineInfo := fmt.Sprintf("Ln %d, Col %d", curLine, curCol)
	barLeft := statusLeft.Render(" SSCode IDE ") + statusGit.Render(" "+gitBranch+" ")
	barText := fmt.Sprintf(" Focus: %s | File: %s | %s | UTF-8 | 󰍹 %s | %s",
		focusName, fileName, lineInfo, m.currentModel, m.statusMsg)
	if m.statusMsg == "" {
		barText = fmt.Sprintf(" Ctrl+P: Command Palette | Alt+1..9: Tabs | F4: Header | Ctrl+1..4: Focus | 󰍹 %s", m.currentModel)
	}

	barText = truncateString(barText, m.width-lipgloss.Width(barLeft)-2)
	bar := barLeft + statusBody.Render(barText)
	finalView := lipgloss.JoinVertical(lipgloss.Left, mainView, bar)

	if m.showCmdPalette {
		return renderCmdPalette(m.cmdPaletteInput.View(), m.width, m.height)
	}

	if m.showModal {
		var modalSb strings.Builder
		modalSb.WriteString(modalTitle.Render("󰍹 Ollama Model Selection (100% Native Go)") + "\n\n")
		modalSb.WriteString(m.modalStatus + "\n\n")

		if m.modalLoading {
			modalSb.WriteString(systemChatMsg.Render("Processing operation in Ollama (localhost:11434)...") + "\n")
		} else {
			for i, md := range m.modelsList {
				statusTag := "[Installed]"
				if !md.Installed {
					statusTag = "[Available for Download]"
				}
				itemStr := fmt.Sprintf("  %s %-22s %-10s %s", GetFileIcon("model.bin", false, false), md.Name, md.Size, statusTag)
				if i == m.modalSel {
					modalSb.WriteString(modalItemSelected.Render("> "+itemStr) + "\n")
				} else {
					modalSb.WriteString(modalItem.Render("  "+itemStr) + "\n")
				}
			}
		}
		modalSb.WriteString("\n" + lipgloss.NewStyle().Foreground(mdTextMuted).Render("j/k or Arrows: Navigate | Enter: Activate/Download | Esc: Close"))

		mContent := modalBox.Render(modalSb.String())
		return renderModalCentered(mContent, m.width, m.height)
	}

	return finalView
}
