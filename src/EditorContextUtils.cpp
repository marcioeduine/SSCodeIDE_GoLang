/* ************************************************************************** */
/*                                                                            */
/*                                                       ::::::::   ::::::::  */
/*    EditorContextUtils.cpp                           :+:    :+: :+:    :+:  */
/*                                                    +:+        +:+          */
/*    By: Ser Superior <marcioeduine@gmail.com>      +#++:++#++ +#++:++#++    */
/*                                                         +#+        +#+     */
/*    Created: 2026/07/02 10:15:56 by Ser Superior #+#    #+# #+#    #+#      */
/*    Updated: 2026/07/02 10:26:01 by Ser Superior ########   ########        */
/*                                                                            */
/* ************************************************************************** */

#include <fstream>
#include "sscode.hpp"

void	EditorContext::SaveFile(void)
{
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
	FileBuffer	&cur(open_files[active_file_idx]);
	int			edit_window_height(screen_rows - 4);
	int			ai_panel_height(ai_panel_open ? 8 : 0);

	(void)ai_panel_height;
	if (ai_panel_open)
		edit_window_height -= 8;
	if (edit_window_height < 1)
		edit_window_height = 1;
	if (cur.cursor_y < cur.row_offset)
		cur.row_offset = cur.cursor_y;
	if (cur.cursor_y >= cur.row_offset + edit_window_height)
		cur.row_offset = cur.cursor_y - edit_window_height + 1;
}
