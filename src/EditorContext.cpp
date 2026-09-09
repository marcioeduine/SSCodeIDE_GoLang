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

#include "sscode.hpp"
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
