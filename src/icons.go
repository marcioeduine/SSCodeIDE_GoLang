/* ************************************************************************** */
/*                                                                            */
/*                                                       ::::::::   ::::::::  */
/*   icons.go			                               :+:    :+: :+:    :+:  */
/*                                                    +:+        +:+          */
/*   By: Ser Superior <marcioeduine@gmail.com>       +#++:++#++ +#++:++#++    */
/*                                                         +#+        +#+     */
/*   Created: 2026/07/09 10:25:00 by Ser Superior  #+#    #+# #+#    #+#      */
/*   Updated: 2026/07/15 21:20:00 by Ser Superior  ########   ########        */
/*                                                                            */
/* ************************************************************************** */

package main

import (
	"path/filepath"
	"strings"
)

// GetFileIcon returns a real file type icon or directory icon.
func GetFileIcon(path string, isDir bool, isOpen bool) string {
	if isDir {
		if isOpen {
			return " " // open directory icon
		}
		return " " // closed directory icon
	}

	ext := strings.ToLower(filepath.Ext(path))
	name := strings.ToLower(filepath.Base(path))

	// Exact file name matches
	switch name {
	case "makefile", "make":
		return " "
	case "dockerfile", "docker-compose.yml":
		return "󰡨 "
	case ".gitignore", ".gitattributes":
		return " "
	case "package.json", "go.mod", "go.sum", "cargo.toml":
		return "󰏗 "
	case "readme.md", "license":
		return "󰈙 "
	}

	// Extension matches
	switch ext {
	case ".go":
		return " "
	case ".cpp", ".cxx", ".cc":
		return " "
	case ".c":
		return " "
	case ".h", ".hpp", ".hxx":
		return " "
	case ".py", ".pyw":
		return " "
	case ".lua":
		return " "
	case ".js", ".mjs", ".cjs":
		return " "
	case ".ts", ".tsx":
		return " "
	case ".html", ".htm":
		return " "
	case ".css", ".scss", ".sass":
		return " "
	case ".json":
		return " "
	case ".yaml", ".yml":
		return " "
	case ".toml", ".ini", ".conf", ".cfg":
		return " "
	case ".md", ".markdown":
		return " "
	case ".sh", ".bash", ".zsh", ".fish":
		return " "
	case ".txt":
		return "󰈙 "
	case ".png", ".jpg", ".jpeg", ".gif", ".svg", ".ico":
		return "󰋩 "
	case ".zip", ".tar", ".gz", ".7z", ".bz2":
		return " "
	default:
		return "󰈙 "
	}
}
