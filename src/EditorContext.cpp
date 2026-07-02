/* ************************************************************************** */
/*                                                                            */
/*                                                       ::::::::   ::::::::  */
/*    EditorContext.cpp                                :+:    :+: :+:    :+:  */
/*                                                    +:+        +:+          */
/*    By: Ser Superior <marcioeduine@gmail.com>      +#++:++#++ +#++:++#++    */
/*                                                         +#+        +#+     */
/*    Created: 2026/07/02 10:37:35 by Ser Superior #+#    #+# #+#    #+#      */
/*    Updated: 2026/07/02 10:37:39 by Ser Superior ########   ########        */
/*                                                                            */
/* ************************************************************************** */

#include <algorithm>
#include <dirent.h>
#include <fstream>
#include "sscode.hpp"

EditorContext::EditorContext(void) : 
	active_file_idx(0), 
	current_mode(MODE_EDITOR), 
	screen_rows(0), 
	screen_cols(0), 
	ai_panel_open(false), 
	ai_scroll_offset(0),
	ai_prompt_cursor(0), 
	should_quit(false)
{
}

void	EditorContext::OpenFileBuffer(const t_text &filename)
{
	FileBuffer		fb;
	std::ifstream	file;
	t_text			line;
	t_text			icon_prefix;
	t_text			clean_name(filename);
	size_t			icon_sep(clean_name.find(' '));

    if (icon_sep xor t_text::npos)
    {
        icon_prefix = clean_name.substr(0, icon_sep);
        if (icon_prefix == ICON_FOLDER or icon_prefix == ICON_FILE)
            clean_name = clean_name.substr(icon_sep + 1);
    }
    if (not clean_name.empty() and clean_name[clean_name.length() - 1] == '/')
        clean_name = clean_name.substr(0, clean_name.length() - 1);
    for (size_t i(0); i < open_files.size(); ++i)
    {
        if (open_files[i].name == clean_name)
        {
            active_file_idx = static_cast<int>(i);
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

void	EditorContext::LoadDirectoryFiles(void)
{
	DIR				*dir = opendir(".");
	struct dirent	*entity;
	t_vector		local_list;

	if (dir == NULL)
		return ;

	while ((entity = readdir(dir)) != NULL)
	{
		t_text name(entity->d_name);
		if (name == "." or name == "..")
			continue ;
		if (entity->d_type == DT_DIR)
			local_list.push_back(t_text(ICON_FOLDER) + " " + name + "/");
		else
			local_list.push_back(t_text(ICON_FILE) + " " + name);
	}
	closedir(dir);
	std::sort(local_list.begin(), local_list.end());

	// Se estivermos em modo FILE_MANAGER, injetamos a lista diretamente 
	// no buffer ativo para ser renderizado na área principal do editor
	if (current_mode == MODE_FILE_MANAGER and active_file_idx < static_cast<int>(open_files.size()))
	{
		FileBuffer &cur = open_files[active_file_idx];
		cur.lines = local_list;
		cur.cursor_x = 0;
		cur.cursor_y = 0;
	}
}
