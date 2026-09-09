/* ************************************************************************** */
/*                                                                            */
/*                                                       ::::::::   ::::::::  */
/*    RendererEngine.cpp                               :+:    :+: :+:    :+:  */
/*                                                    +:+        +:+          */
/*    By: Ser Superior <marcioeduine@gmail.com>      +#++:++#++ +#++:++#++    */
/*                                                         +#+        +#+     */
/*    Created: 2026/07/02 10:37:47 by Ser Superior #+#    #+# #+#    #+#      */
/*    Updated: 2026/09/09 16:50:00 by Ser Superior ########   ########        */
/*                                                                            */
/* ************************************************************************** */

#include "sscode.hpp"
#include <algorithm>
#include <sstream>

static const char	*g_keywords[] = {
	"switch", "case", "default", "if", "else", "while", "for", "do",
	"break", "continue", "return", "class", "struct", "enum", "union",
	"public", "private", "protected", "virtual", "override", "final",
	"template", "typename", "namespace", "using", "try", "catch", "throw",
	"new", "delete", "this", "const", "static", "inline", "explicit",
	"friend", "sizeof", "typedef", "and", "or", "not", "xor"
};
static const int	g_num_keywords = sizeof(g_keywords) / sizeof(g_keywords[0]);

static const char	*g_datatypes[] = {
	"int", "char", "void", "bool", "float", "double", "short", "long",
	"unsigned", "signed", "size_t", "ssize_t", "uint8_t", "uint16_t",
	"uint32_t", "uint64_t", "int8_t", "int16_t", "int32_t", "int64_t",
	"char32_t", "wchar_t", "auto", "t_text", "t_vector", "FileBuffer",
	"EditorContext", "TerminalManager", "RendererEngine"
};
static const int	g_num_datatypes = sizeof(g_datatypes) / sizeof(g_datatypes[0]);

static bool	IsInList(const t_text &word, const char *list[], int size)
{
	for (int i = 0; i < size; ++i)
	{
		if (word == list[i])
			return (true);
	}
	return (false);
}

static t_text	BuildHorizontalLine(int width, const char *ch = "─")
{
	t_text	line = "";
	for (int i = 0; i < width; ++i)
		line += ch;
	return (line);
}

void	RendererEngine::InitColors(void)
{
	if (!has_colors())
		return ;

	start_color();
	use_default_colors();

	short	bg = -1;

	if (COLORS >= 256)
	{
		init_pair(CP_DEFAULT, 255, bg);
		init_pair(CP_HEADER, 254, 236);
		init_pair(CP_HEADER_ACCENT, 141, 236);
		init_pair(CP_TAB_ACTIVE, 255, 60);
		init_pair(CP_TAB_INACTIVE, 247, 235);
		init_pair(CP_TAB_DIRTY, 214, 235);
		init_pair(CP_GUTTER, 243, bg);
		init_pair(CP_GUTTER_ACTIVE, 141, bg);
		init_pair(CP_KEYWORD, 141, bg);
		init_pair(CP_DATATYPE, 183, bg);
		init_pair(CP_STRING, 119, bg);
		init_pair(CP_COMMENT, 244, bg);
		init_pair(CP_PREPROC, 213, bg);
		init_pair(CP_NUMBER, 215, bg);
		init_pair(CP_STATUS, 252, 236);
		init_pair(CP_STATUS_ACCENT, 16, 141);
		init_pair(CP_AI_HEADER, 141, 234);
		init_pair(CP_AI_PANEL, 252, 234);
		init_pair(CP_AI_TEXT, 141, 234);
		init_pair(CP_AI_USER, 214, 234);
		init_pair(CP_AI_SYS, 119, 234);
		init_pair(CP_PROMPT_BOX, 141, 235);
		init_pair(CP_PROMPT_ACCENT, 255, 235);
		init_pair(CP_DRAWER_DIR, 183, bg);
		init_pair(CP_DRAWER_FILE, 252, bg);
		init_pair(CP_DRAWER_SEL, 16, 141);
		init_pair(CP_SCROLLBAR_HANDLE, 255, 141);
		init_pair(CP_SCROLLBAR_TRACK, 238, bg);
		init_pair(CP_NAV_RAIL_BG, 245, 234);
		init_pair(CP_NAV_RAIL_ACTIVE, 255, 238);
		init_pair(CP_NAV_RAIL_INACTIVE, 242, 234);
		init_pair(CP_NAV_RAIL_INDICATOR, 141, 234);
		init_pair(CP_BTN_ACCENT, 255, 60);
		init_pair(CP_HOVER_ACCENT, 231, 60);
		init_pair(CP_MODAL_BG, 252, 236);
		init_pair(CP_MODAL_BORDER, 141, 236);
		init_pair(CP_MODAL_SEL, 16, 141);
	}
	else
	{
		init_pair(CP_DEFAULT, COLOR_WHITE, bg);
		init_pair(CP_HEADER, COLOR_WHITE, COLOR_MAGENTA);
		init_pair(CP_HEADER_ACCENT, COLOR_MAGENTA, COLOR_BLACK);
		init_pair(CP_TAB_ACTIVE, COLOR_WHITE, COLOR_MAGENTA);
		init_pair(CP_TAB_INACTIVE, COLOR_WHITE, COLOR_BLACK);
		init_pair(CP_TAB_DIRTY, COLOR_YELLOW, COLOR_BLACK);
		init_pair(CP_GUTTER, COLOR_CYAN, bg);
		init_pair(CP_GUTTER_ACTIVE, COLOR_MAGENTA, bg);
		init_pair(CP_KEYWORD, COLOR_MAGENTA, bg);
		init_pair(CP_DATATYPE, COLOR_CYAN, bg);
		init_pair(CP_STRING, COLOR_GREEN, bg);
		init_pair(CP_COMMENT, COLOR_BLUE, bg);
		init_pair(CP_PREPROC, COLOR_MAGENTA, bg);
		init_pair(CP_NUMBER, COLOR_YELLOW, bg);
		init_pair(CP_STATUS, COLOR_BLACK, COLOR_WHITE);
		init_pair(CP_STATUS_ACCENT, COLOR_BLACK, COLOR_MAGENTA);
		init_pair(CP_AI_HEADER, COLOR_MAGENTA, COLOR_BLACK);
		init_pair(CP_AI_PANEL, COLOR_WHITE, COLOR_BLACK);
		init_pair(CP_AI_TEXT, COLOR_CYAN, COLOR_BLACK);
		init_pair(CP_AI_USER, COLOR_YELLOW, COLOR_BLACK);
		init_pair(CP_AI_SYS, COLOR_GREEN, COLOR_BLACK);
		init_pair(CP_PROMPT_BOX, COLOR_MAGENTA, COLOR_BLACK);
		init_pair(CP_PROMPT_ACCENT, COLOR_WHITE, COLOR_BLACK);
		init_pair(CP_DRAWER_DIR, COLOR_MAGENTA, bg);
		init_pair(CP_DRAWER_FILE, COLOR_WHITE, bg);
		init_pair(CP_DRAWER_SEL, COLOR_BLACK, COLOR_MAGENTA);
		init_pair(CP_SCROLLBAR_HANDLE, COLOR_WHITE, COLOR_WHITE);
		init_pair(CP_SCROLLBAR_TRACK, COLOR_CYAN, bg);
		init_pair(CP_NAV_RAIL_BG, COLOR_WHITE, COLOR_BLACK);
		init_pair(CP_NAV_RAIL_ACTIVE, COLOR_YELLOW, COLOR_MAGENTA);
		init_pair(CP_NAV_RAIL_INACTIVE, COLOR_CYAN, COLOR_BLACK);
		init_pair(CP_NAV_RAIL_INDICATOR, COLOR_MAGENTA, COLOR_BLACK);
		init_pair(CP_BTN_ACCENT, COLOR_WHITE, COLOR_MAGENTA);
		init_pair(CP_HOVER_ACCENT, COLOR_WHITE, COLOR_MAGENTA);
		init_pair(CP_MODAL_BG, COLOR_WHITE, COLOR_BLACK);
		init_pair(CP_MODAL_BORDER, COLOR_MAGENTA, COLOR_BLACK);
		init_pair(CP_MODAL_SEL, COLOR_BLACK, COLOR_MAGENTA);
	}
}

void	RendererEngine::WrapText(const t_text &text, int max_width, t_vector &output)
{
	t_vector	words;
	t_text		current_word = "";

	for (size_t i = 0; i < text.length(); ++i)
	{
		if (text[i] == '\n')
		{
			if (!current_word.empty()) { words.push_back(current_word); current_word = ""; }
			words.push_back("\n");
		}
		else if (text[i] == ' ')
		{
			if (!current_word.empty()) { words.push_back(current_word); current_word = ""; }
			words.push_back(" ");
		}
		else
		{
			current_word += text[i];
		}
	}
	if (!current_word.empty())
		words.push_back(current_word);

	t_text	current_line = "";
	for (size_t i = 0; i < words.size(); ++i)
	{
		if (words[i] == "\n")
		{
			output.push_back(current_line);
			current_line = "";
		}
		else
		{
			if (GetDisplayWidth(current_line + words[i]) > max_width)
			{
				if (!current_line.empty())
					output.push_back(current_line);
				current_line = (words[i] == " ") ? "" : words[i];
			}
			else
			{
				current_line += words[i];
			}
		}
	}
	if (!current_line.empty())
		output.push_back(current_line);
}

void	RendererEngine::RenderHighlightedLine(const t_text &line, int max_width, int y, int x)
{
	t_vector	chars = SplitUtf8String(line);
	t_text		current_word = "";
	bool		in_string = false;
	int			current_printed_width = 0;

	move(y, x);

	if (!chars.empty() and chars[0] == "#")
	{
		attron(COLOR_PAIR(CP_PREPROC) | A_BOLD);
		t_text fitted = FitText(line, max_width);
		mvaddstr(y, x, fitted.c_str());
		attroff(COLOR_PAIR(CP_PREPROC) | A_BOLD);
		current_printed_width = GetDisplayWidth(fitted);
		for (int fill = current_printed_width; fill < max_width; ++fill)
			addch(' ');
		return ;
	}

	for (size_t j = 0; j < chars.size(); ++j)
	{
		if (current_printed_width >= max_width)
			break ;

		t_text c = chars[j];

		if (!in_string and j + 1 < chars.size() and c == "/" and chars[j + 1] == "/")
		{
			attron(COLOR_PAIR(CP_COMMENT));
			for (size_t rem = j; rem < chars.size() and current_printed_width < max_width; ++rem)
			{
				addstr(chars[rem].c_str());
				current_printed_width += GetDisplayWidth(chars[rem]);
			}
			attroff(COLOR_PAIR(CP_COMMENT));
			break ;
		}

		if (c == "\"")
		{
			if (in_string)
			{
				current_word += c;
				attron(COLOR_PAIR(CP_STRING));
				addstr(current_word.c_str());
				attroff(COLOR_PAIR(CP_STRING));
				current_printed_width += GetDisplayWidth(current_word);
				current_word = "";
				in_string = false;
			}
			else
			{
				if (!current_word.empty())
				{
					attron(COLOR_PAIR(CP_DEFAULT));
					addstr(current_word.c_str());
					attroff(COLOR_PAIR(CP_DEFAULT));
					current_printed_width += GetDisplayWidth(current_word);
				}
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

		if (c == " " or c == "(" or c == ")" or c == "{" or c == "}" or
			c == ";" or c == "," or c == "<" or c == ">" or c == "\t" or
			c == "[" or c == "]" or c == "+" or c == "-" or c == "*" or
			c == "/" or c == "=" or c == "!" or c == "&" or c == "|" or c == ":")
		{
			if (!current_word.empty())
			{
				if (IsInList(current_word, g_keywords, g_num_keywords))
				{
					attron(COLOR_PAIR(CP_KEYWORD) | A_BOLD);
					addstr(current_word.c_str());
					attroff(COLOR_PAIR(CP_KEYWORD) | A_BOLD);
				}
				else if (IsInList(current_word, g_datatypes, g_num_datatypes))
				{
					attron(COLOR_PAIR(CP_DATATYPE));
					addstr(current_word.c_str());
					attroff(COLOR_PAIR(CP_DATATYPE));
				}
				else if (current_word.find_first_not_of("0123456789") == t_text::npos)
				{
					attron(COLOR_PAIR(CP_NUMBER));
					addstr(current_word.c_str());
					attroff(COLOR_PAIR(CP_NUMBER));
				}
				else
				{
					attron(COLOR_PAIR(CP_DEFAULT));
					addstr(current_word.c_str());
					attroff(COLOR_PAIR(CP_DEFAULT));
				}
				current_printed_width += GetDisplayWidth(current_word);
				current_word = "";
			}

			if (current_printed_width >= max_width)
				break ;

			if (c == "\t")
			{
				addstr("    ");
				current_printed_width += 4;
			}
			else
			{
				attron(COLOR_PAIR(CP_DEFAULT));
				addstr(c.c_str());
				attroff(COLOR_PAIR(CP_DEFAULT));
				current_printed_width += GetDisplayWidth(c);
			}
		}
		else
		{
			current_word += c;
		}
	}

	if (!current_word.empty() and current_printed_width < max_width)
	{
		if (IsInList(current_word, g_keywords, g_num_keywords))
		{
			attron(COLOR_PAIR(CP_KEYWORD) | A_BOLD);
			addstr(current_word.c_str());
			attroff(COLOR_PAIR(CP_KEYWORD) | A_BOLD);
		}
		else if (IsInList(current_word, g_datatypes, g_num_datatypes))
		{
			attron(COLOR_PAIR(CP_DATATYPE));
			addstr(current_word.c_str());
			attroff(COLOR_PAIR(CP_DATATYPE));
		}
		else if (current_word.find_first_not_of("0123456789") == t_text::npos)
		{
			attron(COLOR_PAIR(CP_NUMBER));
			addstr(current_word.c_str());
			attroff(COLOR_PAIR(CP_NUMBER));
		}
		else
		{
			attron(COLOR_PAIR(CP_DEFAULT));
			addstr(current_word.c_str());
			attroff(COLOR_PAIR(CP_DEFAULT));
		}
		current_printed_width += GetDisplayWidth(current_word);
	}

	for (int fill = current_printed_width; fill < max_width; ++fill)
		addch(' ');
}



static void RenderModelModal(EditorContext &ctx)
{
	if (!ctx.model_modal_open)
		return ;

	int modal_w = ctx.screen_cols * 70 / 100;
	if (modal_w < 45) modal_w = 45;
	if (modal_w > ctx.screen_cols - 4) modal_w = ctx.screen_cols - 4;

	int modal_h = ctx.screen_rows * 65 / 100;
	if (modal_h < 10) modal_h = 10;
	if (modal_h > ctx.screen_rows - 4) modal_h = ctx.screen_rows - 4;

	int start_x = (ctx.screen_cols - modal_w) / 2;
	int start_y = (ctx.screen_rows - modal_h) / 2;
	int inner_w = modal_w - 2;

	// 1. Linha Superior (Header do Modal)
	move(start_y, start_x);
	attron(COLOR_PAIR(CP_MODAL_BORDER) | A_BOLD);
	addstr("+");
	t_text title = " Select / Download Ollama AI Model ";
	int t_fill = inner_w - GetDisplayWidth(title);
	if (t_fill < 0) t_fill = 0;
	int left_dash = t_fill / 2;
	int right_dash = t_fill - left_dash;
	addstr(BuildHorizontalLine(left_dash, "-").c_str());
	addstr(title.c_str());
	addstr(BuildHorizontalLine(right_dash, "-").c_str());
	addstr("+");
	attroff(COLOR_PAIR(CP_MODAL_BORDER) | A_BOLD);

	// 2. Linha 1: Active Model Status
	move(start_y + 1, start_x);
	attron(COLOR_PAIR(CP_MODAL_BORDER) | A_BOLD);
	addstr("|");
	attroff(COLOR_PAIR(CP_MODAL_BORDER) | A_BOLD);

	attron(COLOR_PAIR(CP_MODAL_BG) | A_BOLD);
	t_text active_str = " Active: " + (ctx.model_modal_active_model.empty() ? "SSBot" : ctx.model_modal_active_model);
	active_str = FitText(active_str, inner_w);
	mvaddstr(start_y + 1, start_x + 1, active_str.c_str());
	int act_w = GetDisplayWidth(active_str);
	for (int fill = act_w; fill < inner_w; ++fill) addch(' ');
	attroff(COLOR_PAIR(CP_MODAL_BG) | A_BOLD);

	attron(COLOR_PAIR(CP_MODAL_BORDER) | A_BOLD);
	mvaddstr(start_y + 1, start_x + modal_w - 1, "|");
	attroff(COLOR_PAIR(CP_MODAL_BORDER) | A_BOLD);

	// 3. Linhas Interiores de Itens
	int list_rows = modal_h - 3;
	int total_items = static_cast<int>(ctx.model_modal_items.size());

	for (int r = 0; r < list_rows; ++r)
	{
		int row_y = start_y + 2 + r;
		move(row_y, start_x);

		attron(COLOR_PAIR(CP_MODAL_BORDER) | A_BOLD);
		addstr("|");
		attroff(COLOR_PAIR(CP_MODAL_BORDER) | A_BOLD);

		int item_idx = r;
		if (item_idx < total_items)
		{
			t_text item_str = ctx.model_modal_items[item_idx];
			bool is_selected = (item_idx == ctx.model_modal_selected_idx);

			t_text line = (is_selected ? " > " : "   ") + item_str;
			line = FitText(line, inner_w);
			int dw = GetDisplayWidth(line);

			int pair = is_selected ? CP_MODAL_SEL : CP_MODAL_BG;
			attron(COLOR_PAIR(pair) | (is_selected ? A_BOLD : A_NORMAL));
			mvaddstr(row_y, start_x + 1, line.c_str());
			for (int fill = dw; fill < inner_w; ++fill) addch(' ');
			attroff(COLOR_PAIR(pair) | (is_selected ? A_BOLD : A_NORMAL));
		}
		else
		{
			attron(COLOR_PAIR(CP_MODAL_BG));
			for (int c = 0; c < inner_w; ++c) addch(' ');
			attroff(COLOR_PAIR(CP_MODAL_BG));
		}

		attron(COLOR_PAIR(CP_MODAL_BORDER) | A_BOLD);
		mvaddstr(row_y, start_x + modal_w - 1, "|");
		attroff(COLOR_PAIR(CP_MODAL_BORDER) | A_BOLD);
	}

	// 4. Linha Inferior (Footer com Atalhos)
	move(start_y + modal_h - 1, start_x);
	attron(COLOR_PAIR(CP_MODAL_BORDER) | A_BOLD);
	addstr("+");
	t_text footer = " UP/DOWN: Navigate | ENTER: Select/Download | ESC: Close ";
	int f_fill = inner_w - GetDisplayWidth(footer);
	if (f_fill < 0) f_fill = 0;
	int f_left = f_fill / 2;
	int f_right = f_fill - f_left;
	addstr(BuildHorizontalLine(f_left, "-").c_str());
	addstr(footer.c_str());
	addstr(BuildHorizontalLine(f_right, "-").c_str());
	addstr("+");
	attroff(COLOR_PAIR(CP_MODAL_BORDER) | A_BOLD);
}

void	RendererEngine::RefreshScreen(EditorContext &ctx)
{
	getmaxyx(stdscr, ctx.screen_rows, ctx.screen_cols);
	ctx.ScrollEditor();

	erase();

	if (ctx.open_files.empty())
		ctx.OpenFileBuffer("");
	FileBuffer &cur = ctx.open_files[ctx.active_file_idx];

	// 1. BARRA SUPERIOR (HEADER LIMPO SEM BOTÕES NEM EMOJIS)
	t_text active_name = cur.name.empty() ? "[Untitled]" : cur.name;
	t_text title = " SSCodeIDE - " + active_name + (cur.is_dirty ? " [+]" : "");

	attron(COLOR_PAIR(CP_HEADER_ACCENT) | A_BOLD);
	mvaddstr(0, 0, title.c_str());
	attroff(COLOR_PAIR(CP_HEADER_ACCENT) | A_BOLD);

	int cur_hx = GetDisplayWidth(title);
	t_text top_right = (ctx.drawer_open ? "EXPLORER: ON" : "EXPLORER: OFF");
	top_right += " | ";
	top_right += (ctx.ai_panel_open ? "CHAT: ON " : "CHAT: OFF ");

	int top_fill = ctx.screen_cols - cur_hx - GetDisplayWidth(top_right);
	if (top_fill < 0) top_fill = 0;
	attron(COLOR_PAIR(CP_HEADER));
	for (int t = 0; t < top_fill; ++t)
		addch(' ');
	addstr(top_right.c_str());
	attroff(COLOR_PAIR(CP_HEADER));

	// 2. BARRA DE SEPARADORES (TABS)
	int tab_col = 0;
	for (size_t i = 0; i < ctx.open_files.size(); ++i)
	{
		t_text tab_name = ctx.open_files[i].name.empty() ? "*Sem Nome*" : ctx.open_files[i].name;
		t_text icon = GetFileIcon(tab_name, false, false);
		tab_name = icon + " " + FitText(tab_name, 18);

		if (static_cast<int>(i) == ctx.active_file_idx)
		{
			attron(COLOR_PAIR(CP_TAB_ACTIVE) | A_BOLD);
			mvaddstr(1, tab_col, (" " + tab_name).c_str());
			if (ctx.open_files[i].is_dirty)
			{
				attroff(COLOR_PAIR(CP_TAB_ACTIVE) | A_BOLD);
				attron(COLOR_PAIR(CP_TAB_DIRTY) | A_BOLD);
				addstr(" [+]");
				attroff(COLOR_PAIR(CP_TAB_DIRTY) | A_BOLD);
				attron(COLOR_PAIR(CP_TAB_ACTIVE) | A_BOLD);
			}
			addstr(" ");
			attroff(COLOR_PAIR(CP_TAB_ACTIVE) | A_BOLD);
			tab_col += GetDisplayWidth(" " + tab_name + (ctx.open_files[i].is_dirty ? " [+]" : "") + " ");
		}
		else
		{
			attron(COLOR_PAIR(CP_TAB_INACTIVE));
			mvaddstr(1, tab_col, ("│ " + tab_name).c_str());
			if (ctx.open_files[i].is_dirty)
			{
				attroff(COLOR_PAIR(CP_TAB_INACTIVE));
				attron(COLOR_PAIR(CP_TAB_DIRTY) | A_BOLD);
				addstr(" [+]");
				attroff(COLOR_PAIR(CP_TAB_DIRTY) | A_BOLD);
				attron(COLOR_PAIR(CP_TAB_INACTIVE));
			}
			addstr(" ");
			attroff(COLOR_PAIR(CP_TAB_INACTIVE));
			tab_col += GetDisplayWidth("│ " + tab_name + (ctx.open_files[i].is_dirty ? " [+]" : "") + " ");
		}
		if (tab_col >= ctx.screen_cols - 10)
			break ;
	}

	// Separador [+] New
	if (tab_col < ctx.screen_cols - 10)
	{
		attron(COLOR_PAIR(CP_HEADER_ACCENT));
		mvaddstr(1, tab_col, "│ [+] New ");
		attroff(COLOR_PAIR(CP_HEADER_ACCENT));
		tab_col += GetDisplayWidth("│ [+] New ");
	}

	attron(COLOR_PAIR(CP_TAB_INACTIVE));
	for (int fill = tab_col; fill < ctx.screen_cols; ++fill)
		mvaddch(1, fill, ' ');
	attroff(COLOR_PAIR(CP_TAB_INACTIVE));

	// 3. CÁLCULO DE DIMENSÕES (ESQUEMA 3 COLUNAS SEM NAV RAIL LATERAL)
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

	int code_window_width = ctx.screen_cols - drawer_width - ai_panel_width;
	if (code_window_width < 10) code_window_width = 10;
	int line_number_width = 7;
	int effective_code_text_width = code_window_width - line_number_width - 1;
	if (effective_code_text_width < 4) effective_code_text_width = 4;

	// Scrollbar
	int total_lines = static_cast<int>(cur.lines.size());
	int scroll_handle_size = (edit_window_height * edit_window_height) / (total_lines > 0 ? total_lines : 1);
	if (scroll_handle_size < 1) scroll_handle_size = 1;
	if (scroll_handle_size > edit_window_height) scroll_handle_size = edit_window_height;
	int scroll_handle_pos = 0;
	if (total_lines > edit_window_height)
		scroll_handle_pos = (cur.row_offset * (edit_window_height - scroll_handle_size)) / (total_lines - edit_window_height);

	// 4. ÁREA CENTRAL E ESQUERDA (DRAWER + CÓDIGO)
	for (int i = 0; i < edit_window_height; ++i)
	{
		int screen_y = 2 + i;
		int file_line_idx = i + cur.row_offset;
		bool is_handle = (i >= scroll_handle_pos and i < scroll_handle_pos + scroll_handle_size);

		// A) Drawer lateral (Explorer)
		if (ctx.drawer_open)
		{
			int drawer_start_x = 0;
			move(screen_y, drawer_start_x);

			if (i == 0) // Header do Explorer
			{
				bool is_exp_focused = (ctx.current_mode == MODE_FILE_MANAGER);
				if (is_exp_focused) attron(COLOR_PAIR(CP_HEADER_ACCENT) | A_BOLD);
				else attron(COLOR_PAIR(CP_AI_HEADER) | A_BOLD);

				t_text exp_title = is_exp_focused ? " 📂 EXPLORER [● FOCUS] " : " 📁 EXPLORER ";
				mvaddstr(screen_y, drawer_start_x, exp_title.c_str());
				int drawn = GetDisplayWidth(exp_title);
				for (int s = drawn; s < drawer_width - 1; ++s)
					addch(' ');

				if (is_exp_focused) attroff(COLOR_PAIR(CP_HEADER_ACCENT) | A_BOLD);
				else attroff(COLOR_PAIR(CP_AI_HEADER) | A_BOLD);
			}
			else
			{
				int data_idx = i - 1;
				if (data_idx < static_cast<int>(ctx.explorer_items.size()))
				{
					const ExplorerItem &item = ctx.explorer_items[data_idx];
					t_text indent = "";
					for (int d = 0; d < item.depth; ++d) indent += " ";

					t_text icon = GetFileIcon(item.name, item.is_dir, item.is_expanded);
					t_text line_str = "";
					if (item.is_dir)
					{
						if (item.name == ".. (Up)") line_str = indent + "^ " + item.name;
						else line_str = indent + icon + " " + item.name + "/";
					}
					else
					{
						line_str = indent + "  " + icon + " " + item.name;
					}
					line_str = FitText(line_str, drawer_width - 2);

					bool is_selected = (ctx.current_mode == MODE_FILE_MANAGER and data_idx == ctx.selected_file_idx);

					if (is_selected)
					{
						attron(COLOR_PAIR(CP_DRAWER_SEL) | A_BOLD);
						t_text print_str = "> " + line_str;
						mvaddstr(screen_y, drawer_start_x, print_str.c_str());
						int drawn_w = GetDisplayWidth(print_str);
						for (int s = drawn_w; s < drawer_width - 1; ++s)
							addch(' ');
						attroff(COLOR_PAIR(CP_DRAWER_SEL) | A_BOLD);
					}
					else
					{
						if (item.is_dir) attron(COLOR_PAIR(CP_DRAWER_DIR) | A_BOLD);
						else attron(COLOR_PAIR(CP_DRAWER_FILE));

						t_text print_str = "  " + line_str;
						mvaddstr(screen_y, drawer_start_x, print_str.c_str());
						int drawn_w = GetDisplayWidth(print_str);
						for (int s = drawn_w; s < drawer_width - 1; ++s)
							addch(' ');

						if (item.is_dir) attroff(COLOR_PAIR(CP_DRAWER_DIR) | A_BOLD);
						else attroff(COLOR_PAIR(CP_DRAWER_FILE));
					}
				}
				else
				{
					for (int s = 0; s < drawer_width - 1; ++s)
						addch(' ');
				}
			}
			attron(COLOR_PAIR(CP_GUTTER));
			mvaddstr(screen_y, drawer_start_x + drawer_width - 1, "│");
			attroff(COLOR_PAIR(CP_GUTTER));
		}

		// B) Editor de Código (Coluna do Meio)
		int code_start_x = drawer_width;
		int code_end_x = drawer_width + code_window_width - 1;
		move(screen_y, code_start_x);

		if (file_line_idx >= total_lines)
		{
			attron(COLOR_PAIR(CP_GUTTER));
			mvaddstr(screen_y, code_start_x, "~      ");
			for (int space = 0; space < effective_code_text_width; ++space)
				addch(' ');
			if (is_handle)
			{
				attron(COLOR_PAIR(CP_SCROLLBAR_HANDLE));
				mvaddstr(screen_y, code_end_x, "#");
				attroff(COLOR_PAIR(CP_SCROLLBAR_HANDLE));
			}
			else
			{
				attron(COLOR_PAIR(CP_SCROLLBAR_TRACK));
				mvaddstr(screen_y, code_end_x, "│");
				attroff(COLOR_PAIR(CP_SCROLLBAR_TRACK));
			}
			attroff(COLOR_PAIR(CP_GUTTER));
		}
		else
		{
			int line_num = file_line_idx + 1;
			char num_buf[16];
			snprintf(num_buf, sizeof(num_buf), "%4d │ ", line_num);

			if (file_line_idx == cur.cursor_y)
				attron(COLOR_PAIR(CP_GUTTER_ACTIVE) | A_BOLD);
			else
				attron(COLOR_PAIR(CP_GUTTER));
			mvaddstr(screen_y, code_start_x, num_buf);
			if (file_line_idx == cur.cursor_y)
				attroff(COLOR_PAIR(CP_GUTTER_ACTIVE) | A_BOLD);
			else
				attroff(COLOR_PAIR(CP_GUTTER));

			RenderHighlightedLine(cur.lines[file_line_idx], effective_code_text_width, screen_y, code_start_x + line_number_width);

			if (is_handle)
			{
				attron(COLOR_PAIR(CP_SCROLLBAR_HANDLE));
				mvaddstr(screen_y, code_end_x, "#");
				attroff(COLOR_PAIR(CP_SCROLLBAR_HANDLE));
			}
			else
			{
				attron(COLOR_PAIR(CP_SCROLLBAR_TRACK));
				mvaddstr(screen_y, code_end_x, "│");
				attroff(COLOR_PAIR(CP_SCROLLBAR_TRACK));
			}
		}
	}

	// 5. PAINEL SSBOT CLI INTERACTIVO (COLUNA DA DIREITA)
	int ai_input_y = -1;
	int ai_input_x = -1;

	if (ctx.ai_panel_open and ai_panel_width > 0)
	{
		int ai_start_x = ctx.screen_cols - ai_panel_width;
		int ai_cols = ai_panel_width;

		// Linha divisória vertical
		for (int r = 2; r < ctx.screen_rows - 1; ++r)
		{
			attron(COLOR_PAIR(CP_GUTTER));
			mvaddstr(r, ai_start_x, "│");
			attroff(COLOR_PAIR(CP_GUTTER));
		}

		int content_x = ai_start_x + 1;
		int content_w = ai_cols - 1;

		// Header do SSBot Chat
		bool is_ai_focused = (ctx.current_mode == MODE_AI_PROMPT);
		t_text header_title = is_ai_focused ? "── [SSBot Chat ● FOCUS] " : "── [SSBot Chat] ";

		if (is_ai_focused) attron(COLOR_PAIR(CP_HEADER_ACCENT) | A_BOLD);
		else attron(COLOR_PAIR(CP_AI_HEADER) | A_BOLD);

		mvaddstr(2, content_x, FitText(header_title, content_w).c_str());
		int drawn_h = GetDisplayWidth(FitText(header_title, content_w));
		if (content_w - drawn_h > 0)
			addstr(BuildHorizontalLine(content_w - drawn_h, "─").c_str());

		if (is_ai_focused) attroff(COLOR_PAIR(CP_HEADER_ACCENT) | A_BOLD);
		else attroff(COLOR_PAIR(CP_AI_HEADER) | A_BOLD);

		int history_rows = edit_window_height - 2;
		for (int r = 0; r < history_rows; ++r)
		{
			int row_y = 3 + r;
			move(row_y, content_x);
			attron(COLOR_PAIR(CP_AI_PANEL));

			int data_idx = r + ctx.ai_scroll_offset;
			if (data_idx < static_cast<int>(ctx.ai_response_lines.size()))
			{
				t_text line = ctx.ai_response_lines[data_idx];
				line = FitText(line, content_w - 1);

				t_text user_tag = "You ❯ ";
				t_text bot_tag = "SSBot ❯ ";

				if (line.find(user_tag) == 0)
				{
					attron(COLOR_PAIR(CP_AI_USER) | A_BOLD);
					mvaddstr(row_y, content_x, user_tag.c_str());
					attroff(COLOR_PAIR(CP_AI_USER) | A_BOLD);

					attron(COLOR_PAIR(CP_DEFAULT));
					addstr(line.substr(user_tag.length()).c_str());
					attroff(COLOR_PAIR(CP_DEFAULT));
				}
				else if (line.find(bot_tag) == 0)
				{
					attron(COLOR_PAIR(CP_AI_TEXT) | A_BOLD);
					mvaddstr(row_y, content_x, "[SSBot] ❯ ");
					addstr(line.substr(bot_tag.length()).c_str());
					attroff(COLOR_PAIR(CP_AI_TEXT) | A_BOLD);
				}
				else if (line.find("[SSBot]") == 0)
				{
					attron(COLOR_PAIR(CP_AI_TEXT) | A_BOLD);
					mvaddstr(row_y, content_x, line.c_str());
					attroff(COLOR_PAIR(CP_AI_TEXT) | A_BOLD);
				}
				else if (line.find("[System]") == 0 or line.find("[Command]") == 0)
				{
					attron(COLOR_PAIR(CP_AI_SYS) | A_BOLD);
					mvaddstr(row_y, content_x, line.c_str());
					attroff(COLOR_PAIR(CP_AI_SYS) | A_BOLD);
				}
				else if (line.find("[Error]") == 0)
				{
					attron(COLOR_PAIR(CP_TAB_DIRTY) | A_BOLD);
					mvaddstr(row_y, content_x, line.c_str());
					attroff(COLOR_PAIR(CP_TAB_DIRTY) | A_BOLD);
				}
				else if (line.find("──") == 0 or line.find("--") == 0)
				{
					attron(COLOR_PAIR(CP_GUTTER));
					mvaddstr(row_y, content_x, line.c_str());
					attroff(COLOR_PAIR(CP_GUTTER));
				}
				else
				{
					attron(COLOR_PAIR(CP_AI_PANEL));
					mvaddstr(row_y, content_x, line.c_str());
					attroff(COLOR_PAIR(CP_AI_PANEL));
				}

				int drawn_w = GetDisplayWidth(line);
				attron(COLOR_PAIR(CP_AI_PANEL));
				for (int fill = drawn_w; fill < content_w; ++fill)
					addch(' ');
				attroff(COLOR_PAIR(CP_AI_PANEL));
			}
			else
			{
				for (int fill = 0; fill < content_w; ++fill)
					addch(' ');
			}
			attroff(COLOR_PAIR(CP_AI_PANEL));
		}

		// Linha interactiva de input CLI do SSBot no fundo da coluna da direita
		ai_input_y = 2 + edit_window_height - 1;
		attron(COLOR_PAIR(CP_AI_PANEL));
		mvaddstr(ai_input_y, content_x, "");
		attroff(COLOR_PAIR(CP_AI_PANEL));

		t_text prompt_tag = "SSBot ❯ ";
		if (ctx.current_mode == MODE_AI_PROMPT)
			attron(COLOR_PAIR(CP_HEADER_ACCENT) | A_BOLD);
		else
			attron(COLOR_PAIR(CP_GUTTER));
		mvaddstr(ai_input_y, content_x, prompt_tag.c_str());
		if (ctx.current_mode == MODE_AI_PROMPT)
			attroff(COLOR_PAIR(CP_HEADER_ACCENT) | A_BOLD);
		else
			attroff(COLOR_PAIR(CP_GUTTER));

		attron(COLOR_PAIR(CP_DEFAULT) | A_BOLD);
		addstr(ctx.ai_current_input.c_str());
		attroff(COLOR_PAIR(CP_DEFAULT) | A_BOLD);

		int input_w = GetDisplayWidth(prompt_tag) + GetDisplayWidth(ctx.ai_current_input);
		attron(COLOR_PAIR(CP_AI_PANEL));
		for (int fill = input_w; fill < content_w; ++fill)
			addch(' ');
		attroff(COLOR_PAIR(CP_AI_PANEL));

		ai_input_x = content_x + GetDisplayWidth(prompt_tag) + GetDisplayWidth(ctx.ai_current_input.substr(0, ctx.ai_prompt_cursor));
		if (ai_input_x >= ctx.screen_cols) ai_input_x = ctx.screen_cols - 1;
	}

	// 6. BARRA DE ESTADO INFERIOR
	int status_y = ctx.screen_rows - 1;
	t_text file_name = cur.name.empty() ? "[Untitled]" : cur.name;
	t_text mode_str = (ctx.current_mode == MODE_FILE_MANAGER ? "FILES" : (ctx.current_mode == MODE_AI_PROMPT ? "SSBOT CLI" : (ctx.current_mode == MODE_MODAL ? "MODAL" : "NORMAL")));
	t_text pos_str = "Ln " + IntToString(cur.cursor_y + 1) + ", Col " + IntToString(cur.cursor_x + 1);
	t_text hints_str = (ctx.current_mode == MODE_AI_PROMPT ? "Type /models for Ollama downloader modal • [Esc: Editor]" : "^B: Explorer • ^G: SSBot CLI • ^K / F4: Models Modal • /help /save /quit");
	t_text status = " " + mode_str + " • " + file_name + (cur.is_dirty ? " [+]" : "") + " • " + pos_str + " • " + hints_str + " ";
	status = FitText(status, ctx.screen_cols);

	attron(COLOR_PAIR(CP_STATUS_ACCENT) | A_BOLD);
	mvaddstr(status_y, 0, (" " + mode_str + " ").c_str());
	attroff(COLOR_PAIR(CP_STATUS_ACCENT) | A_BOLD);

	int start_pos = GetDisplayWidth(" " + mode_str + " ");
	attron(COLOR_PAIR(CP_STATUS));
	mvaddstr(status_y, start_pos, ("• " + file_name + (cur.is_dirty ? " [+]" : "") + " • " + pos_str + " • " + hints_str).c_str());
	int drawn = GetDisplayWidth(" " + mode_str + " • " + file_name + (cur.is_dirty ? " [+]" : "") + " • " + pos_str + " • " + hints_str);
	for (int f = drawn; f < ctx.screen_cols; ++f)
		addch(' ');
	attroff(COLOR_PAIR(CP_STATUS));

	// 7. POPUP MODAL INTERACTIVO DE SELEÇÃO DE MODELOS OLLAMA
	if (ctx.model_modal_open or ctx.current_mode == MODE_MODAL)
	{
		RenderModelModal(ctx);
	}

	// 8. GESTÃO E POSICIONAMENTO DO CURSOR
	if (ctx.current_mode == MODE_AI_PROMPT and ai_input_y != -1)
	{
		move(ai_input_y, ai_input_x);
		curs_set(1);
	}
	else if (ctx.current_mode == MODE_FILE_MANAGER or ctx.current_mode == MODE_MODAL)
	{
		curs_set(0);
	}
	else
	{
		int code_start_x = drawer_width;
		int visual_cursor_x = code_start_x + line_number_width;
		if (cur.cursor_y < total_lines)
		{
			t_vector l_chars = SplitUtf8String(cur.lines[cur.cursor_y]);
			for (int k = 0; k < cur.cursor_x and k < static_cast<int>(l_chars.size()); ++k)
			{
				if (l_chars[k] == "\t") visual_cursor_x += 4;
				else visual_cursor_x += GetDisplayWidth(l_chars[k]);
			}
		}
		int physical_cursor_y = 2 + (cur.cursor_y - cur.row_offset);
		move(physical_cursor_y, visual_cursor_x);
		curs_set(1);
	}

	refresh();
}
