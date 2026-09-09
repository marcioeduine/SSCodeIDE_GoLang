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
