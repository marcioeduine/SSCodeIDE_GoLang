/* ************************************************************************** */
/*                                                                            */
/*                                                       ::::::::   ::::::::  */
/*   ui.go				                               :+:    :+: :+:    :+:  */
/*                                                    +:+        +:+          */
/*   By: Ser Superior <marcioeduine@gmail.com>       +#++:++#++ +#++:++#++    */
/*                                                         +#+        +#+     */
/*   Created: 2026/07/09 10:25:00 by Ser Superior  #+#    #+# #+#    #+#      */
/*   Updated: 2026/07/15 21:20:00 by Ser Superior  ########   ########        */
/*                                                                            */
/* ************************************************************************** */

package main

import (
	"fmt"
	"path/filepath"
	"strings"

	"github.com/charmbracelet/lipgloss"
)

var (
	mdBgDark      = lipgloss.Color("#1E1E2E")
	mdSurfaceDark = lipgloss.Color("#252630")
	mdPanelHeader = lipgloss.Color("#181825")
	mdPurple      = lipgloss.Color("#D0BCFF")
	mdPurpleDark  = lipgloss.Color("#381E72")
	mdCyan        = lipgloss.Color("#89B4FA")
	mdTextPrimary = lipgloss.Color("#CDD6F4")
	mdTextMuted   = lipgloss.Color("#6C7086")
	mdBorderDim   = lipgloss.Color("#45475A")
	mdGreen       = lipgloss.Color("#A6E3A1")
	mdRed         = lipgloss.Color("#F38BA8")
	mdTabActiveBg = lipgloss.Color("#313244")

	boxFocused = lipgloss.NewStyle().
			Border(lipgloss.RoundedBorder()).
			BorderForeground(mdPurple).
			Padding(0, 0)

	boxUnfocused = lipgloss.NewStyle().
			Border(lipgloss.RoundedBorder()).
			BorderForeground(mdBorderDim).
			Padding(0, 0)

	titleFocused = lipgloss.NewStyle().
			Bold(true).
			Foreground(lipgloss.Color("#11111B")).
			Background(mdPurple).
			Padding(0, 1)

	titleUnfocused = lipgloss.NewStyle().
			Bold(true).
			Foreground(mdTextMuted).
			Background(mdSurfaceDark).
			Padding(0, 1)

	tabActiveStyle = lipgloss.NewStyle().
			Bold(true).
			Foreground(mdTextPrimary).
			Background(mdTabActiveBg).
			Padding(0, 1).
			Border(lipgloss.NormalBorder(), false, false, true, false).
			BorderForeground(mdPurple)

	tabInactiveStyle = lipgloss.NewStyle().
			Foreground(mdTextMuted).
			Background(mdPanelHeader).
			Padding(0, 1)

	statusLeft = lipgloss.NewStyle().
			Bold(true).
			Foreground(lipgloss.Color("#11111B")).
			Background(mdPurple).
			Padding(0, 1)

	statusGit = lipgloss.NewStyle().
			Foreground(lipgloss.Color("#11111B")).
			Background(mdCyan).
			Padding(0, 1)

	statusBody = lipgloss.NewStyle().
			Foreground(mdTextPrimary).
			Background(mdSurfaceDark).
			Padding(0, 1)

	modalBox = lipgloss.NewStyle().
			Border(lipgloss.RoundedBorder()).
			BorderForeground(mdPurple).
			Background(mdSurfaceDark).
			Padding(1, 3)

	modalTitle = lipgloss.NewStyle().
			Bold(true).
			Foreground(mdPurple).
			MarginBottom(1)

	modalItem = lipgloss.NewStyle().
			Foreground(mdTextPrimary)

	modalItemSelected = lipgloss.NewStyle().
				Bold(true).
				Foreground(lipgloss.Color("#11111B")).
				Background(mdPurple)

	cmdBox = lipgloss.NewStyle().
		Border(lipgloss.RoundedBorder()).
		BorderForeground(mdCyan).
		Background(mdSurfaceDark).
		Padding(0, 1)

	userChatMsg = lipgloss.NewStyle().
			Foreground(mdPurple).
			Bold(true)

	botChatMsg = lipgloss.NewStyle().
			Foreground(mdCyan)

	systemChatMsg = lipgloss.NewStyle().
			Foreground(mdGreen).
			Italic(true)

	errorChatMsg = lipgloss.NewStyle().
			Foreground(mdRed).
			Bold(true)

	gitStagedStyle = lipgloss.NewStyle().
			Foreground(mdGreen).
			Bold(true)

	gitModifiedStyle = lipgloss.NewStyle().
			Foreground(mdCyan).
			Bold(true)

	gitUntrackedStyle = lipgloss.NewStyle().
			Foreground(mdRed)
)

func setTheme(themeName string) string {
	switch strings.ToLower(themeName) {
	case "dracula":
		mdPurple = lipgloss.Color("#BD93F9")
		mdCyan = lipgloss.Color("#8BE9FD")
		mdSurfaceDark = lipgloss.Color("#282A36")
		mdBorderDim = lipgloss.Color("#44475A")
		mdTextPrimary = lipgloss.Color("#F8F8F2")
		return "Theme changed to Dracula"
	case "nord":
		mdPurple = lipgloss.Color("#B48EAD")
		mdCyan = lipgloss.Color("#88C0D0")
		mdSurfaceDark = lipgloss.Color("#2E3440")
		mdBorderDim = lipgloss.Color("#4C566A")
		mdTextPrimary = lipgloss.Color("#ECEFF4")
		return "Theme changed to Nord"
	case "catppuccin", "mocha", "dark":
		mdPurple = lipgloss.Color("#D0BCFF")
		mdCyan = lipgloss.Color("#89B4FA")
		mdSurfaceDark = lipgloss.Color("#252630")
		mdBorderDim = lipgloss.Color("#45475A")
		mdTextPrimary = lipgloss.Color("#CDD6F4")
		return "Theme changed to Catppuccin / Material V3"
	default:
		return "Available themes: catppuccin, dracula, nord"
	}
}

func truncateString(s string, maxWidth int) string {
	if maxWidth <= 0 {
		return ""
	}
	runes := []rune(s)
	if len(runes) <= maxWidth {
		return s
	}
	return string(runes[:maxWidth])
}

func wrapRawText(text string, maxWidth int) []string {
	if maxWidth <= 0 {
		maxWidth = 10
	}
	var lines []string
	for _, paragraph := range strings.Split(text, "\n") {
		if paragraph == "" {
			lines = append(lines, "")
			continue
		}
		runes := []rune(paragraph)
		for len(runes) > 0 {
			if len(runes) <= maxWidth {
				lines = append(lines, string(runes))
				break
			}
			cut := maxWidth
			for i := maxWidth; i > 0; i-- {
				if runes[i] == ' ' {
					cut = i
					break
				}
			}
			lines = append(lines, string(runes[:cut]))
			if cut < len(runes) && runes[cut] == ' ' {
				cut++
			}
			runes = runes[cut:]
		}
	}
	return lines
}

func renderPanel(title string, content string, focused bool, width, height int) string {
	var style lipgloss.Style
	var tStyle lipgloss.Style

	if focused {
		style = boxFocused
		tStyle = titleFocused
	} else {
		style = boxUnfocused
		tStyle = titleUnfocused
	}

	innerWidth := width - 2
	innerHeight := height - 2
	if innerWidth < 4 {
		innerWidth = 4
	}
	if innerHeight < 2 {
		innerHeight = 2
	}

	dispTitle := title
	if len([]rune(dispTitle)) > innerWidth {
		dispTitle = truncateString(dispTitle, innerWidth-1) + "…"
	}
	header := tStyle.Render(dispTitle)

	contentLines := strings.Split(content, "\n")
	fixedLines := make([]string, 0, innerHeight)

	fixedLines = append(fixedLines, header)

	bodyHeight := innerHeight - 1

	for i := 0; i < bodyHeight; i++ {
		line := ""
		if i < len(contentLines) {
			line = contentLines[i]
		}
		fixedLines = append(fixedLines, line)
	}

	body := strings.Join(fixedLines, "\n")
	style = style.Width(innerWidth).Height(innerHeight)

	return style.Render(body)
}

func renderTabBar(tabs []EditorTab, activeIdx int, width int) string {
	if width <= 0 {
		return ""
	}
	if len(tabs) == 0 {
		msg := tabInactiveStyle.Render("[ No Open Files ]")
		if lipgloss.Width(msg) > width {
			runes := []rune(msg)
			if len(runes) > width {
				return string(runes[:width])
			}
		}
		return msg
	}

	var renderedTabs []string
	for i, tab := range tabs {
		icon := GetFileIcon(tab.Path, false, false)
		modifiedMark := ""
		if tab.IsModified {
			modifiedMark = " [*]"
		}
		title := fmt.Sprintf("%s %s%s ×", icon, filepath.Base(tab.Path), modifiedMark)

		if i == activeIdx {
			renderedTabs = append(renderedTabs, tabActiveStyle.Render(title))
		} else {
			renderedTabs = append(renderedTabs, tabInactiveStyle.Render(title))
		}
	}

	row := strings.Join(renderedTabs, " ")
	if lipgloss.Width(row) > width {
		runes := []rune(row)
		if len(runes) > width {
			return string(runes[:width])
		}
	}
	return row
}

func renderModalCentered(modalContent string, width, height int) string {
	return lipgloss.Place(
		width,
		height,
		lipgloss.Center,
		lipgloss.Center,
		modalContent,
		lipgloss.WithWhitespaceBackground(lipgloss.Color("#11111B")),
		lipgloss.WithWhitespaceChars(" "),
	)
}

func renderCmdPalette(cmdInputView string, width, height int) string {
	cContent := cmdBox.Render(cmdInputView)

	return lipgloss.Place(
		width,
		height,
		lipgloss.Center,
		lipgloss.Center,
		cContent,
		lipgloss.WithWhitespaceBackground(lipgloss.Color("#11111B")),
		lipgloss.WithWhitespaceChars(" "),
	)
}
