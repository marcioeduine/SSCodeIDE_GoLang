/* ************************************************************************** */
/*                                                                            */
/*                                                       ::::::::   ::::::::  */
/*    main.cpp                                         :+:    :+: :+:    :+:  */
/*                                                    +:+        +:+          */
/*    By: Ser Superior <marcioeduine@gmail.com>      +#++:++#++ +#++:++#++    */
/*                                                         +#+        +#+     */
/*    Created: 2026/07/02 10:37:47 by Ser Superior #+#    #+# #+#    #+#      */
/*    Updated: 2026/09/09 16:50:00 by Ser Superior ########   ########        */
/*                                                                            */
/* ************************************************************************** */

#include "sscode.hpp"
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <sstream>

static char	CharToLower(char ch)
{
	return (static_cast<char>(std::tolower(static_cast<unsigned char>(ch))));
}

static t_text	TrimCopy(const t_text &s)
{
	size_t	start(0);
	size_t	end(0);

	if (s.empty()) return ("");
	start = s.find_first_not_of(" \t\r\n");
	if (start == t_text::npos) return ("");
	end = s.find_last_not_of(" \t\r\n");
	return (s.substr(start, end - start + 1));
}

static t_text	CodepointToUtf8(wint_t cp)
{
	t_text	out = "";
	if (cp <= 0x7F)
	{
		out += static_cast<char>(cp);
	}
	else if (cp <= 0x7FF)
	{
		out += static_cast<char>(0xC0 | ((cp >> 6) & 0x1F));
		out += static_cast<char>(0x80 | (cp & 0x3F));
	}
	else if (cp <= 0xFFFF)
	{
		out += static_cast<char>(0xE0 | ((cp >> 12) & 0x0F));
		out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
		out += static_cast<char>(0x80 | (cp & 0x3F));
	}
	else if (cp <= 0x10FFFF)
	{
		out += static_cast<char>(0xF0 | ((cp >> 18) & 0x07));
		out += static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
		out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
		out += static_cast<char>(0x80 | (cp & 0x3F));
	}
	return (out);
}

static bool	HandleCliCommand(EditorContext &ctx, const t_text &input)
{
	t_text	line(TrimCopy(input));
	t_text	cmd("");
	t_text	arg("");
	size_t	sp(0);

	if (line.empty() or line[0] != '/') return (false);
	sp = line.find(' ');
	if (sp == t_text::npos) cmd = line.substr(1);
	else
	{
		cmd = line.substr(1, sp - 1);
		arg = TrimCopy(line.substr(sp + 1));
	}

	std::transform(cmd.begin(), cmd.end(), cmd.begin(), CharToLower);

	if (cmd == "help")
	{
		ctx.AddCliMessage("── [SSBot CLI Commands]", "───────────────────────────");
		ctx.AddCliMessage("  /help       ", "Shows this help list");
		ctx.AddCliMessage("  /models     ", "Lists available Ollama AI models");
		ctx.AddCliMessage("  /pull <mod> ", "Downloads a model via Ollama (e.g. /pull qwen2.5-coder:0.5b)");
		ctx.AddCliMessage("  /set_model  ", "Sets the active model for SSBot");
		ctx.AddCliMessage("  /clear      ", "Clears chat history and AI context");
		ctx.AddCliMessage("  /files, /ls ", "Opens drawer and lists files");
		ctx.AddCliMessage("  /drawer, /exp", "Toggles/collapses Explorer window");
		ctx.AddCliMessage("  /chat, /close", "Collapses SSBot CLI panel");
		ctx.AddCliMessage("  /expand      ", "Toggles expanded size of SSBot CLI");
		ctx.AddCliMessage("  /open <file>", "Opens a file in the editor");
		ctx.AddCliMessage("  /save       ", "Saves currently active file");
		ctx.AddCliMessage("  /model      ", "Shows active AI model");
		ctx.AddCliMessage("  /editor     ", "Returns focus to code editor");
		ctx.AddCliMessage("  /quit, /exit", "Quits SSCodeIDE");
		ctx.AddCliMessage("  !<command>  ", "Executes shell command (e.g. !ls, !git status)");
		ctx.AddCliMessage("  <question>  ", "Sends a question directly to SSBot");
		ctx.AddCliMessage("──────────────────────────────────────────────────────", "");
		return (true);
	}
	if (cmd == "models")
	{
		ctx.OpenModelModal();
		return (true);
	}
	if (cmd == "pull" or cmd == "download")
	{
		if (arg.empty())
		{
			ctx.OpenModelModal();
			return (true);
		}
		ctx.AddCliMessage("[System]", "Downloading model '" + arg + "' via Ollama...");
		t_text pull_cmd = "./ss_ai_bridge.py --pull '" + arg + "' 2>&1";
		FILE *pipe = popen(pull_cmd.c_str(), "r");
		if (pipe)
		{
			char buffer[512];
			while (fgets(buffer, sizeof(buffer), pipe) != NULL)
			{
				t_text line = SanitizeForUi(buffer);
				if (!line.empty())
					ctx.ai_response_lines.push_back("  " + line);
			}
			pclose(pipe);
		}
		ctx.ScrollCliToBottom();
		return (true);
	}
	if (cmd == "set_model" or (cmd == "model" and !arg.empty()))
	{
		t_text model_name = arg;
		if (model_name.empty())
		{
			ctx.AddCliMessage("[Error]", "Correct usage: /set_model <model_name>");
			return (true);
		}
		t_text set_cmd = "./ss_ai_bridge.py --set-model '" + model_name + "' 2>&1";
		FILE *pipe = popen(set_cmd.c_str(), "r");
		if (pipe)
		{
			char buffer[512];
			while (fgets(buffer, sizeof(buffer), pipe) != NULL)
			{
				t_text line = SanitizeForUi(buffer);
				if (!line.empty())
					ctx.ai_response_lines.push_back("  " + line);
			}
			pclose(pipe);
		}
		ctx.model_modal_active_model = model_name;
		ctx.AddCliMessage("[System]", "Active model updated to: " + model_name);
		ctx.ScrollCliToBottom();
		return (true);
	}
	if (cmd == "editor")
	{
		ctx.current_mode = MODE_EDITOR;
		ctx.AddCliMessage("[System]", "Focus returned to code editor.");
		return (true);
	}
	if (cmd == "clear")
	{
		t_text clean_cmd = "./ss_ai_bridge.py '__CLEAR_CONTEXT__' > /dev/null 2>&1";
		int dummy = system(clean_cmd.c_str());
		(void)dummy;
		ctx.ai_response_lines.clear();
		ctx.ai_scroll_offset = 0;
		ctx.AddCliMessage("[System]", "Context and conversation history cleared.");
		return (true);
	}
	if (cmd == "files" or cmd == "ls")
	{
		ctx.LoadDirectoryFiles();
		ctx.drawer_open = true;
		ctx.current_mode = MODE_FILE_MANAGER;
		ctx.AddCliMessage("[System]", "Explorer opened with " + IntToString(ctx.explorer_items.size()) + " items.");
		return (true);
	}
	if (cmd == "drawer" or cmd == "explorer" or cmd == "exp")
	{
		ctx.drawer_open = !ctx.drawer_open;
		if (ctx.drawer_open) ctx.LoadDirectoryFiles();
		ctx.AddCliMessage("[System]", ctx.drawer_open ? "Explorer expanded." : "Explorer collapsed.");
		return (true);
	}
	if (cmd == "chat" or cmd == "close" or cmd == "hide")
	{
		ctx.ai_panel_open = false;
		if (ctx.current_mode == MODE_AI_PROMPT) ctx.current_mode = MODE_EDITOR;
		return (true);
	}
	if (cmd == "expand")
	{
		ctx.ai_panel_expanded = !ctx.ai_panel_expanded;
		ctx.AddCliMessage("[System]", ctx.ai_panel_expanded ? "SSBot CLI maximised." : "SSBot CLI standard size.");
		return (true);
	}
	if (cmd == "save")
	{
		if (!ctx.open_files.empty()) ctx.SaveFile();
		ctx.AddCliMessage("[System]", "File saved successfully.");
		return (true);
	}
	if (cmd == "open")
	{
		if (arg.empty())
		{
			ctx.AddCliMessage("[Error]", "Correct usage: /open <file_path>");
			return (true);
		}
		ctx.OpenFileBuffer(arg);
		ctx.current_mode = MODE_EDITOR;
		ctx.AddCliMessage("[System]", "File opened: " + arg);
		return (true);
	}
	if (cmd == "model")
	{
		FILE *pipe = popen("./ss_ai_bridge.py --list 2>&1", "r");
		if (pipe)
		{
			char buffer[512];
			while (fgets(buffer, sizeof(buffer), pipe) != NULL)
			{
				t_text line = SanitizeForUi(buffer);
				if (line.find("MODELO_ACTIVO:") == 0)
					ctx.AddCliMessage("[SSBot]", "Active Ollama model: " + line.substr(15));
			}
			pclose(pipe);
		}
		return (true);
	}
	if (cmd == "quit" or cmd == "exit")
	{
		ctx.should_quit = true;
		return (true);
	}
	ctx.AddCliMessage("[Error]", "Unknown command: /" + cmd + ". Type /help.");
	return (true);
}

static void	ExecuteShellCommand(EditorContext &ctx, const t_text &cmd_str)
{
	t_text	shell_cmd = TrimCopy(cmd_str);
	if (shell_cmd.empty()) return ;

	ctx.AddCliMessage("[Command]", "$ " + shell_cmd);

	FILE *pipe = popen((shell_cmd + " 2>&1").c_str(), "r");
	if (!pipe)
	{
		ctx.AddCliMessage("[Error]", "Failed to execute shell command.");
		return ;
	}

	char buffer[512];
	bool had_output = false;
	while (fgets(buffer, sizeof(buffer), pipe) != NULL)
	{
		t_text line = SanitizeForUi(buffer);
		if (!line.empty())
		{
			ctx.ai_response_lines.push_back("  " + line);
			had_output = true;
		}
	}
	pclose(pipe);

	if (!had_output)
		ctx.ai_response_lines.push_back("  (executed with exit code 0)");

	ctx.ScrollCliToBottom();
}

static void	ExecuteAIPrompt(EditorContext &ctx, RendererEngine &renderer)
{
	t_text	input = TrimCopy(ctx.ai_current_input);
	ctx.ai_current_input = "";
	ctx.ai_prompt_cursor = 0;

	if (input.empty())
		return ;

	// 1. Comando shell via prefixo "!" ou "/run "
	if (input[0] == '!')
	{
		ExecuteShellCommand(ctx, input.substr(1));
		renderer.RefreshScreen(ctx);
		return ;
	}
	if (input.find("/run ") == 0)
	{
		ExecuteShellCommand(ctx, input.substr(5));
		renderer.RefreshScreen(ctx);
		return ;
	}

	// 2. Comando interno CLI
	if (input[0] == '/')
	{
		ctx.AddCliMessage("You ❯", input);
		HandleCliCommand(ctx, input);
		renderer.RefreshScreen(ctx);
		return ;
	}

	// 3. Conversação inteligente com SSBot
	ctx.AddCliMessage("You ❯", input);
	ctx.ai_response_lines.push_back("  [SSBot thinking...]");
	ctx.ScrollCliToBottom();

	renderer.RefreshScreen(ctx);

	t_text escaped_prompt = "";
	for (size_t i = 0; i < input.length(); ++i)
	{
		if (input[i] == '\'') escaped_prompt += "'\\''";
		else escaped_prompt += input[i];
	}

	t_text cmd = "./ss_ai_bridge.py '" + escaped_prompt + "' 2>&1";
	FILE *pipe = popen(cmd.c_str(), "r");
	t_text full_response = "";

	if (pipe)
	{
		char buffer[512];
		while (fgets(buffer, sizeof(buffer), pipe) != NULL)
			full_response += buffer;
		pclose(pipe);
	}

	// Remove indicador temporário
	if (!ctx.ai_response_lines.empty())
		ctx.ai_response_lines.pop_back();

	full_response = SanitizeForUi(full_response);
	if (TrimCopy(full_response).empty())
		full_response = "// No response from SSBot. Check if Ollama is active.";

	t_vector wrapped_lines;
	int max_text_width = ctx.screen_cols - 6;
	if (max_text_width < 10) max_text_width = 10;
	renderer.WrapText(full_response, max_text_width, wrapped_lines);

	if (!wrapped_lines.empty())
	{
		ctx.ai_response_lines.push_back("SSBot ❯ " + wrapped_lines[0]);
		for (size_t w = 1; w < wrapped_lines.size(); ++w)
			ctx.ai_response_lines.push_back("         " + wrapped_lines[w]);
	}
	else
	{
		ctx.ai_response_lines.push_back("SSBot ❯ (no content)");
	}

	ctx.ScrollCliToBottom();
	renderer.RefreshScreen(ctx);
}

static void	InsertUtf8Char(EditorContext &ctx, const t_text &utf8_char)
{
	if (ctx.open_files.empty()) return ;
	FileBuffer	&cur(ctx.open_files[ctx.active_file_idx]);
	t_vector	chars(SplitUtf8String(cur.lines[cur.cursor_y]));

	ctx.SaveUndoState(cur);
	if (cur.cursor_x > static_cast<int>(chars.size()))
		cur.cursor_x = static_cast<int>(chars.size());
	chars.insert(chars.begin() + cur.cursor_x, utf8_char);
	cur.lines[cur.cursor_y] = JoinUtf8Chars(chars);
	cur.cursor_x++;
}

static void	DeleteChar(EditorContext &ctx)
{
	if (ctx.open_files.empty()) return ;
	FileBuffer	&cur(ctx.open_files[ctx.active_file_idx]);
	t_vector	chars(SplitUtf8String(cur.lines[cur.cursor_y]));

	ctx.SaveUndoState(cur);
	if (cur.cursor_x > 0)
	{
		if (cur.cursor_x <= static_cast<int>(chars.size()))
			chars.erase(chars.begin() + cur.cursor_x - 1);
		cur.lines[cur.cursor_y] = JoinUtf8Chars(chars);
		cur.cursor_x--;
	}
	else if (cur.cursor_y > 0)
	{
		cur.cursor_x = SplitUtf8String(ctx.open_files[ctx.active_file_idx].lines[cur.cursor_y - 1]).size();
		ctx.open_files[ctx.active_file_idx].lines[cur.cursor_y - 1] += cur.lines[cur.cursor_y];
		ctx.open_files[ctx.active_file_idx].lines.erase(ctx.open_files[ctx.active_file_idx].lines.begin() + cur.cursor_y);
		cur.cursor_y--;
	}
}

static void	UpdateHoverTarget(EditorContext &ctx, int x, int y)
{
	ctx.hover_target = "";
	int nav_rail_width = 4;
	int drawer_width = ctx.drawer_open ? 24 : 0;
	if (drawer_width > ctx.screen_cols / 3) drawer_width = ctx.screen_cols / 3;

	int ai_panel_width = 0;
	if (ctx.ai_panel_open)
	{
		if (ctx.ai_panel_expanded)
			ai_panel_width = ctx.screen_cols * 45 / 100;
		else
			ai_panel_width = ctx.screen_cols * 32 / 100;
		if (ai_panel_width > ctx.screen_cols / 2) ai_panel_width = ctx.screen_cols / 2;
		if (ai_panel_width < 22) ai_panel_width = 22;
	}
	int edit_window_height = ctx.screen_rows - 3;
	if (edit_window_height < 1) edit_window_height = 1;

	// 1. Linha Superior (Header)
	if (y == 0)
	{
		int title_w = GetDisplayWidth(" 󱚥 SSCodeIDE ");
		t_text btn_exp = ctx.drawer_open ? "[ 📁 Explorer:ON ] " : "[ 📁 Explorer:OFF ] ";
		t_text btn_chat = ctx.ai_panel_open ? "[ 󱚥 SSBot:ON ] " : "[ 󱚥 SSBot:OFF ] ";
		t_text btn_save = "[ 💾 Save:^S ] ";
		t_text btn_open = "[ 📂 Open:^O ] ";

		int exp_start = title_w;
		int exp_end = exp_start + GetDisplayWidth(btn_exp);
		int chat_start = exp_end;
		int chat_end = chat_start + GetDisplayWidth(btn_chat);
		int save_start = chat_end;
		int save_end = save_start + GetDisplayWidth(btn_save);
		int open_start = save_end;
		int open_end = open_start + GetDisplayWidth(btn_open);

		if (x >= exp_start and x < exp_end) ctx.hover_target = "btn_explorer";
		else if (x >= chat_start and x < chat_end) ctx.hover_target = "btn_chat";
		else if (x >= save_start and x < save_end) ctx.hover_target = "btn_save";
		else if (x >= open_start and x < open_end) ctx.hover_target = "btn_open";
		return ;
	}

	// 2. Linha de Tabs
	if (y == 1)
	{
		if (x >= nav_rail_width)
		{
			int tab_col = nav_rail_width;
			for (size_t i = 0; i < ctx.open_files.size(); ++i)
			{
				t_text tab_name = ctx.open_files[i].name.empty() ? "*Untitled*" : ctx.open_files[i].name;
				tab_name = FitText(tab_name, 20);
				t_text full_tab = (static_cast<int>(i) == ctx.active_file_idx ? " " : "│ ") + tab_name + (ctx.open_files[i].is_dirty ? " [+]" : "") + " ";
				int tw = GetDisplayWidth(full_tab);
				if (x >= tab_col and x < tab_col + tw)
				{
					ctx.hover_target = "tab_" + IntToString(static_cast<int>(i));
					return ;
				}
				tab_col += tw;
			}
			if (x >= tab_col and x < tab_col + 11)
				ctx.hover_target = "btn_new_tab";
		}
		return ;
	}

	// 3. Nav Rail Lateral
	if (x >= 0 and x < nav_rail_width)
	{
		if (y == 2 or y == 3) ctx.hover_target = "nav_explorer";
		else if (y == 5 or y == 6) ctx.hover_target = "nav_chat";
		else if (y == 8 or y == 9) ctx.hover_target = "nav_help";
		return ;
	}

	// 4. SSBot Chat Column (Direita)
	if (ctx.ai_panel_open and x >= ctx.screen_cols - ai_panel_width)
	{
		if (y == 2) ctx.hover_target = "cli_header";
		else ctx.hover_target = "cli_body";
		return ;
	}

	// 5. Drawer lateral (Explorer)
	if (ctx.drawer_open and x >= nav_rail_width and x < nav_rail_width + drawer_width and y >= 2 and y < 2 + edit_window_height)
	{
		if (y == 2)
		{
			ctx.hover_target = "explorer_header";
			return ;
		}
		int item_idx = y - 3;
		if (item_idx >= 0 and item_idx < static_cast<int>(ctx.explorer_items.size()))
		{
			ctx.hover_target = "explorer_item_" + IntToString(item_idx);
			ctx.selected_file_idx = item_idx;
		}
		return ;
	}
}

static void	HandleMouseClick(EditorContext &ctx, RendererEngine &renderer, int x, int y)
{
	(void)renderer;
	UpdateHoverTarget(ctx, x, y);

	int nav_rail_width = 4;
	int drawer_width = ctx.drawer_open ? 24 : 0;
	if (drawer_width > ctx.screen_cols / 3) drawer_width = ctx.screen_cols / 3;

	int ai_panel_width = 0;
	if (ctx.ai_panel_open)
	{
		if (ctx.ai_panel_expanded)
			ai_panel_width = ctx.screen_cols * 45 / 100;
		else
			ai_panel_width = ctx.screen_cols * 32 / 100;
		if (ai_panel_width > ctx.screen_cols / 2) ai_panel_width = ctx.screen_cols / 2;
		if (ai_panel_width < 22) ai_panel_width = 22;
	}
	int edit_window_height = ctx.screen_rows - 3;
	if (edit_window_height < 1) edit_window_height = 1;

	// 1. Linha Superior (Header com botões interactivos)
	if (y == 0)
	{
		int title_w = GetDisplayWidth(" 󱚥 SSCodeIDE ");
		t_text btn_exp = ctx.drawer_open ? "[ 📁 Explorer:ON ] " : "[ 📁 Explorer:OFF ] ";
		t_text btn_chat = ctx.ai_panel_open ? "[ 󱚥 SSBot:ON ] " : "[ 󱚥 SSBot:OFF ] ";
		t_text btn_save = "[ 💾 Save:^S ] ";
		t_text btn_open = "[ 📂 Open:^O ] ";

		int exp_start = title_w;
		int exp_end = exp_start + GetDisplayWidth(btn_exp);
		int chat_start = exp_end;
		int chat_end = chat_start + GetDisplayWidth(btn_chat);
		int save_start = chat_end;
		int save_end = save_start + GetDisplayWidth(btn_save);
		int open_start = save_end;
		int open_end = open_start + GetDisplayWidth(btn_open);

		if (x >= exp_start and x < exp_end)
		{
			ctx.drawer_open = !ctx.drawer_open;
			if (ctx.drawer_open) ctx.LoadDirectoryFiles();
			else if (ctx.current_mode == MODE_FILE_MANAGER) ctx.current_mode = MODE_EDITOR;
			return ;
		}
		if (x >= chat_start and x < chat_end)
		{
			ctx.ai_panel_open = !ctx.ai_panel_open;
			if (ctx.ai_panel_open) ctx.current_mode = MODE_AI_PROMPT;
			else if (ctx.current_mode == MODE_AI_PROMPT) ctx.current_mode = MODE_EDITOR;
			return ;
		}
		if (x >= save_start and x < save_end)
		{
			ctx.SaveFile();
			ctx.AddCliMessage("[System]", "File saved successfully.");
			return ;
		}
		if (x >= open_start and x < open_end)
		{
			ctx.drawer_open = true;
			ctx.LoadDirectoryFiles();
			ctx.current_mode = MODE_FILE_MANAGER;
			return ;
		}
		return ;
	}

	// 2. Linha de Separadores (Tabs)
	if (y == 1)
	{
		if (x >= nav_rail_width)
		{
			int tab_col = nav_rail_width;
			bool clicked_tab = false;
			for (size_t i = 0; i < ctx.open_files.size(); ++i)
			{
				t_text tab_name = ctx.open_files[i].name.empty() ? "*Untitled*" : ctx.open_files[i].name;
				tab_name = FitText(tab_name, 20);
				t_text full_tab = (static_cast<int>(i) == ctx.active_file_idx ? " " : "│ ") + tab_name + (ctx.open_files[i].is_dirty ? " [+]" : "") + " ";
				int tw = GetDisplayWidth(full_tab);
				if (x >= tab_col and x < tab_col + tw)
				{
					ctx.active_file_idx = static_cast<int>(i);
					ctx.current_mode = MODE_EDITOR;
					clicked_tab = true;
					break ;
				}
				tab_col += tw;
			}
			if (!clicked_tab and x >= tab_col and x < tab_col + 11) // [+] New
			{
				ctx.OpenFileBuffer("");
				ctx.current_mode = MODE_EDITOR;
			}
		}
		return ;
	}

	// 3. Nav Rail Lateral (Colunas 0..3)
	if (x >= 0 and x < nav_rail_width)
	{
		if (y == 2 or y == 3) // Botão Explorer
		{
			ctx.drawer_open = !ctx.drawer_open;
			if (ctx.drawer_open) ctx.LoadDirectoryFiles();
			else if (ctx.current_mode == MODE_FILE_MANAGER) ctx.current_mode = MODE_EDITOR;
			return ;
		}
		if (y == 5 or y == 6) // Botão Chat
		{
			ctx.ai_panel_open = !ctx.ai_panel_open;
			if (ctx.ai_panel_open) ctx.current_mode = MODE_AI_PROMPT;
			else if (ctx.current_mode == MODE_AI_PROMPT) ctx.current_mode = MODE_EDITOR;
			return ;
		}
		if (y == 8 or y == 9) // Botão Ajuda
		{
			if (!ctx.ai_panel_open) ctx.ai_panel_open = true;
			ctx.current_mode = MODE_AI_PROMPT;
			ctx.AddCliMessage("You ❯", "/help");
			HandleCliCommand(ctx, "/help");
			return ;
		}
		return ;
	}

	// 4. SSBot Chat Column (Direita)
	if (ctx.ai_panel_open and x >= ctx.screen_cols - ai_panel_width)
	{
		if (y == 2) // Header do SSBot Chat
		{
			if (x >= ctx.screen_cols - 10)
			{
				ctx.ai_panel_open = false;
				if (ctx.current_mode == MODE_AI_PROMPT) ctx.current_mode = MODE_EDITOR;
			}
			else
			{
				ctx.ai_panel_expanded = !ctx.ai_panel_expanded;
			}
			return ;
		}

		ctx.current_mode = MODE_AI_PROMPT;
		int ai_input_y = 2 + edit_window_height - 1;
		if (y == ai_input_y)
		{
			int content_x = (ctx.screen_cols - ai_panel_width) + 1;
			int prompt_start_x = content_x + GetDisplayWidth("SSBot ❯ ");
			int rel_prompt_x = x - prompt_start_x;
			if (rel_prompt_x < 0) ctx.ai_prompt_cursor = 0;
			else
			{
				t_vector inp_chars = SplitUtf8String(ctx.ai_current_input);
				int acc_w = 0;
				int c_idx = 0;
				for (size_t k = 0; k < inp_chars.size(); ++k)
				{
					int cw = GetDisplayWidth(inp_chars[k]);
					if (acc_w + cw / 2 >= rel_prompt_x) break ;
					acc_w += cw;
					c_idx++;
				}
				ctx.ai_prompt_cursor = c_idx;
			}
		}
		return ;
	}

	// 5. Drawer lateral (Explorer)
	if (ctx.drawer_open and x >= nav_rail_width and x < nav_rail_width + drawer_width and y >= 2 and y < 2 + edit_window_height)
	{
		if (y == 2) // Header do Explorer
		{
			ctx.drawer_open = false;
			if (ctx.current_mode == MODE_FILE_MANAGER) ctx.current_mode = MODE_EDITOR;
			return ;
		}

		int item_idx = y - 3;
		if (item_idx >= 0 and item_idx < static_cast<int>(ctx.explorer_items.size()))
		{
			ctx.selected_file_idx = item_idx;
			if (ctx.explorer_items[item_idx].is_dir)
			{
				ctx.ToggleDirectoryExpand(item_idx);
			}
			else
			{
				ctx.OpenFileBuffer(ctx.explorer_items[item_idx].path);
				ctx.current_mode = MODE_EDITOR;
			}
		}
		return ;
	}

	// 6. Editor de Código (Coluna do Meio)
	int code_start_x = nav_rail_width + drawer_width;
	int code_window_width = ctx.screen_cols - nav_rail_width - drawer_width - ai_panel_width;
	int line_number_width = 7;

	if (x >= code_start_x and x < code_start_x + code_window_width and y >= 2 and y < 2 + edit_window_height and !ctx.open_files.empty())
	{
		ctx.current_mode = MODE_EDITOR;
		FileBuffer &cur = ctx.open_files[ctx.active_file_idx];
		int target_y = cur.row_offset + (y - 2);
		if (target_y >= static_cast<int>(cur.lines.size()))
			target_y = cur.lines.empty() ? 0 : static_cast<int>(cur.lines.size() - 1);
		cur.cursor_y = target_y;

		int rel_x = x - (code_start_x + line_number_width);
		if (rel_x < 0) rel_x = 0;
		if (target_y < static_cast<int>(cur.lines.size()))
		{
			t_vector l_chars = SplitUtf8String(cur.lines[target_y]);
			int acc_w = 0;
			int cx = 0;
			for (size_t k = 0; k < l_chars.size(); ++k)
			{
				int cw = (l_chars[k] == "\t") ? 4 : GetDisplayWidth(l_chars[k]);
				if (acc_w + cw / 2 >= rel_x) break ;
				acc_w += cw;
				cx++;
			}
			cur.cursor_x = cx;
		}
		return ;
	}
}

static void	ProcessKeyPress(EditorContext &ctx, RendererEngine &renderer, int c)
{
	if (c == 0 or c == KEY_MOUSE_IGNORE)
		return ;

	if (c == KEY_RESIZE)
	{
		renderer.RefreshScreen(ctx);
		return ;
	}

	// Evento de hover (movimento do rato)
	if (c == KEY_MOUSE_HOVER)
	{
		UpdateHoverTarget(ctx, ctx.hover_x, ctx.hover_y);
		renderer.RefreshScreen(ctx);
		return ;
	}

	// Clique do rato
	if (c == KEY_MOUSE_CLICK)
	{
		HandleMouseClick(ctx, renderer, ctx.mouse_x, ctx.mouse_y);
		return ;
	}

	// Scroll do rato
	if (c == KEY_MOUSE_SCROLL_UP)
	{
		if (ctx.ai_panel_open and ctx.ai_scroll_offset > 0)
			ctx.ai_scroll_offset--;
		else if (!ctx.open_files.empty() and ctx.open_files[ctx.active_file_idx].row_offset > 0)
			ctx.open_files[ctx.active_file_idx].row_offset--;
		return ;
	}
	if (c == KEY_MOUSE_SCROLL_DOWN)
	{
		if (ctx.ai_panel_open and ctx.ai_scroll_offset < static_cast<int>(ctx.ai_response_lines.size()) - 4)
			ctx.ai_scroll_offset++;
		else if (!ctx.open_files.empty())
		{
			FileBuffer &cur = ctx.open_files[ctx.active_file_idx];
			if (cur.row_offset < static_cast<int>(cur.lines.size()) - 1)
				cur.row_offset++;
		}
		return ;
	}

	// ATALHOS GLOBAIS DE COLAPSO / EXPANSÃO
	if (c == CTRL_KEY('b') or c == KEY_F(2))
	{
		ctx.drawer_open = !ctx.drawer_open;
		if (ctx.drawer_open)
		{
			ctx.LoadDirectoryFiles();
			ctx.AddCliMessage("[System]", "Explorer expanded.");
		}
		else
		{
			if (ctx.current_mode == MODE_FILE_MANAGER)
				ctx.current_mode = MODE_EDITOR;
			ctx.AddCliMessage("[System]", "Explorer collapsed.");
		}
		return ;
	}

	if (c == CTRL_KEY('g') or c == KEY_F(3))
	{
		ctx.ai_panel_open = !ctx.ai_panel_open;
		if (ctx.ai_panel_open)
		{
			ctx.current_mode = MODE_AI_PROMPT;
			ctx.ScrollCliToBottom();
		}
		else if (ctx.current_mode == MODE_AI_PROMPT)
		{
			ctx.current_mode = MODE_EDITOR;
		}
		return ;
	}

	// ATALHO DE ALTERNÂNCIA DE FOCO DE JANELA (Tab / F6 / Ctrl+W)
	if (c == KEY_F(6) or c == CTRL_KEY('w'))
	{
		if (ctx.current_mode == MODE_EDITOR)
		{
			if (ctx.ai_panel_open) ctx.current_mode = MODE_AI_PROMPT;
			else if (ctx.drawer_open) ctx.current_mode = MODE_FILE_MANAGER;
		}
		else if (ctx.current_mode == MODE_AI_PROMPT)
		{
			if (ctx.drawer_open) ctx.current_mode = MODE_FILE_MANAGER;
			else ctx.current_mode = MODE_EDITOR;
		}
		else if (ctx.current_mode == MODE_FILE_MANAGER)
		{
			ctx.current_mode = MODE_EDITOR;
		}
		return ;
	}

	if (c == CTRL_KEY('k') or c == KEY_F(4))
	{
		ctx.OpenModelModal();
		return ;
	}

	// MODO MODAL (SELEÇÃO E DOWNLOAD DE MODELOS OLLAMA)
	if (ctx.current_mode == MODE_MODAL or ctx.model_modal_open)
	{
		if (c == 'k' or c == KEY_UP)
		{
			if (ctx.model_modal_selected_idx > 0)
				ctx.model_modal_selected_idx--;
		}
		else if (c == 'j' or c == KEY_DOWN)
		{
			if (ctx.model_modal_selected_idx < static_cast<int>(ctx.model_modal_items.size()) - 1)
				ctx.model_modal_selected_idx++;
		}
		else if (c == 27 or c == 'q' or c == KEY_CANCEL)
		{
			ctx.CloseModelModal();
		}
		else if (c == '\r')
		{
			if (!ctx.model_modal_items.empty() and
				ctx.model_modal_selected_idx < static_cast<int>(ctx.model_modal_items.size()))
			{
				t_text item_str = ctx.model_modal_items[ctx.model_modal_selected_idx];
				t_text model_name = "";
				size_t bracket_close = item_str.find("] ");
				if (bracket_close != t_text::npos)
					model_name = item_str.substr(bracket_close + 2);
				else
					model_name = item_str;

				if (!model_name.empty())
				{
					if (item_str.find("[AVAILABLE]") == 0)
					{
						ctx.AddCliMessage("[System]", "Downloading model '" + model_name + "' via Ollama...");
						renderer.RefreshScreen(ctx);
						t_text pull_cmd = "./ss_ai_bridge.py --pull '" + model_name + "' 2>&1";
						FILE *pipe = popen(pull_cmd.c_str(), "r");
						if (pipe)
						{
							char buffer[512];
							while (fgets(buffer, sizeof(buffer), pipe) != NULL)
							{
								t_text line = SanitizeForUi(buffer);
								if (!line.empty())
									ctx.ai_response_lines.push_back("  " + line);
							}
							pclose(pipe);
						}
						// Defina o modelo como ativo após descarregar
						t_text set_cmd = "./ss_ai_bridge.py --set-model '" + model_name + "' 2>&1";
						FILE *spipe = popen(set_cmd.c_str(), "r");
						if (spipe) pclose(spipe);

						ctx.OpenModelModal();
					}
					else
					{
						t_text set_cmd = "./ss_ai_bridge.py --set-model '" + model_name + "' 2>&1";
						FILE *pipe = popen(set_cmd.c_str(), "r");
						if (pipe)
						{
							char buffer[512];
							while (fgets(buffer, sizeof(buffer), pipe) != NULL)
							{
								t_text line = SanitizeForUi(buffer);
								if (!line.empty())
									ctx.ai_response_lines.push_back("  " + line);
							}
							pclose(pipe);
						}
						ctx.AddCliMessage("[System]", "Active model set to: " + model_name);
						ctx.CloseModelModal();
					}
				}
			}
		}
		return ;
	}

	// MODO SSBOT CLI
	if (ctx.current_mode == MODE_AI_PROMPT)
	{
		if (c == CTRL_KEY('q'))
		{
			ctx.should_quit = true;
			return ;
		}
		if (c == '\r')
		{
			ExecuteAIPrompt(ctx, renderer);
		}
		else if (c == 127 or c == KEY_BACKSPACE or c == 8)
		{
			if (ctx.ai_prompt_cursor > 0)
			{
				t_vector ai_chars = SplitUtf8String(ctx.ai_current_input);
				if (ctx.ai_prompt_cursor <= static_cast<int>(ai_chars.size()))
				{
					ai_chars.erase(ai_chars.begin() + ctx.ai_prompt_cursor - 1);
					ctx.ai_current_input = JoinUtf8Chars(ai_chars);
					ctx.ai_prompt_cursor--;
				}
			}
		}
		else if (c == KEY_DC)
		{
			t_vector ai_chars = SplitUtf8String(ctx.ai_current_input);
			if (ctx.ai_prompt_cursor < static_cast<int>(ai_chars.size()))
			{
				ai_chars.erase(ai_chars.begin() + ctx.ai_prompt_cursor);
				ctx.ai_current_input = JoinUtf8Chars(ai_chars);
			}
		}
		else if (c == KEY_LEFT and ctx.ai_prompt_cursor > 0)
		{
			ctx.ai_prompt_cursor--;
		}
		else if (c == KEY_RIGHT)
		{
			t_vector ai_chars = SplitUtf8String(ctx.ai_current_input);
			if (ctx.ai_prompt_cursor < static_cast<int>(ai_chars.size()))
				ctx.ai_prompt_cursor++;
		}
		else if (c == KEY_HOME)
		{
			ctx.ai_prompt_cursor = 0;
		}
		else if (c == KEY_END)
		{
			ctx.ai_prompt_cursor = SplitUtf8String(ctx.ai_current_input).size();
		}
		else if (c == KEY_UP)
		{
			if (ctx.ai_scroll_offset > 0)
				ctx.ai_scroll_offset--;
		}
		else if (c == KEY_DOWN)
		{
			if (ctx.ai_scroll_offset < static_cast<int>(ctx.ai_response_lines.size()) - 1)
				ctx.ai_scroll_offset++;
		}
		else if (c == KEY_PPAGE)
		{
			ctx.ai_scroll_offset = std::max(0, ctx.ai_scroll_offset - 5);
		}
		else if (c == KEY_NPAGE)
		{
			ctx.ai_scroll_offset = std::min(static_cast<int>(ctx.ai_response_lines.size()) - 1, ctx.ai_scroll_offset + 5);
		}
		else if (c == 27) // ESC para voltar ao editor
		{
			ctx.current_mode = MODE_EDITOR;
		}
		else if (c == CTRL_KEY('c'))
		{
			ctx.ai_current_input = "";
			ctx.ai_prompt_cursor = 0;
		}
		else if (c >= 32 and c < 1000)
		{
			t_text ch_str = CodepointToUtf8(static_cast<wint_t>(c));
			t_vector ai_chars = SplitUtf8String(ctx.ai_current_input);
			if (ctx.ai_prompt_cursor > static_cast<int>(ai_chars.size()))
				ctx.ai_prompt_cursor = ai_chars.size();
			ai_chars.insert(ai_chars.begin() + ctx.ai_prompt_cursor, ch_str);
			ctx.ai_current_input = JoinUtf8Chars(ai_chars);
			ctx.ai_prompt_cursor++;
		}
		return ;
	}

	// MODO GESTOR DE FICHEIROS (EXPLORER FOCADO)
	if (ctx.current_mode == MODE_FILE_MANAGER)
	{
		if (c == 'k' or c == KEY_UP)
		{
			if (ctx.selected_file_idx > 0) ctx.selected_file_idx--;
		}
		else if (c == 'j' or c == KEY_DOWN)
		{
			if (ctx.selected_file_idx < static_cast<int>(ctx.explorer_items.size()) - 1) ctx.selected_file_idx++;
		}
		else if (c == '\r')
		{
			if (!ctx.explorer_items.empty() and ctx.selected_file_idx < static_cast<int>(ctx.explorer_items.size()))
			{
				if (ctx.explorer_items[ctx.selected_file_idx].is_dir)
				{
					ctx.ToggleDirectoryExpand(ctx.selected_file_idx);
				}
				else
				{
					ctx.OpenFileBuffer(ctx.explorer_items[ctx.selected_file_idx].path);
					ctx.current_mode = MODE_EDITOR;
				}
			}
		}
		else if (c == '\t' or c == 27 or c == 'q')
		{
			ctx.current_mode = MODE_EDITOR;
		}
		return ;
	}

	// DEMAIS ATALHOS DO EDITOR
	if (c == '/' and ctx.current_mode == MODE_EDITOR)
	{
		ctx.ai_panel_open = true;
		ctx.current_mode = MODE_AI_PROMPT;
		ctx.ai_current_input = "/";
		ctx.ai_prompt_cursor = 1;
		ctx.ScrollCliToBottom();
		return ;
	}
	if (c == CTRL_KEY('s')) { ctx.SaveFile(); return ; }
	if (c == CTRL_KEY('q')) { ctx.should_quit = true; return ; }
	if (c == CTRL_KEY('o'))
	{
		ctx.LoadDirectoryFiles();
		ctx.current_mode = (ctx.current_mode == MODE_FILE_MANAGER ? MODE_EDITOR : MODE_FILE_MANAGER);
		ctx.drawer_open = (ctx.current_mode == MODE_FILE_MANAGER);
		return ;
	}
	if (c == CTRL_KEY('l'))
	{
		if (!ctx.open_files.empty()) ctx.active_file_idx = (ctx.active_file_idx + 1) % ctx.open_files.size();
		return ;
	}
	if (c == CTRL_KEY('h'))
	{
		if (!ctx.open_files.empty()) ctx.active_file_idx = (ctx.active_file_idx - 1 + ctx.open_files.size()) % ctx.open_files.size();
		return ;
	}

	if (ctx.open_files.empty())
		return ;

	FileBuffer	&cur(ctx.open_files[ctx.active_file_idx]);
	t_vector	chars(SplitUtf8String(cur.lines[cur.cursor_y]));

	if (c == CTRL_KEY('z'))
	{
		if (!cur.undo_stack.empty())
		{
			cur.lines = cur.undo_stack.back();
			cur.undo_stack.pop_back();
			if (cur.cursor_y >= static_cast<int>(cur.lines.size())) cur.cursor_y = cur.lines.size() - 1;
			int max_x = SplitUtf8String(cur.lines[cur.cursor_y]).size();
			if (cur.cursor_x > max_x) cur.cursor_x = max_x;
			if (cur.undo_stack.empty()) cur.is_dirty = false;
		}
		return ;
	}
	if (c == CTRL_KEY('c'))
	{
		ctx.clipboard.clear();
		ctx.clipboard.push_back(cur.lines[cur.cursor_y]);
		return ;
	}
	if (c == CTRL_KEY('v'))
	{
		if (!ctx.clipboard.empty())
		{
			ctx.SaveUndoState(cur);
			for (size_t i = 0; i < ctx.clipboard.size(); ++i)
			{
				cur.lines.insert(cur.lines.begin() + cur.cursor_y + 1, ctx.clipboard[i]);
				cur.cursor_y++;
			}
			cur.cursor_x = 0;
		}
		return ;
	}
	if (c == CTRL_KEY('a'))
	{
		cur.cursor_x = chars.size();
		return ;
	}

	// NAVEGAÇÃO E EDIÇÃO NO BUFFER
	if (c == KEY_LEFT and cur.cursor_x > 0) cur.cursor_x--;
	else if (c == KEY_RIGHT and cur.cursor_x < static_cast<int>(chars.size())) cur.cursor_x++;
	else if (c == KEY_UP and cur.cursor_y > 0)
	{
		cur.cursor_y--;
		int max_x = SplitUtf8String(cur.lines[cur.cursor_y]).size();
		if (cur.cursor_x > max_x) cur.cursor_x = max_x;
	}
	else if (c == KEY_DOWN and cur.cursor_y < static_cast<int>(cur.lines.size() - 1))
	{
		cur.cursor_y++;
		int max_x = SplitUtf8String(cur.lines[cur.cursor_y]).size();
		if (cur.cursor_x > max_x) cur.cursor_x = max_x;
	}
	else if (c == KEY_HOME) cur.cursor_x = 0;
	else if (c == KEY_END) cur.cursor_x = chars.size();
	else if (c == KEY_PPAGE)
	{
		cur.cursor_y = std::max(0, cur.cursor_y - 10);
		int max_x = SplitUtf8String(cur.lines[cur.cursor_y]).size();
		if (cur.cursor_x > max_x) cur.cursor_x = max_x;
	}
	else if (c == KEY_NPAGE)
	{
		cur.cursor_y = std::min(static_cast<int>(cur.lines.size() - 1), cur.cursor_y + 10);
		int max_x = SplitUtf8String(cur.lines[cur.cursor_y]).size();
		if (cur.cursor_x > max_x) cur.cursor_x = max_x;
	}
	else if (c == KEY_DC)
	{
		if (cur.cursor_x < static_cast<int>(chars.size()))
		{
			ctx.SaveUndoState(cur);
			chars.erase(chars.begin() + cur.cursor_x);
			cur.lines[cur.cursor_y] = JoinUtf8Chars(chars);
		}
		else if (cur.cursor_x == static_cast<int>(chars.size()) and cur.cursor_y < static_cast<int>(cur.lines.size() - 1))
		{
			ctx.SaveUndoState(cur);
			cur.lines[cur.cursor_y] += cur.lines[cur.cursor_y + 1];
			cur.lines.erase(cur.lines.begin() + cur.cursor_y + 1);
		}
	}
	else if (c == '\t')
	{
		InsertUtf8Char(ctx, "    ");
	}
	else if (c == '\r')
	{
		ctx.SaveUndoState(cur);
		t_text next_line = JoinUtf8Chars(t_vector(chars.begin() + cur.cursor_x, chars.end()));
		cur.lines[cur.cursor_y] = JoinUtf8Chars(t_vector(chars.begin(), chars.begin() + cur.cursor_x));
		cur.lines.insert(cur.lines.begin() + cur.cursor_y + 1, next_line);
		cur.cursor_y++;
		cur.cursor_x = 0;
	}
	else if (c == 127 or c == KEY_BACKSPACE or c == 8)
	{
		DeleteChar(ctx);
	}
	else if (c >= 32 and c < 1000)
	{
		t_text payload = CodepointToUtf8(static_cast<wint_t>(c));
		InsertUtf8Char(ctx, payload);
	}
}

int	main(int argc, char **argv)
{
	EditorContext	ctx;
	TerminalManager	terminal;
	RendererEngine	renderer;
	int				c(0);

	terminal.Init(ctx);
	renderer.InitColors();
	ctx.LoadDirectoryFiles();

	if (argc >= 2)
	{
		for (int i = 1; i < argc; ++i)
			ctx.OpenFileBuffer(argv[i]);
	}
	else
	{
		ctx.OpenFileBuffer("");
	}

	while (!ctx.should_quit)
	{
		renderer.RefreshScreen(ctx);
		c = terminal.ReadKey(ctx);
		ProcessKeyPress(ctx, renderer, c);
	}

	terminal.Shutdown();
	return (0);
}
