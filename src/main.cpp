/* ************************************************************************** */
/*                                                                            */
/*                                                       ::::::::   ::::::::  */
/*    main.cpp			                               :+:    :+: :+:    :+:  */
/*                                                    +:+        +:+          */
/*    By: Ser Superior <marcioeduine@gmail.com>      +#++:++#++ +#++:++#++    */
/*                                                         +#+        +#+     */
/*    Created: 2026/07/02 10:37:47 by Ser Superior #+#    #+# #+#    #+#      */
/*    Updated: 2026/07/02 10:37:50 by Ser Superior ########   ########        */
/*                                                                            */
/* ************************************************************************** */

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cctype>
#include "sscode.hpp"
#include <unistd.h>

struct termios orig_termios;

static char    CharToLower(char ch)
{
	return (static_cast<char>(std::tolower(static_cast<unsigned char>(ch))));
}

static t_text    TrimCopy(const t_text &s)
{
	size_t    start(0);
	size_t    end(0);

	if (s.empty()) return ("");
	start = s.find_first_not_of(" \t\r\n");
	if (start == t_text::npos) return ("");
	end = s.find_last_not_of(" \t\r\n");
	return (s.substr(start, end - start + 1));
}

static t_text    SanitizeForUi(const t_text &raw)
{
	t_text            clean("");
	size_t            i(0);
	unsigned char    b(0);

	while (i < raw.length())
	{
		b = static_cast<unsigned char>(raw[i]);
		if (b == 0x1B)
		{
			if (i + 1 < raw.length() and raw[i + 1] == '[')
			{
				i += 2;
				while (i < raw.length())
				{
					unsigned char f = static_cast<unsigned char>(raw[i]);
					if (f >= 0x40 and f <= 0x7E) { i++; break ; }
					i++;
				}
				continue ;
			}
			i++;
			continue ;
		}
		if (raw[i] == '\r' or b < 32 or b == 127) { i++; continue ; }
		if (raw[i] == '\t') { clean += "    "; i++; continue ; }
		clean += raw[i];
		i++;
	}
	return (clean);
}

static void    SetAiMessage(EditorContext &ctx, RendererEngine &renderer, const t_text &msg)
{
	int        effective_width(ctx.screen_cols - 4);
	t_text    clean_msg(SanitizeForUi(msg));

	if (effective_width < 10) effective_width = 10;
	ctx.ai_response_lines.clear();
	ctx.ai_scroll_offset = 0;
	renderer.WrapText(clean_msg, effective_width, ctx.ai_response_lines);
	if (ctx.ai_response_lines.empty())
		ctx.ai_response_lines.push_back("(sem conteúdo)");
	ctx.ai_panel_open = true;
}

static bool    IsUtf8LeadByte(unsigned char b) { return (b >= 0xC2 and b <= 0xF4); }
static bool    IsUtf8ContinuationByte(unsigned char b) { return (b >= 0x80 and b <= 0xBF); }

static int    GetUtf8CharLength(unsigned char c)
{
	if ((c & 0x80) == 0) return (1);
	if ((c & 0xE0) == 0xC0) return (2);
	if ((c & 0xF0) == 0xE0) return (3);
	if ((c & 0xF8) == 0xF0) return (4);
	return (1);
}

static t_vector    SplitUtf8String(const t_text &str)
{
	t_vector    chars;
	size_t        i(0);
	int            len(0);

	while (i < str.length())
	{
		len = GetUtf8CharLength(static_cast<unsigned char>(str[i]));
		if (i + len <= str.length()) { chars.push_back(str.substr(i, len)); i += len; }
		else { chars.push_back(str.substr(i, 1)); i++; }
	}
	return (chars);
}

static t_text    JoinUtf8Chars(const t_vector &chars)
{
	t_text    out("");
	for (size_t i(0); i < chars.size(); ++i) out += chars[i];
	return (out);
}

static bool    HandlePromptCommand(EditorContext &ctx, RendererEngine &renderer, const t_text &prompt)
{
	t_text    line(TrimCopy(prompt));
	t_text    cmd("");
	t_text    arg("");
	size_t    sp(0);

	if (line.empty() or line[0] != '/') return (false);
	sp = line.find(' ');
	if (sp == t_text::npos) cmd = line.substr(1);
	else
	{
		cmd = line.substr(1, sp - 1);
		arg = TrimCopy(line.substr(sp + 1));
	}
	
	std::transform(cmd.begin(), cmd.end(), cmd.begin(), CharToLower);

	if (cmd == "editor")
	{
		ctx.current_mode = MODE_EDITOR;
		SetAiMessage(ctx, renderer, "Modo editor activo.");
		return (true);
	}
	if (cmd == "clear")
	{
		t_text clean_cmd("./ss_ai_bridge.py '__CLEAR_CONTEXT__' > /dev/null 2>&1");
		system(clean_cmd.c_str());
		ctx.ai_response_lines.clear();
		ctx.ai_scroll_offset = 0;
		ctx.ai_panel_open = false;
		return (true);
	}
	if (cmd == "files")
	{
		if (ctx.current_mode == MODE_FILE_MANAGER)
		{
			ctx.current_mode = MODE_EDITOR;
			SetAiMessage(ctx, renderer, "Modo ficheiros fechado.");
		}
		else
		{
			ctx.current_mode = MODE_FILE_MANAGER;
			ctx.LoadDirectoryFiles();
			SetAiMessage(ctx, renderer, "Modo ficheiros activo.");
		}
		return (true);
	}
	if (cmd == "save")
	{
		if (not ctx.open_files.empty()) ctx.SaveFile();
		SetAiMessage(ctx, renderer, "Ficheiro guardado.");
		return (true);
	}
	if (cmd == "open")
	{
		if (arg.empty()) { SetAiMessage(ctx, renderer, "Uso: /open <ficheiro>"); return (true); }
		ctx.OpenFileBuffer(arg);
		ctx.current_mode = MODE_EDITOR;
		SetAiMessage(ctx, renderer, "Ficheiro aberto: " + arg);
		return (true);
	}
	if (cmd == "quit" or cmd == "exit")
	{
		ctx.should_quit = true;
		return (true);
	}
	SetAiMessage(ctx, renderer, "Comando desconhecido: /" + cmd);
	return (true);
}

static void    ExecuteAIPrompt(EditorContext &ctx, RendererEngine &renderer)
{
	t_text    escaped_prompt("");
	t_text    cmd("");
	FILE    *pipe(NULL);
	char    buffer[512];
	t_text    full_response("");
	int        effective_width(0);

	if (ctx.ai_current_input.empty() or ctx.ai_current_input.find_first_not_of(" \t\r\n") == t_text::npos)
	{
		ctx.current_mode = MODE_EDITOR;
		return ;
	}
	if (HandlePromptCommand(ctx, renderer, ctx.ai_current_input))
	{
		ctx.ai_current_input = "";
		return ;
	}
	for (size_t i(0); i < ctx.ai_current_input.length(); ++i)
	{
		if (ctx.ai_current_input[i] == '\'') escaped_prompt += "'\\''";
		else escaped_prompt += ctx.ai_current_input[i];
	}
	std::cout << "\x1b[" << ctx.screen_rows << ";1H\x1b[48;2;235;160;0m\x1b[38;2;0;0;0;1m A PROCESSAR IA COM CONTEXTO... \x1b[0m\x1b[K" << std::flush;

	cmd = "./ss_ai_bridge.py '" + escaped_prompt + "' 2>&1";
	pipe = popen(cmd.c_str(), "r");
	if (not pipe) return ;
	while (fgets(buffer, sizeof(buffer), pipe) != NULL)
		full_response += buffer;
	pclose(pipe);

	full_response = SanitizeForUi(full_response);
	if (TrimCopy(full_response).empty())
		full_response = "// Sem resposta do backend IA. Verifica Ollama.";
	effective_width = ctx.screen_cols - 4;
	if (effective_width < 10) effective_width = 10;
	ctx.ai_response_lines.clear();
	ctx.ai_scroll_offset = 0;
	renderer.WrapText(full_response, effective_width, ctx.ai_response_lines);
	if (ctx.ai_response_lines.empty())
		ctx.ai_response_lines.push_back("(sem conteúdo)");
	ctx.ai_panel_open = true;
	ctx.current_mode = MODE_EDITOR;
	ctx.ai_current_input = "";
}

static void    InsertUtf8Char(EditorContext &ctx, const t_text &utf8_char)
{
	FileBuffer    &cur(ctx.open_files[ctx.active_file_idx]);
	t_vector    chars(SplitUtf8String(cur.lines[cur.cursor_y]));

	ctx.SaveUndoState(cur);
	chars.insert(chars.begin() + cur.cursor_x, utf8_char);
	cur.lines[cur.cursor_y] = JoinUtf8Chars(chars);
	cur.cursor_x++;
}

static void    DeleteChar(EditorContext &ctx)
{
	FileBuffer    &cur(ctx.open_files[ctx.active_file_idx]);
	t_vector    chars(SplitUtf8String(cur.lines[cur.cursor_y]));

	ctx.SaveUndoState(cur);
	if (cur.cursor_x > 0)
	{
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

static void    ProcessKeyPress(EditorContext &ctx, RendererEngine &renderer, unsigned int c)
{
	char            next_b(0);
	t_text            utf8_payload("");
	int                extra_bytes(0);
	bool            valid_utf8(true);
	unsigned char    cont_byte(0);

	if (c == MOUSE_EVENT_IGNORE) return ;

	// Interceção de scroll do rato e setas globais se o painel da IA estiver aberto
	if (ctx.ai_panel_open)
	{
		if (c == MOUSE_SCROLL_UP)
		{
			if (ctx.ai_scroll_offset > 0) ctx.ai_scroll_offset--;
			return ;
		}
		if (c == MOUSE_SCROLL_DOWN)
		{
			if (ctx.ai_scroll_offset < static_cast<int>(ctx.ai_response_lines.size()) - 4) ctx.ai_scroll_offset++;
			return ;
		}
	}

	if (c == '/' and ctx.current_mode == MODE_EDITOR) { ctx.current_mode = MODE_AI_PROMPT; ctx.ai_current_input = "/"; return ; }

	if (ctx.current_mode == MODE_AI_PROMPT)
	{
		if (c == '\r') ExecuteAIPrompt(ctx, renderer);
		else if (c == 127)
		{
			if (not ctx.ai_current_input.empty())
			{
				t_vector ai_chars(SplitUtf8String(ctx.ai_current_input));
				if (not ai_chars.empty()) { ai_chars.pop_back(); ctx.ai_current_input = JoinUtf8Chars(ai_chars); }
			}
		}
		else if (c == '\x1b') ctx.current_mode = MODE_EDITOR;
		else if (c >= 32 and c <= 255)
		{
			unsigned char first_byte = static_cast<unsigned char>(c);
			if (first_byte >= 32 and first_byte <= 126) { ctx.ai_current_input += static_cast<char>(first_byte); return ; }
			if (not IsUtf8LeadByte(first_byte)) return ;
			utf8_payload += static_cast<char>(first_byte);
			extra_bytes = GetUtf8CharLength(first_byte) - 1;
			for (int b(0); b < extra_bytes; ++b)
			{
				if (read(STDIN_FILENO, &next_b, 1) != 1) { valid_utf8 = false; break ; }
				cont_byte = static_cast<unsigned char>(next_b);
				if (not IsUtf8ContinuationByte(cont_byte)) { valid_utf8 = false; break ; }
				utf8_payload += next_b;
			}
			if (valid_utf8) ctx.ai_current_input += utf8_payload;
		}
		return ;
	}

	if (ctx.current_mode == MODE_FILE_MANAGER)
	{
		FileBuffer &cur(ctx.open_files[ctx.active_file_idx]);
		if (c == 'k' or c == ARROW_UP)
		{
			if (cur.cursor_y > 0) cur.cursor_y--;
		}
		else if (c == 'j' or c == ARROW_DOWN)
		{
			if (cur.cursor_y < static_cast<int>(cur.lines.size() - 1)) cur.cursor_y++;
		}
		else if (c == '\r')
		{
			if (not cur.lines.empty() and cur.cursor_y < static_cast<int>(cur.lines.size()))
			{
				t_text selected_line = cur.lines[cur.cursor_y];
				if (not selected_line.empty())
				{
					ctx.OpenFileBuffer(selected_line);
					ctx.current_mode = MODE_EDITOR;
				}
			}
		}
		else if (c == '\x1b' or c == 'q')
		{
			ctx.current_mode = MODE_EDITOR;
		}
		return ;
	}

	if (c == CTRL_KEY('l')) { if (not ctx.open_files.empty()) ctx.active_file_idx = (ctx.active_file_idx + 1) % ctx.open_files.size(); return ; }
	if (c == CTRL_KEY('h')) { if (not ctx.open_files.empty()) ctx.active_file_idx = (ctx.active_file_idx - 1 + ctx.open_files.size()) % ctx.open_files.size(); return ; }

	FileBuffer    &cur(ctx.open_files[ctx.active_file_idx]);
	t_vector    chars(SplitUtf8String(cur.lines[cur.cursor_y]));

	if (c == CTRL_KEY('z'))
	{
		if (not cur.undo_stack.empty())
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
	if (c == CTRL_KEY('c')) { ctx.clipboard.clear(); ctx.clipboard.push_back(cur.lines[cur.cursor_y]); return ; }
	if (c == CTRL_KEY('v'))
	{
		if (not ctx.clipboard.empty())
		{
			ctx.SaveUndoState(cur);
			for (size_t i(0); i < ctx.clipboard.size(); ++i)
			{
				cur.lines.insert(cur.lines.begin() + cur.cursor_y + 1, ctx.clipboard[i]);
				cur.cursor_y++;
			}
			cur.cursor_x = 0;
		}
		return ;
	}
	if (c == ARROW_LEFT and cur.cursor_x > 0) cur.cursor_x--;
	else if (c == ARROW_RIGHT and cur.cursor_x < static_cast<int>(chars.size())) cur.cursor_x++;
	else if (c == ARROW_UP and cur.cursor_y > 0)
	{
		cur.cursor_y--;
		int max_x = SplitUtf8String(cur.lines[cur.cursor_y]).size();
		if (cur.cursor_x > max_x) cur.cursor_x = max_x;
	}
	else if (c == ARROW_DOWN and cur.cursor_y < static_cast<int>(cur.lines.size() - 1))
	{
		cur.cursor_y++;
		int max_x = SplitUtf8String(cur.lines[cur.cursor_y]).size();
		if (cur.cursor_x > max_x) cur.cursor_x = max_x;
	}
	else if (c == HOME_KEY) cur.cursor_x = 0;
	else if (c == END_KEY) cur.cursor_x = chars.size();
	else if (c == DEL_KEY)
	{
		if (cur.cursor_x < static_cast<int>(chars.size())) { ctx.SaveUndoState(cur); chars.erase(chars.begin() + cur.cursor_x); cur.lines[cur.cursor_y] = JoinUtf8Chars(chars); }
		else if (cur.cursor_x == static_cast<int>(chars.size()) and cur.cursor_y < static_cast<int>(cur.lines.size() - 1))
		{
			ctx.SaveUndoState(cur);
			cur.lines[cur.cursor_y] += cur.lines[cur.cursor_y + 1];
			cur.lines.erase(cur.lines.begin() + cur.cursor_y + 1);
		}
	}
	else if (c == '\t') { InsertUtf8Char(ctx, "\t"); }
	else if (c == '\r')
	{
		ctx.SaveUndoState(cur);
		t_text next_line = JoinUtf8Chars(t_vector(chars.begin() + cur.cursor_x, chars.end()));
		cur.lines[cur.cursor_y] = JoinUtf8Chars(t_vector(chars.begin(), chars.begin() + cur.cursor_x));
		cur.lines.insert(cur.lines.begin() + cur.cursor_y + 1, next_line);
		cur.cursor_y++;
		cur.cursor_x = 0;
	}
	else if (c == 127) DeleteChar(ctx);
	else if (c >= 32)
	{
		unsigned char first_byte = static_cast<unsigned char>(c);
		if (first_byte >= 32 and first_byte <= 126) { utf8_payload += static_cast<char>(first_byte); InsertUtf8Char(ctx, utf8_payload); return ; }
		if (not IsUtf8LeadByte(first_byte)) return ;
		utf8_payload += static_cast<char>(first_byte);
		extra_bytes = GetUtf8CharLength(first_byte) - 1;
		for (int b(0); b < extra_bytes; ++b)
		{
			if (read(STDIN_FILENO, &next_b, 1) != 1) { valid_utf8 = false; break ; }
			cont_byte = static_cast<unsigned char>(next_b);
			if (not IsUtf8ContinuationByte(cont_byte)) { valid_utf8 = false; break ; }
			utf8_payload += next_b;
		}
		if (valid_utf8) InsertUtf8Char(ctx, utf8_payload);
	}
}

int    main(int argc, char **argv)
{
	EditorContext    ctx;
	TerminalManager    terminal;
	RendererEngine    renderer;
	unsigned int    c(0);

	terminal.EnableRawMode(ctx);
	if (argc >= 2)
	{
		for (int i(1); i < argc; ++i)
			ctx.OpenFileBuffer(argv[i]);
	}
	else
		ctx.OpenFileBuffer("");

	while (true)
	{
		renderer.RefreshScreen(ctx);
		c = terminal.ReadKey();
		ProcessKeyPress(ctx, renderer, c);
		if (c == CTRL_KEY('q') or ctx.should_quit)
			break ;
	}
	terminal.DisableRawMode();
	return (0);
}
