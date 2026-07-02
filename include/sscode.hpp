/* ************************************************************************** */
/*                                                                            */
/*                                                       ::::::::   ::::::::  */
/*    sscode.hpp                                       :+:    :+: :+:    :+:  */
/*                                                    +:+        +:+          */
/*    By: Ser Superior <marcioeduine@gmail.com>      +#++:++#++ +#++:++#++    */
/*                                                         +#+        +#+     */
/*    Created: 2026/07/02 10:15:56 by Ser Superior #+#    #+# #+#    #+#      */
/*    Updated: 2026/07/02 10:26:01 by Ser Superior ########   ########        */
/*                                                                            */
/* ************************************************************************** */

#ifndef SSCODE_HPP
# define SSCODE_HPP

# include <iostream>
# include <string>
# include <termios.h>
# include <vector>

// --- CORES ANSI RGB 24-BITS ---
# define RGB_BG_DARK        "\x1b[48;2;13;17;23m"
# define RGB_TAB_BG         "\x1b[48;2;22;27;34m"
# define RGB_TAB_ACT        "\x1b[48;2;47;129;247;38;2;255;255;255;1m"
# define RGB_TEXT           "\x1b[38;2;230;237;243m"
# define RGB_GUTTER         "\x1b[48;2;13;17;23;38;2;110;118;129m"
# define RGB_HEADER_BG      "\x1b[48;2;22;27;34m"
# define RGB_HEADER_ACCENT  "\x1b[38;2;235;160;0;1m"
# define RGB_STATUS_BG      "\x1b[48;2;47;129;247m"
# define RGB_STATUS_TEXT    "\x1b[38;2;255;255;255;1m"
# define RGB_AI_PANEL       "\x1b[48;2;20;20;30m"
# define RGB_AI_TEXT        "\x1b[38;2;140;180;255m"

// --- CORES HIGHLIGHT SYNTAX ---
# define RGB_KEYWORD        "\x1b[38;2;255;123;114;1m"
# define RGB_DATATYPE       "\x1b[38;2;121;192;255;1m"
# define RGB_NUMBER         "\x1b[38;2;110;213;106m"
# define RGB_STRING         "\x1b[38;2;165;214;255m"
# define RGB_COMMENT        "\x1b[38;2;139;148;158;3m"
# define RGB_PREPROC        "\x1b[38;2;255;161;151m"
# define RGB_RESET          "\x1b[0m"

// --- ÍCONES ---
# define ICON_FOLDER        "📁"
# define ICON_FILE          "📄"

# define CTRL_KEY(k)        ((k) & 0x1f)

enum e_editorMode
{
	MODE_EDITOR,
	MODE_FILE_MANAGER,
	MODE_AI_PROMPT
};

enum e_specialKeys
{
	ARROW_LEFT = 1000,
	ARROW_RIGHT,
	ARROW_UP,
	ARROW_DOWN,
	HOME_KEY,
	END_KEY,
	PAGE_UP,
	PAGE_DOWN,
	DEL_KEY,
	MOUSE_SCROLL_UP,
	MOUSE_SCROLL_DOWN,
	MOUSE_EVENT_IGNORE
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

class	EditorContext
{
	public:
		std::vector<FileBuffer>	open_files;
		int						active_file_idx;
		e_editorMode			current_mode;
		int						screen_rows;
		int						screen_cols;
		
		// Painel de IA e Chat
		bool					ai_panel_open;
		t_vector				ai_response_lines;
		int						ai_scroll_offset;
		t_text					ai_current_input;
		int						ai_prompt_cursor;
		t_text					ai_full_response;
		
		bool					should_quit;
		t_vector				clipboard;

		EditorContext(void);
		void	OpenFileBuffer(const t_text &filename);
		void	LoadDirectoryFiles(void);
		void	SaveFile(void);
		void	SaveUndoState(FileBuffer &fb);
		void	ScrollEditor(void);
};

class	TerminalManager
{
	private:
		struct termios	orig_termios;

	public:
		void			EnableRawMode(EditorContext &ctx);
		void			DisableRawMode(void);
		void			GetTerminalSize(EditorContext &ctx);
		unsigned int	ReadKey(void);
};

class	RendererEngine
{
	public:
		void	RefreshScreen(EditorContext &ctx);
		void	RenderHighlightedLine(const t_text &line, int max_width);
		void	WrapText(const t_text &text, int max_width, t_vector &output);
};

#endif
