/* ************************************************************************** */
/*                                                                            */
/*                                                       ::::::::   ::::::::  */
/*   main.go			                               :+:    :+: :+:    :+:  */
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
	"os"

	tea "github.com/charmbracelet/bubbletea"
)

func main() {
	p := tea.NewProgram(
		initialModel(),
		tea.WithAltScreen(),
		tea.WithMouseCellMotion(),
	)
	if _, err := p.Run(); err != nil {
		fmt.Printf("Error executing SSCodeIDE: %v\n", err)
		os.Exit(1)
	}
}
