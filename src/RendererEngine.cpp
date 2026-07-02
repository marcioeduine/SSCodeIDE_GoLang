/* ************************************************************************** */
/*                                                                            */
/*                                                       ::::::::   ::::::::  */
/*    RendererEngine.cpp                               :+:    :+: :+:    :+:  */
/*                                                    +:+        +:+          */
/*    By: Ser Superior <marcioeduine@gmail.com>      +#++:++#++ +#++:++#++    */
/*                                                         +#+        +#+     */
/*    Created: 2026/07/02 10:37:47 by Ser Superior #+#    #+# #+#    #+#      */
/*    Updated: 2026/07/02 10:37:50 by Ser Superior ########   ########        */
/*                                                                            */
/* ************************************************************************** */

#include <iostream>
#include <sstream>
#include "sscode.hpp"

static int	GetUtf8CharLength(unsigned char c)
{
	if ((c & 0x80) == 0) return (1);
	if ((c & 0xE0) == 0xC0) return (2);
	if ((c & 0xF0) == 0xE0) return (3);
	if ((c & 0xF8) == 0xF0) return (4);
	return (1);
}

static t_vector	SplitUtf8String(const t_text &str)
{
	t_vector	chars;
	size_t		i(0);
	int			len(0);

	while (i < str.length())
	{
		len = GetUtf8CharLength(static_cast<unsigned char>(str[i]));
		if (i + len <= str.length())
		{
			chars.push_back(str.substr(i, len));
			i += len;
		}
		else
		{
			chars.push_back(str.substr(i, 1));
			i++;
		}
	}
	return (chars);
}

static size_t	GetUtf8Width(const t_text &str)
{
	size_t		count(0);
	size_t		i(0);

	while (i < str.length())
	{
		i += GetUtf8CharLength(static_cast<unsigned char>(str[i]));
		count++;
	}
	return (count);
}

static bool	IsInList(const t_text &word, const char *list[], int size)
{
	for (int i(0); i < size; ++i)
	{
		if (word == list[i])
			return (true);
	}
	return (false);
}

static t_text	FitText(const t_text &text, int width)
{
	if (width <= 0) return ("");
	if (static_cast<int>(text.length()) <= width) return (text);
	if (width <= 3) return (text.substr(0, width));
	return (text.substr(0, width - 3) + "...");
}

static t_text	BuildHorizontalLine(int width)
{
	t_text	line("");
	for (int i(0); i < width; ++i)
		line += "─";
	return (line);
}

static t_text	IntToString(int value)
{
	std::ostringstream	oss;
	oss << value;
	return (oss.str());
}

void	RendererEngine::WrapText(const t_text &text, int max_width, t_vector &output)
{
	t_vector	words;
	t_text		current_word("");
	t_text		current_line("");

	for (size_t i(0); i < text.length(); ++i)
	{
		if (text[i] == '\n')
		{
			if (not current_word.empty()) { words.push_back(current_word); current_word = ""; }
			words.push_back("\n");
		}
		else if (text[i] == ' ')
		{
			if (not current_word.empty()) { words.push_back(current_word); current_word = ""; }
			words.push_back(" ");
		}
		else
			current_word += text[i];
	}
	if (not current_word.empty())
		words.push_back(current_word);

	for (size_t i(0); i < words.size(); ++i)
	{
		if (words[i] == "\n")
		{
			output.push_back(current_line);
			current_line = "";
		}
		else
		{
			if (current_line.length() + words[i].length() > static_cast<size_t>(max_width))
			{
				if (not current_line.empty())
					output.push_back(current_line);
				current_line = (words[i] == " ") ? "" : words[i];
			}
			else
				current_line += words[i];
		}
	}
	if (not current_line.empty())
		output.push_back(current_line);
}

void	RendererEngine::RenderHighlightedLine(const t_text &line, int max_width)
{
	const char	*keywords[] = { "switch", "if", "else", "while", "for", "break", "return", "define", "include", "and", "or", "not", "xor" };
	const char	*datatypes[] = { "int", "char", "void", "struct", "class", "bool", "char32_t", "size_t" };
	t_vector	chars(SplitUtf8String(line));
	t_text		current_word("");
	bool		in_string(false);
	int			current_printed_width(0);
	t_text		c("");

	if (not chars.empty() and chars[0] == "#")
	{
		if (line.length() > static_cast<size_t>(max_width))
			std::cout << RGB_PREPROC << line.substr(0, max_width) << RGB_RESET;
		else
			std::cout << RGB_PREPROC << line << RGB_RESET;
		return ;
	}
	for (size_t j(0); j < chars.size(); ++j)
	{
		if (current_printed_width >= max_width)
			break ;
		c = chars[j];
		if (not in_string and j + 1 < chars.size() and c == "/" and chars[j + 1] == "/")
		{
			std::cout << RGB_COMMENT;
			for (size_t rem(j); rem < chars.size() and current_printed_width < max_width; ++rem)
			{
				std::cout << chars[rem];
				current_printed_width++;
			}
			std::cout << RGB_RESET;
			return ;
		}
		if (c == "\"")
		{
			if (in_string)
			{
				current_word += c;
				std::cout << RGB_STRING << current_word << RGB_RESET;
				current_printed_width += SplitUtf8String(current_word).size();
				current_word = "";
				in_string = false;
			}
			else
			{
				std::cout << RGB_TEXT << current_word << RGB_RESET;
				current_printed_width += SplitUtf8String(current_word).size();
				current_word = "\"";
				in_string = true;
			}
			continue ;
		}
		if (in_string)
		{
			current_word += c;
			continue ;
		}
		if (c == " " or c == "(" or c == ")" or c == "{" or c == "}" or c == ";" or c == "," or c == "<" or c == ">" or c == "\t")
		{
			if (IsInList(current_word, keywords, 13)) std::cout << RGB_KEYWORD << current_word << RGB_RESET;
			else if (IsInList(current_word, datatypes, 8)) std::cout << RGB_DATATYPE << current_word << RGB_RESET;
			else if (current_word.find_first_not_of("0123456789") == t_text::npos and not current_word.empty())
				std::cout << RGB_NUMBER << current_word << RGB_RESET;
			else std::cout << RGB_TEXT << current_word << RGB_RESET;

			current_printed_width += SplitUtf8String(current_word).size();
			if (c == "\t")
			{
				std::cout << "    ";
				current_printed_width += 4;
			}
			else
			{
				std::cout << RGB_TEXT << c << RGB_RESET;
				current_printed_width++;
			}
			current_word = "";
		}
		else
			current_word += c;
	}
	if (current_printed_width < max_width)
	{
		if (IsInList(current_word, keywords, 13)) std::cout << RGB_KEYWORD << current_word << RGB_RESET;
		else if (IsInList(current_word, datatypes, 8)) std::cout << RGB_DATATYPE << current_word << RGB_RESET;
		else std::cout << RGB_TEXT << current_word << RGB_RESET;
		current_printed_width += SplitUtf8String(current_word).size();
	}
	for (int fill(current_printed_width); fill < max_width; ++fill)
		std::cout << " ";
}

void	RendererEngine::RefreshScreen(EditorContext &ctx)
{
	t_text	top_left(" SSCodeIDE ");
	t_text	top_buttons(" [Save:^S] [Open:^O] [AI:^G] ");
	t_text	top_right(ctx.ai_panel_open ? "AI CHAT ON" : "AI CHAT OFF");
	int		top_fill(0);
	t_text	dirty_flag("");
	t_text	tab_name("");

	std::cout << "\x1b[?25l\x1b[H" << RGB_BG_DARK;

	top_fill = ctx.screen_cols - static_cast<int>(top_left.length() + top_buttons.length() + top_right.length());
	if (top_fill < 1) top_fill = 1;
	std::cout << RGB_HEADER_BG << RGB_HEADER_ACCENT << top_left << RGB_TEXT << top_buttons;
	for (int t(0); t < top_fill; ++t)
		std::cout << " ";
	std::cout << RGB_STATUS_TEXT << top_right << RGB_RESET << "\x1b[K\r\n";

	std::cout << RGB_TAB_BG << " ";
	for (size_t i(0); i < ctx.open_files.size(); ++i)
	{
		dirty_flag = ctx.open_files[i].is_dirty ? " \x1b[31m[+]\x1b[0m" : "";
		tab_name = ctx.open_files[i].name.empty() ? "*Sem Nome*" : ctx.open_files[i].name;
		tab_name = FitText(tab_name, 24);
		if (static_cast<int>(i) == ctx.active_file_idx)
			std::cout << RGB_TAB_ACT << " " << tab_name << dirty_flag << " " << RGB_TAB_BG;
		else
			std::cout << RGB_GUTTER << "│ " << RGB_TEXT << tab_name << dirty_flag << " ";
	}
	std::cout << "\x1b[K" << RGB_RESET << "\r\n";

	FileBuffer	&cur(ctx.open_files[ctx.active_file_idx]);
	int			visual_cursor_x(0);
	int			line_number_width(7);
	int			ai_panel_height(ctx.ai_panel_open ? 8 : 0);
	int			edit_window_height(ctx.screen_rows - 4 - ai_panel_height);
	int			effective_code_text_width(ctx.screen_cols - line_number_width - 1);

	if (effective_code_text_width < 8) effective_code_text_width = 8;

	int			total_lines(cur.lines.size());
	int			scroll_handle_size((edit_window_height * edit_window_height) / (total_lines > 0 ? total_lines : 1));
	if (scroll_handle_size < 1) scroll_handle_size = 1;
	if (scroll_handle_size > edit_window_height) scroll_handle_size = edit_window_height;
	int			scroll_handle_pos(0);
	if (total_lines > edit_window_height)
		scroll_handle_pos = (cur.row_offset * (edit_window_height - scroll_handle_size)) / (total_lines - edit_window_height);

	for (int i(0); i < edit_window_height; ++i)
	{
		int		file_line_idx(i + cur.row_offset);
		bool	is_handle(i >= scroll_handle_pos and i < scroll_handle_pos + scroll_handle_size);
		t_text	scroll_char(is_handle ? "█" : "│");

		if (file_line_idx >= static_cast<int>(cur.lines.size()))
		{
			std::cout << RGB_GUTTER << "~\x1b[0m";
			for (int space(1); space < effective_code_text_width + line_number_width; ++space) std::cout << " ";
			std::cout << RGB_GUTTER << scroll_char << RGB_RESET;
		}
		else
		{
			std::cout << RGB_GUTTER;
			if (file_line_idx + 1 < 10) std::cout << "   " << (file_line_idx + 1) << " │ ";
			else if (file_line_idx + 1 < 100) std::cout << "  " << (file_line_idx + 1) << " │ ";
			else std::cout << " " << (file_line_idx + 1) << " │ ";
			std::cout << RGB_RESET;

			if (file_line_idx == cur.cursor_y)
			{
				t_vector l_chars(SplitUtf8String(cur.lines[file_line_idx]));
				visual_cursor_x = line_number_width;
				for (int k(0); k < cur.cursor_x and k < static_cast<int>(l_chars.size()); ++k)
				{
					if (l_chars[k] == "\t") visual_cursor_x += 4; else visual_cursor_x += 1;
				}
			}
			RenderHighlightedLine(cur.lines[file_line_idx], effective_code_text_width);
			std::cout << RGB_GUTTER << scroll_char << RGB_RESET;
		}
		std::cout << "\x1b[K\r\n";
	}

	// Renderização do Painel do Chat IA com Suporte a Scroll Histórico
	if (ctx.ai_panel_open)
	{
		std::cout << RGB_GUTTER << BuildHorizontalLine(ctx.screen_cols) << RGB_RESET << "\r\n";
		int chat_view_lines = ai_panel_height - 1;
		
		for (int i(0); i < chat_view_lines; ++i)
		{
			std::cout << RGB_AI_PANEL << " ";
			if (i == 0)
			{
				t_text panel_title("SSCodeIDE - Chat Co-Pilot (Use Mousewheel/Setas para Scroll)");
				std::cout << RGB_HEADER_ACCENT << panel_title;
				for (int fill(panel_title.length()); fill < ctx.screen_cols - 2; ++fill) std::cout << " ";
			}
			else
			{
				int data_idx = (i - 1) + ctx.ai_scroll_offset;
				if (data_idx < static_cast<int>(ctx.ai_response_lines.size()))
				{
					t_text ai_line(ctx.ai_response_lines[data_idx]);
					std::cout << RGB_AI_TEXT << ai_line;
					int text_w = GetUtf8Width(ai_line);
					for (int fill(text_w); fill < ctx.screen_cols - 2; ++fill) std::cout << " ";
				}
				else
				{
					for (int fill(0); fill < ctx.screen_cols - 2; ++fill) std::cout << " ";
				}
			}
			std::cout << " " << RGB_RESET << "\x1b[K\r\n";
		}
	}

	if (ctx.current_mode != MODE_AI_PROMPT)
	{
		std::cout << RGB_GUTTER << BuildHorizontalLine(ctx.screen_cols) << RGB_RESET << "\r\n";
		t_text file_name(cur.name.empty() ? "[Sem Nome]" : cur.name);
		t_text mode(ctx.current_mode == MODE_FILE_MANAGER ? "FILES" : "NORMAL");
		t_text pos("Ln " + IntToString(cur.cursor_y + 1) + ", Col " + IntToString(cur.cursor_x + 1));
		t_text hints("/editor /files /save /open /clear /quit");
		t_text status(" " + mode + " • " + file_name + (cur.is_dirty ? " [+]" : "") + " • " + pos + " • " + hints);
		status = FitText(status, ctx.screen_cols);
		std::cout << RGB_STATUS_BG << RGB_STATUS_TEXT << status << RGB_RESET << "\x1b[K";
	}

	if (ctx.current_mode == MODE_AI_PROMPT)
	{
		int		box_width(60);
		int		start_y(ctx.screen_rows / 2 - 2);
		if (start_y < 1) start_y = 1;
		int		start_x((ctx.screen_cols - box_width) / 2);
		if (start_x < 1) start_x = 1;

		t_text title(" PROMPT IA ");
		std::cout << "\x1b[" << start_y << ";" << start_x << "H" << RGB_HEADER_BG << RGB_HEADER_ACCENT << "╭" << title << BuildHorizontalLine(box_width - 2 - title.length()) << "╮" << RGB_RESET;
		
		t_text content(" ❯ " + ctx.ai_current_input);
		t_text fitted_content(FitText(content, box_width - 2));
		std::cout << "\x1b[" << start_y + 1 << ";" << start_x << "H" << RGB_HEADER_BG << RGB_HEADER_ACCENT << "│" << RGB_TEXT << fitted_content;
		
		int fill = box_width - 2 - GetUtf8Width(fitted_content);
		if (fill < 0) fill = 0;
		for (int f(0); f < fill; ++f) std::cout << " ";
		std::cout << RGB_HEADER_ACCENT << "│" << RGB_RESET;
		std::cout << "\x1b[" << start_y + 2 << ";" << start_x << "H" << RGB_HEADER_BG << RGB_HEADER_ACCENT << "╰" << BuildHorizontalLine(box_width - 2) << "╯" << RGB_RESET;

		if (not ctx.ai_current_input.empty() and ctx.ai_current_input[0] == '/')
		{
			const char	*available_cmds[] = { "/editor", "/files", "/save", "/open ", "/clear", "/quit" };
			int			num_cmds = 6;
			int			current_list_y(start_y + 3);

			for (int cmd_idx(0); cmd_idx < num_cmds; ++cmd_idx)
			{
				t_text candidate(available_cmds[cmd_idx]);
				if (candidate.find(ctx.ai_current_input) == 0)
				{
					std::cout << "\x1b[" << current_list_y << ";" << start_x << "H\x1b[K"
							  << "\x1b[38;2;235;160;0m  • " << RGB_TEXT << candidate << RGB_RESET;
					current_list_y++;
				}
			}
		}

		int prompt_cursor_x = start_x + 1 + GetUtf8Width(" ❯ ") + GetUtf8Width(ctx.ai_current_input);
		if (prompt_cursor_x > start_x + box_width - 2) prompt_cursor_x = start_x + box_width - 2;
		std::cout << "\x1b[" << start_y + 1 << ";" << prompt_cursor_x << "H\x1b[?25h" << std::flush;
	}
	else
	{
		int physical_cursor_y((cur.cursor_y - cur.row_offset) + 3); 
		int physical_cursor_x(visual_cursor_x + 1);
		if (physical_cursor_y < 3) physical_cursor_y = 3;
		if (physical_cursor_y > ctx.screen_rows - 1) physical_cursor_y = ctx.screen_rows - 1;
		if (physical_cursor_x < 1) physical_cursor_x = 1;
		if (physical_cursor_x > ctx.screen_cols) physical_cursor_x = ctx.screen_cols;
		std::cout << "\x1b[" << physical_cursor_y << ";" << physical_cursor_x << "H" << "\x1b[?25h" << std::flush;
	}
}
