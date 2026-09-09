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

#include "sscode.hpp"
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
