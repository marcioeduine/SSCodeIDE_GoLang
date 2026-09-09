/* ************************************************************************** */
/*                                                                            */
/*                                                       ::::::::   ::::::::  */
/*   ai.go				                               :+:    :+: :+:    :+:  */
/*                                                    +:+        +:+          */
/*   By: Ser Superior <marcioeduine@gmail.com>       +#++:++#++ +#++:++#++    */
/*                                                         +#+        +#+     */
/*   Created: 2026/07/09 10:25:00 by Ser Superior  #+#    #+# #+#    #+#      */
/*   Updated: 2026/07/15 21:20:00 by Ser Superior  ########   ########        */
/*                                                                            */
/* ************************************************************************** */

package main

import (
	"bytes"
	"encoding/json"
	"fmt"
	"io"
	"net/http"
	"os"
	"os/exec"
	"strings"
	"time"

	tea "github.com/charmbracelet/bubbletea"
)

func getOllamaBaseURL() string {
	if host := os.Getenv("OLLAMA_HOST"); host != "" {
		if !strings.HasPrefix(host, "http://") && !strings.HasPrefix(host, "https://") {
			return "http://" + host
		}
		return host
	}
	return "http://localhost:11434"
}

type ModelInfo struct {
	Name      string `json:"name"`
	Installed bool   `json:"installed"`
	Size      string `json:"size"`
}

type modelsMsg struct {
	models []ModelInfo
	err    error
}

type aiResponseMsg struct {
	response string
	err      error
}

type pullFinishedMsg struct {
	model string
	err   error
}

type setModelFinishedMsg struct {
	model string
	err   error
}

type shellResultMsg struct {
	cmd    string
	output string
	err    error
}

type ollamaTagItem struct {
	Name string `json:"name"`
	Size int64  `json:"size"`
}

type ollamaTagsResponse struct {
	Models []ollamaTagItem `json:"models"`
}

type ollamaGenerateRequest struct {
	Model  string `json:"model"`
	Prompt string `json:"prompt"`
	Stream bool   `json:"stream"`
}

type ollamaGenerateResponse struct {
	Response string `json:"response"`
	Error    string `json:"error,omitempty"`
}

type ollamaPullRequest struct {
	Name   string `json:"name"`
	Stream bool   `json:"stream"`
}

var recommendedModels = []string{
	"SSBot:latest",
	"deepseek-coder:1.3b",
	"qwen2.5-coder:0.5b",
	"qwen2.5-coder:1.5b",
	"qwen2.5-coder:7b",
	"llama3.2:1b",
	"llama3.2:3b",
	"codellama:7b",
	"mistral:latest",
	"phi3:mini",
	"gemma2:2b",
	"deepseek-r1:1.5b",
}

func fetchModelsCmd() tea.Cmd {
	return func() tea.Msg {
		baseURL := getOllamaBaseURL()
		client := http.Client{Timeout: 5 * time.Second}
		resp, err := client.Get(baseURL + "/api/tags")

		installedMap := make(map[string]int64)
		if err == nil && resp.StatusCode == 200 {
			body, _ := io.ReadAll(resp.Body)
			resp.Body.Close()

			var tagsResp ollamaTagsResponse
			if err := json.Unmarshal(body, &tagsResp); err == nil {
				for _, item := range tagsResp.Models {
					installedMap[item.Name] = item.Size
				}
			}
		}

		var result []ModelInfo
		seen := make(map[string]bool)

		for name, sizeBytes := range installedMap {
			seen[name] = true
			sizeMB := fmt.Sprintf("%.1f GB", float64(sizeBytes)/(1024*1024*1024))
			result = append(result, ModelInfo{
				Name:      name,
				Installed: true,
				Size:      sizeMB,
			})
		}

		for _, name := range recommendedModels {
			if !seen[name] {
				seen[name] = true
				result = append(result, ModelInfo{
					Name:      name,
					Installed: false,
					Size:      "Remote",
				})
			}
		}

		if len(result) == 0 {
			return modelsMsg{err: fmt.Errorf("No models found. Ensure Ollama service is running at %s", baseURL)}
		}

		return modelsMsg{models: result}
	}
}

func pullModelCmd(modelName string) tea.Cmd {
	return func() tea.Msg {
		baseURL := getOllamaBaseURL()
		reqPayload := ollamaPullRequest{
			Name:   modelName,
			Stream: false,
		}
		data, err := json.Marshal(reqPayload)
		if err != nil {
			return pullFinishedMsg{model: modelName, err: err}
		}

		client := http.Client{Timeout: 10 * time.Minute}
		resp, err := client.Post(baseURL+"/api/pull", "application/json", bytes.NewBuffer(data))
		if err != nil {
			return pullFinishedMsg{model: modelName, err: fmt.Errorf("Failed to connect to Ollama service (%s): %v", baseURL, err)}
		}
		defer resp.Body.Close()

		if resp.StatusCode != 200 {
			body, _ := io.ReadAll(resp.Body)
			return pullFinishedMsg{model: modelName, err: fmt.Errorf("Ollama API error (%d): %s", resp.StatusCode, string(body))}
		}

		return pullFinishedMsg{model: modelName}
	}
}

func setModelCmd(modelName string) tea.Cmd {
	return func() tea.Msg {
		return setModelFinishedMsg{model: modelName}
	}
}

func sendAIChatCmd(prompt string, modelName string) tea.Cmd {
	return func() tea.Msg {
		baseURL := getOllamaBaseURL()
		if modelName == "" {
			modelName = "deepseek-coder:1.3b"
		}

		reqPayload := ollamaGenerateRequest{
			Model:  modelName,
			Prompt: prompt,
			Stream: false,
		}
		data, err := json.Marshal(reqPayload)
		if err != nil {
			return aiResponseMsg{err: err}
		}

		client := http.Client{Timeout: 2 * time.Minute}
		resp, err := client.Post(baseURL+"/api/generate", "application/json", bytes.NewBuffer(data))
		if err != nil {
			return aiResponseMsg{err: fmt.Errorf("Failed to communicate with Ollama (%s): %v", baseURL, err)}
		}
		defer resp.Body.Close()

		body, err := io.ReadAll(resp.Body)
		if err != nil {
			return aiResponseMsg{err: fmt.Errorf("Failed to read Ollama response: %v", err)}
		}

		if resp.StatusCode != 200 {
			return aiResponseMsg{err: fmt.Errorf("Ollama API error (%d): %s", resp.StatusCode, string(body))}
		}

		var genResp ollamaGenerateResponse
		if err := json.Unmarshal(body, &genResp); err != nil {
			return aiResponseMsg{err: fmt.Errorf("Failed to parse JSON response: %v", err)}
		}

		if genResp.Error != "" {
			return aiResponseMsg{err: fmt.Errorf("Ollama error: %s", genResp.Error)}
		}

		return aiResponseMsg{response: strings.TrimSpace(genResp.Response)}
	}
}

func runShellCmd(cmdStr string) tea.Cmd {
	return func() tea.Msg {
		cmd := exec.Command("bash", "-c", cmdStr)
		out, err := cmd.CombinedOutput()
		output := strings.TrimSpace(string(out))
		if err != nil && output == "" {
			output = err.Error()
		}
		return shellResultMsg{cmd: cmdStr, output: output, err: err}
	}
}
