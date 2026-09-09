/* ************************************************************************** */
/*                                                                            */
/*                                                       ::::::::   ::::::::  */
/*    sscode.cpp                                       :+:    :+: :+:    :+:  */
/*                                                    +:+        +:+          */
/*    By: Ser Superior <marcioeduine@gmail.com>      +#++:++#++ +#++:++#++    */
/*                                                         +#+        +#+     */
/*    Created: 2026/07/02 01:38:25 by Ser Superior #+#    #+# #+#    #+#      */
/*    Updated: 2026/09/09 18:50:00 by Ser Superior ########   ########        */
/*                                                                            */
/* ************************************************************************** */

#define _XOPEN_SOURCE_EXTENDED 1

#include <locale.h>
#include <wchar.h>
#include <ncurses.h>
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <dirent.h>
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <sstream>
#include <chrono>

/* ************************************************************************** */
/*                                                                            */
/*                                                       ::::::::   ::::::::  */
/*    sscode.hpp                                       :+:    :+: :+:    :+:  */
/*                                                    +:+        +:+          */
/*    By: Ser Superior <marcioeduine@gmail.com>      +#++:++#++ +#++:++#++    */
/*                                                         +#+        +#+     */
/*    Created: 2026/07/02 10:15:56 by Ser Superior #+#    #+# #+#    #+#      */
/*    Updated: 2026/09/09 16:50:00 by Ser Superior ########   ########        */
/*                                                                            */
/* ************************************************************************** */

#ifndef SSCODE_HPP
# define SSCODE_HPP

# define _XOPEN_SOURCE_EXTENDED 1

# include <locale.h>
# include <wchar.h>
# include <ncurses.h>
# include <iostream>
# include <string>
# include <vector>

// --- ICONOGRAFIA TEXTUAL LIMPA (SEM EMOJIS) ---
# define ICON_FOLDER        "[DIR]"
# define ICON_FILE          "[FILE]"
# define ICON_ROBOT         "[SSBot]"

# define CTRL_KEY(k)        ((k) & 0x1f)

# define KEY_MOUSE_SCROLL_UP    1050
# define KEY_MOUSE_SCROLL_DOWN  1051
# define KEY_MOUSE_IGNORE       1052
# define KEY_MOUSE_CLICK        1053
# define KEY_MOUSE_HOVER        1054

enum e_editorMode
{
	MODE_EDITOR,
	MODE_FILE_MANAGER,
	MODE_AI_PROMPT,
	MODE_MODAL
};

enum e_colorPair
{
	CP_DEFAULT = 1,
	CP_HEADER,
	CP_HEADER_ACCENT,
	CP_TAB_ACTIVE,
	CP_TAB_INACTIVE,
	CP_TAB_DIRTY,
	CP_GUTTER,
	CP_GUTTER_ACTIVE,
	CP_KEYWORD,
	CP_DATATYPE,
	CP_STRING,
	CP_COMMENT,
	CP_PREPROC,
	CP_NUMBER,
	CP_STATUS,
	CP_STATUS_ACCENT,
	CP_AI_HEADER,
	CP_AI_PANEL,
	CP_AI_TEXT,
	CP_AI_USER,
	CP_AI_SYS,
	CP_PROMPT_BOX,
	CP_PROMPT_ACCENT,
	CP_DRAWER_DIR,
	CP_DRAWER_FILE,
	CP_DRAWER_SEL,
	CP_SCROLLBAR_HANDLE,
	CP_SCROLLBAR_TRACK,
	CP_NAV_RAIL_BG,
	CP_NAV_RAIL_ACTIVE,
	CP_NAV_RAIL_INACTIVE,
	CP_NAV_RAIL_INDICATOR,
	CP_BTN_ACCENT,
	CP_HOVER_ACCENT,
	CP_MODAL_BG,
	CP_MODAL_BORDER,
	CP_MODAL_SEL
};

typedef std::string 		t_text;
typedef std::vector<t_text>	t_vector;

struct FileBuffer
{
	t_vector				lines;
	std::vector<t_vector>	undo_stack;
	t_text					name;
	int						cursor_x;
	int						cursor_y;
	int						row_offset;
	bool					is_dirty;
};

struct ExplorerItem
{
	t_text					name;
	t_text					path;
	bool					is_dir;
	bool					is_expanded;
	int						depth;
};

class	EditorContext
{
	public:
		std::vector<FileBuffer>	open_files;
		int						active_file_idx;
		e_editorMode			current_mode;
		int						screen_rows;
		int						screen_cols;

		// Gestor de Ficheiros e Gaveta Lateral (Drawer)
		bool						drawer_open;
		t_vector					file_list;
		std::vector<ExplorerItem>	explorer_items;
		t_text						current_dir;
		int							selected_file_idx;
		
		// Chatbot CLI (SSBot)
		bool					ai_panel_open;
		bool					ai_panel_expanded;
		t_vector				ai_response_lines;
		int						ai_scroll_offset;
		t_text					ai_current_input;
		int						ai_prompt_cursor;
		t_text					ai_full_response;

		// Modal Interativo de Modelos Ollama
		bool					model_modal_open;
		t_vector				model_modal_items;
		int						model_modal_selected_idx;
		t_text					model_modal_active_model;

		// Interacção com Rato
		int						mouse_x;
		int						mouse_y;
		int						mouse_bstate;
		int						hover_x;
		int						hover_y;
		t_text					hover_target;
		
		bool					should_quit;
		t_vector				clipboard;

		EditorContext(void);
		void	OpenFileBuffer(const t_text &filename);
		void	LoadDirectoryFiles(const t_text &dir_path = ".");
		void	ToggleDirectoryExpand(int index);
		void	OpenModelModal(void);
		void	CloseModelModal(void);
		void	SaveFile(void);
		void	SaveUndoState(FileBuffer &fb);
		void	ScrollEditor(void);
		void	AddCliMessage(const t_text &prefix, const t_text &message);
		void	ScrollCliToBottom(void);
};

class	TerminalManager
{
	public:
		void			Init(EditorContext &ctx);
		void			Shutdown(void);
		void			GetTerminalSize(EditorContext &ctx);
		int				ReadKey(EditorContext &ctx);
};

class	RendererEngine
{
	public:
		void	InitColors(void);
		void	RefreshScreen(EditorContext &ctx);
		void	RenderHighlightedLine(const t_text &line, int max_width, int y, int x);
		void	WrapText(const t_text &text, int max_width, t_vector &output);
};

int				GetDisplayWidth(const t_text &str);
t_vector		SplitUtf8String(const t_text &str);
t_text			JoinUtf8Chars(const t_vector &chars);
t_text			FitText(const t_text &text, int width);
t_text			SanitizeForUi(const t_text &raw);
t_text			IntToString(int val);
t_text			GetFileIcon(const t_text &filename, bool is_dir = false, bool is_expanded = false);

#endif

/* ************************************************************************** */
/*                                                                            */
/*                                                       ::::::::   ::::::::  */
/*    EditorContextUtils.cpp                           :+:    :+: :+:    :+:  */
/*                                                    +:+        +:+          */
/*    By: Ser Superior <marcioeduine@gmail.com>      +#++:++#++ +#++:++#++    */
/*                                                         +#+        +#+     */
/*    Created: 2026/07/02 10:15:56 by Ser Superior #+#    #+# #+#    #+#      */
/*    Updated: 2026/09/09 15:10:00 by Ser Superior ########   ########        */
/*                                                                            */
/* ************************************************************************** */


#include <fstream>
#include <sstream>

void	EditorContext::SaveFile(void)
{
	if (open_files.empty() or active_file_idx >= static_cast<int>(open_files.size()))
		return ;

	FileBuffer		&cur(open_files[active_file_idx]);
	std::ofstream	file;

	if (cur.name.empty())
		cur.name = "output.ss";
	file.open(cur.name.c_str());
	if (file.is_open())
	{
		for (size_t i(0); i < cur.lines.size(); ++i)
			file << cur.lines[i] << "\n";
		file.close();
		cur.is_dirty = false;
	}
}

void	EditorContext::SaveUndoState(FileBuffer &fb)
{
	if (fb.undo_stack.size() > 50)
		fb.undo_stack.erase(fb.undo_stack.begin());
	fb.undo_stack.push_back(fb.lines);
	fb.is_dirty = true;
}

void	EditorContext::ScrollEditor(void)
{
	if (open_files.empty() or active_file_idx >= static_cast<int>(open_files.size()))
		return ;

	FileBuffer	&cur(open_files[active_file_idx]);
	int			ai_panel_height(ai_panel_open ? 8 : 0);
	int			edit_window_height(screen_rows - 3 - ai_panel_height);

	if (edit_window_height < 1)
		edit_window_height = 1;
	if (cur.cursor_y < cur.row_offset)
		cur.row_offset = cur.cursor_y;
	if (cur.cursor_y >= cur.row_offset + edit_window_height)
		cur.row_offset = cur.cursor_y - edit_window_height + 1;
	if (cur.row_offset < 0)
		cur.row_offset = 0;
}

int	GetDisplayWidth(const t_text &str)
{
	if (str.empty())
		return (0);
	std::vector<wchar_t> wbuf(str.length() + 1);
	size_t count = mbstowcs(&wbuf[0], str.c_str(), str.length() + 1);
	if (count == static_cast<size_t>(-1))
		return (static_cast<int>(str.length()));
	int width = 0;
	for (size_t i = 0; i < count; ++i)
	{
		int w = wcwidth(wbuf[i]);
		if (w > 0)
			width += w;
	}
	return (width);
}

static int	GetUtf8CharLength(unsigned char c)
{
	if ((c & 0x80) == 0) return (1);
	if ((c & 0xE0) == 0xC0) return (2);
	if ((c & 0xF0) == 0xE0) return (3);
	if ((c & 0xF8) == 0xF0) return (4);
	return (1);
}

t_vector	SplitUtf8String(const t_text &str)
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

t_text	JoinUtf8Chars(const t_vector &chars)
{
	t_text	out("");
	for (size_t i(0); i < chars.size(); ++i)
		out += chars[i];
	return (out);
}

t_text	FitText(const t_text &text, int width)
{
	if (width <= 0) return ("");
	if (GetDisplayWidth(text) <= width) return (text);
	if (width <= 3) return (text.substr(0, width));

	t_vector chars = SplitUtf8String(text);
	t_text out = "";
	for (size_t i = 0; i < chars.size(); ++i)
	{
		if (GetDisplayWidth(out + chars[i]) > width - 3)
			break ;
		out += chars[i];
	}
	return (out + "...");
}

t_text	SanitizeForUi(const t_text &raw)
{
	t_text			clean("");
	size_t			i(0);
	unsigned char	b(0);

	while (i < raw.length())
	{
		b = static_cast<unsigned char>(raw[i]);
		if (b == 0x1B)
		{
			if (i + 1 < raw.length() and raw[i + 1] == '[')
			{
				i += 2;
				while (i < raw.length() and ((raw[i] >= '0' and raw[i] <= '9') or raw[i] == ';'))
					i++;
				if (i < raw.length()) i++;
				continue ;
			}
			if (i + 1 < raw.length() and raw[i + 1] == ']')
			{
				i += 2;
				while (i < raw.length() and raw[i] != '\a') i++;
				if (i < raw.length()) i++;
				continue ;
			}
			i++;
			continue ;
		}
		if (raw[i] == '\r') { i++; continue ; }
		if (raw[i] == '\t') { clean += "    "; i++; continue ; }
		if (raw[i] == '\n') { clean += '\n'; i++; continue ; }
		if (b < 32 or b == 127) { i++; continue ; }
		clean += raw[i];
		i++;
	}
	return (clean);
}

t_text	IntToString(int val)
{
	std::ostringstream	oss;
	oss << val;
	return (oss.str());
}

t_text	GetFileIcon(const t_text &filename, bool is_dir, bool is_expanded)
{
	if (is_dir)
	{
		if (filename == ".. (Up)") return ("^");
		return (is_expanded ? "📂" : "📁");
	}

	size_t dot = filename.find_last_of('.');
	t_text ext = "";
	if (dot != t_text::npos)
		ext = filename.substr(dot);

	if (ext == ".cpp" or ext == ".c" or ext == ".cc" or ext == ".cxx")
		return ("");
	if (ext == ".hpp" or ext == ".h" or ext == ".hh" or ext == ".hxx")
		return ("");
	if (ext == ".py" or ext == ".pyw")
		return ("");
	if (ext == ".sh" or ext == ".bash" or filename == "Makefile" or ext == ".mk")
		return ("");
	if (ext == ".md" or ext == ".txt")
		return ("");
	if (ext == ".json" or ext == ".yaml" or ext == ".yml")
		return ("");
	if (filename == ".gitignore" or filename == ".gitattributes" or ext == ".git")
		return ("");
	if (ext == ".ss")
		return ("⚙");

	return ("📄");
}

/* ************************************************************************** */
/*                                                                            */
/*                                                       ::::::::   ::::::::  */
/*    EditorContext.cpp                                :+:    :+: :+:    :+:  */
/*                                                    +:+        +:+          */
/*    By: Ser Superior <marcioeduine@gmail.com>      +#++:++#++ +#++:++#++    */
/*                                                         +#+        +#+     */
/*    Created: 2026/07/02 10:37:35 by Ser Superior #+#    #+# #+#    #+#      */
/*    Updated: 2026/09/09 16:50:00 by Ser Superior ########   ########        */
/*                                                                            */
/* ************************************************************************** */


#include <algorithm>
#include <dirent.h>
#include <fstream>

EditorContext::EditorContext(void) :
	active_file_idx(0),
	current_mode(MODE_EDITOR),
	screen_rows(24),
	screen_cols(80),
	drawer_open(false),
	current_dir("."),
	selected_file_idx(0),
	ai_panel_open(false),
	ai_panel_expanded(false),
	ai_scroll_offset(0),
	ai_current_input(""),
	ai_prompt_cursor(0),
	ai_full_response(""),
	model_modal_open(false),
	model_modal_selected_idx(0),
	model_modal_active_model(""),
	mouse_x(0),
	mouse_y(0),
	mouse_bstate(0),
	hover_x(-1),
	hover_y(-1),
	hover_target(""),
	should_quit(false)
{
	ai_response_lines.push_back("-- [SSBot CLI v1.0] ------------------------------------------");
	ai_response_lines.push_back("Intelligent assistant & SSCodeIDE CLI terminal active.");
	ai_response_lines.push_back("Type /help for IDE commands, /models for model selector modal.");
	ai_response_lines.push_back("Supports native shell commands prefixed with ! (e.g. !ls, !git status).");
	ai_response_lines.push_back("Use [Esc] to return to the editor or [Ctrl+G] to toggle.");
	ai_response_lines.push_back("--------------------------------------------------------------");
}

static t_text TrimCopyLocal(const t_text &s)
{
	size_t start = s.find_first_not_of(" \t\r\n");
	if (start == t_text::npos) return "";
	size_t end = s.find_last_not_of(" \t\r\n");
	return s.substr(start, end - start + 1);
}

void	EditorContext::OpenModelModal(void)
{
	model_modal_items.clear();
	model_modal_selected_idx = 0;
	model_modal_active_model = "";

	FILE *pipe = popen("./ss_ai_bridge.py --list-all 2>&1", "r");
	if (pipe)
	{
		char buffer[512];
		while (fgets(buffer, sizeof(buffer), pipe) != NULL)
		{
			t_text line = SanitizeForUi(buffer);
			if (line.find("ACTIVE_MODEL:") == 0)
			{
				model_modal_active_model = TrimCopyLocal(line.substr(13));
			}
			else if (!line.empty())
			{
				model_modal_items.push_back(line);
			}
		}
		pclose(pipe);
	}

	model_modal_open = true;
	current_mode = MODE_MODAL;
}

void	EditorContext::CloseModelModal(void)
{
	model_modal_open = false;
	current_mode = MODE_EDITOR;
}

void	EditorContext::OpenFileBuffer(const t_text &filename)
{
	FileBuffer		fb;
	std::ifstream	file;
	t_text			line;
	t_text			clean_name(filename);
	size_t			icon_sep(clean_name.find(' '));

	if (icon_sep != t_text::npos)
	{
		t_text icon_prefix = clean_name.substr(0, icon_sep);
		if (icon_prefix == ICON_FOLDER or icon_prefix == ICON_FILE or icon_prefix == "[DIR]" or icon_prefix == "[FILE]" or icon_prefix == ">" or icon_prefix == "v")
			clean_name = clean_name.substr(icon_sep + 1);
	}
	if (!clean_name.empty() and clean_name[clean_name.length() - 1] == '/')
		clean_name = clean_name.substr(0, clean_name.length() - 1);

	for (size_t i(0); i < open_files.size(); ++i)
	{
		if (open_files[i].name == clean_name)
		{
			active_file_idx = static_cast<int>(i);
			if (open_files[i].lines.empty()) open_files[i].lines.push_back("");
			if (open_files[i].cursor_y >= static_cast<int>(open_files[i].lines.size()))
				open_files[i].cursor_y = static_cast<int>(open_files[i].lines.size() - 1);
			if (open_files[i].cursor_y < 0) open_files[i].cursor_y = 0;
			return ;
		}
	}

	fb.name = clean_name;
	fb.cursor_x = 0;
	fb.cursor_y = 0;
	fb.row_offset = 0;
	fb.is_dirty = false;

	file.open(clean_name.c_str());
	if (file.is_open())
	{
		while (std::getline(file, line))
			fb.lines.push_back(line);
		file.close();
	}
	if (fb.lines.empty())
		fb.lines.push_back("");

	open_files.push_back(fb);
	active_file_idx = static_cast<int>(open_files.size() - 1);
}

static void BuildLegacyFileList(EditorContext &ctx)
{
	ctx.file_list.clear();
	for (size_t i = 0; i < ctx.explorer_items.size(); ++i)
	{
		const ExplorerItem &item = ctx.explorer_items[i];
		t_text indent = "";
		for (int d = 0; d < item.depth; ++d)
			indent += "  ";
		if (item.is_dir)
		{
			t_text arrow = item.is_expanded ? "▼ " : "▶ ";
			ctx.file_list.push_back(indent + arrow + ICON_FOLDER + " " + item.name + "/");
		}
		else
		{
			ctx.file_list.push_back(indent + "  " + ICON_FILE + " " + item.name);
		}
	}
}

void	EditorContext::LoadDirectoryFiles(const t_text &dir_path)
{
	current_dir = dir_path;
	explorer_items.clear();

	DIR				*dir = opendir(current_dir.c_str());
	struct dirent	*entity;

	if (dir == NULL)
		return ;

	if (current_dir != "." and current_dir != "./")
	{
		ExplorerItem parent_item;
		parent_item.name = ".. (Up)";
		size_t last_slash = current_dir.find_last_of('/');
		if (last_slash != t_text::npos and last_slash > 0)
			parent_item.path = current_dir.substr(0, last_slash);
		else
			parent_item.path = ".";
		parent_item.is_dir = true;
		parent_item.is_expanded = false;
		parent_item.depth = 0;
		explorer_items.push_back(parent_item);
	}

	std::vector<ExplorerItem> dirs;
	std::vector<ExplorerItem> files;

	while ((entity = readdir(dir)) != NULL)
	{
		t_text name(entity->d_name);
		if (name == "." or name == "..")
			continue ;
		ExplorerItem item;
		item.name = name;
		if (current_dir == "." or current_dir == "./")
			item.path = name;
		else
			item.path = current_dir + "/" + name;
		item.is_dir = (entity->d_type == DT_DIR);
		item.is_expanded = false;
		item.depth = 0;
		if (item.is_dir) dirs.push_back(item);
		else files.push_back(item);
	}
	closedir(dir);

	auto comp = [](const ExplorerItem &a, const ExplorerItem &b) {
		return a.name < b.name;
	};
	std::sort(dirs.begin(), dirs.end(), comp);
	std::sort(files.begin(), files.end(), comp);

	for (size_t i = 0; i < dirs.size(); ++i) explorer_items.push_back(dirs[i]);
	for (size_t i = 0; i < files.size(); ++i) explorer_items.push_back(files[i]);

	BuildLegacyFileList(*this);

	if (selected_file_idx >= static_cast<int>(explorer_items.size()))
		selected_file_idx = 0;
}

void	EditorContext::ToggleDirectoryExpand(int index)
{
	if (index < 0 or index >= static_cast<int>(explorer_items.size()))
		return ;

	ExplorerItem &item = explorer_items[index];
	if (!item.is_dir)
		return ;

	if (item.name == ".. (Up)")
	{
		LoadDirectoryFiles(item.path);
		return ;
	}

	if (item.is_expanded)
	{
		item.is_expanded = false;
		int remove_start = index + 1;
		int remove_count = 0;
		while (remove_start + remove_count < static_cast<int>(explorer_items.size()) and
			   explorer_items[remove_start + remove_count].depth > item.depth)
		{
			remove_count++;
		}
		if (remove_count > 0)
			explorer_items.erase(explorer_items.begin() + remove_start, explorer_items.begin() + remove_start + remove_count);
	}
	else
	{
		item.is_expanded = true;
		DIR *dir = opendir(item.path.c_str());
		if (dir)
		{
			struct dirent *entity;
			std::vector<ExplorerItem> sub_dirs;
			std::vector<ExplorerItem> sub_files;
			while ((entity = readdir(dir)) != NULL)
			{
				t_text name(entity->d_name);
				if (name == "." or name == "..") continue ;
				ExplorerItem sub_item;
				sub_item.name = name;
				sub_item.path = item.path + "/" + name;
				sub_item.is_dir = (entity->d_type == DT_DIR);
				sub_item.is_expanded = false;
				sub_item.depth = item.depth + 1;
				if (sub_item.is_dir) sub_dirs.push_back(sub_item);
				else sub_files.push_back(sub_item);
			}
			closedir(dir);

			auto comp = [](const ExplorerItem &a, const ExplorerItem &b) {
				return a.name < b.name;
			};
			std::sort(sub_dirs.begin(), sub_dirs.end(), comp);
			std::sort(sub_files.begin(), sub_files.end(), comp);

			int insert_idx = index + 1;
			for (size_t i = 0; i < sub_dirs.size(); ++i)
			{
				explorer_items.insert(explorer_items.begin() + insert_idx, sub_dirs[i]);
				insert_idx++;
			}
			for (size_t i = 0; i < sub_files.size(); ++i)
			{
				explorer_items.insert(explorer_items.begin() + insert_idx, sub_files[i]);
				insert_idx++;
			}
		}
	}

	BuildLegacyFileList(*this);
}

void	EditorContext::AddCliMessage(const t_text &prefix, const t_text &message)
{
	t_text clean = SanitizeForUi(message);
	t_text line = "";

	if (!prefix.empty())
		line = prefix + " " + clean;
	else
		line = clean;

	// Divide mensagens de múltiplas linhas
	size_t start = 0;
	while (start < line.length())
	{
		size_t next_nl = line.find('\n', start);
		if (next_nl == t_text::npos)
		{
			ai_response_lines.push_back(line.substr(start));
			break ;
		}
		ai_response_lines.push_back(line.substr(start, next_nl - start));
		start = next_nl + 1;
	}
	ScrollCliToBottom();
}

void	EditorContext::ScrollCliToBottom(void)
{
	int view_lines = 8;
	int total = static_cast<int>(ai_response_lines.size());
	if (total > view_lines)
		ai_scroll_offset = total - view_lines;
	else
		ai_scroll_offset = 0;
}

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

/* ************************************************************************** */
/*                                                                            */
/*                                                       ::::::::   ::::::::  */
/*    TerminalManager.cpp                              :+:    :+: :+:    :+:  */
/*                                                    +:+        +:+          */
/*    By: Ser Superior <marcioeduine@gmail.com>      +#++:++#++ +#++:++#++    */
/*                                                         +#+        +#+     */
/*    Created: 2026/07/02 10:37:56 by Ser Superior #+#    #+# #+#    #+#      */
/*    Updated: 2026/09/09 15:10:00 by Ser Superior ########   ########        */
/*                                                                            */
/* ************************************************************************** */


#include <cstdlib>
#include <chrono>

static bool	g_ncurses_initialized = false;

static void	StaticCleanup(void)
{
	if (g_ncurses_initialized)
	{
		printf("\033[?1000l\033[?1002l\033[?1006l");
		fflush(stdout);
		endwin();
		g_ncurses_initialized = false;
	}
}

void	TerminalManager::GetTerminalSize(EditorContext &ctx)
{
	getmaxyx(stdscr, ctx.screen_rows, ctx.screen_cols);
	if (ctx.screen_rows <= 0) ctx.screen_rows = 24;
	if (ctx.screen_cols <= 0) ctx.screen_cols = 80;
}

void	TerminalManager::Init(EditorContext &ctx)
{
	setlocale(LC_ALL, "");
	initscr();
	raw();
	noecho();
	keypad(stdscr, TRUE);
	set_escdelay(25);
	curs_set(1);
	mousemask(0, NULL);
	printf("\033[?1000l\033[?1002l\033[?1003l\033[?1006l");
	fflush(stdout);

	g_ncurses_initialized = true;
	atexit(StaticCleanup);

	GetTerminalSize(ctx);
}

void	TerminalManager::Shutdown(void)
{
	StaticCleanup();
}

int		TerminalManager::ReadKey(EditorContext &ctx)
{
	wint_t		wch = 0;
	int			ret = get_wch(&wch);

	if (ret == KEY_CODE_YES)
	{
		if (wch == KEY_MOUSE)
		{
			MEVENT	event;
			if (getmouse(&event) == OK)
			{
				if (event.bstate & BUTTON4_PRESSED)
					return (KEY_MOUSE_SCROLL_UP);
				if (event.bstate & BUTTON5_PRESSED)
					return (KEY_MOUSE_SCROLL_DOWN);
				if (event.bstate & (BUTTON1_CLICKED | BUTTON1_PRESSED | BUTTON1_RELEASED | BUTTON1_DOUBLE_CLICKED))
				{
					static int last_cx = -1;
					static int last_cy = -1;
					static std::chrono::steady_clock::time_point last_click_tp;

					auto now = std::chrono::steady_clock::now();
					auto diff_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_click_tp).count();

					if (event.x == last_cx and event.y == last_cy and diff_ms < 150)
						return (KEY_MOUSE_IGNORE);

					last_cx = event.x;
					last_cy = event.y;
					last_click_tp = now;

					ctx.mouse_x = event.x;
					ctx.mouse_y = event.y;
					ctx.mouse_bstate = event.bstate;
					return (KEY_MOUSE_CLICK);
				}
			}
			return (KEY_MOUSE_IGNORE);
		}
		if (wch == KEY_BACKSPACE)
			return (127);
		return (static_cast<int>(wch));
	}
	else if (ret == OK)
	{
		if (wch == 127 or wch == 8)
			return (127);
		if (wch == '\n' or wch == '\r')
			return ('\r');
		return (static_cast<int>(wch));
	}
	return (0);
}

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
