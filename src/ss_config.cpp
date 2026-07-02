#include "ss_code_ide.hpp"
#include <fstream>
#include <iostream>
#include <sstream>

static Color	parseHexColor(const t_text& hexStr)
{
	t_text			cleanStr(hexStr);
	unsigned long	val(0);
	unsigned char	r(255);
	unsigned char	g(255);
	unsigned char	b(255);
	unsigned char	a(255);

	if (!cleanStr.empty() && cleanStr[0] == '#')
		cleanStr = cleanStr.substr(1);
	if (cleanStr.length() >= 6)
	{
		val = std::stoul(cleanStr, NULL, 16);
		if (cleanStr.length() == 8)
		{
			r = (val >> 24) & 0xFF;
			g = (val >> 16) & 0xFF;
			b = (val >> 8) & 0xFF;
			a = val & 0xFF;
		}
		else
		{
			r = (val >> 16) & 0xFF;
			g = (val >> 8) & 0xFF;
			b = val & 0xFF;
			a = 255;
		}
	}
	return (Color{r, g, b, a});
}

static t_text	trim(const t_text& str)
{
	size_t	first(str.find_first_not_of(" \t\r\n"));
	size_t	last(0);

	if (first == std::string::npos)
		return ("");
	last = str.find_last_not_of(" \t\r\n");
	return (str.substr(first, (last - first + 1)));
}

static void	writeDefaultConfig(const t_text& filepath)
{
	std::ofstream	file(filepath.c_str());

	if (file.is_open())
	{
		file << "# SSCodeIDE Configuration File\n";
		file << "font = FiraCode-Regular.ttf\n";
		file << "font_bold = FiraCode-Bold.ttf\n";
		file << "font_size = 18\n";
		file << "font_spacing = 1.0\n";
		file << "vertical_stride = 24\n";
		file << "start_x = 20\n";
		file << "start_y = 25\n";
		file << "tab_size = 4\n";
		file << "cursor_trail = 1\n";
		file << "cursor_trail_decay_min = 0.1\n";
		file << "cursor_trail_decay_max = 0.4\n";
		file << "cursor_trail_threshold = 2.0\n";
		file << "cursor_trail_decay_speed = 3.0\n";
		file << "bg_color = #000000e6\n";
		file << "text_color = #cdd6f4ff\n";
		file << "comment_color = #9a9996ff\n";
		file << "keyword_color = #e0569fff\n";
		file << "type_color = #5bc8afff\n";
		file << "string_color = #ffa348ff\n";
		file << "cursor_color = #f5c2e7ff\n";
		file << "selection_color = #cdd6f428\n";
		file << "padding_vertical = 60\n";
		file << "padding_horizontal = 20\n";
		file.close();
	}
}

void	loadConfig(Config& config, const t_text& filepath)
{
	std::ifstream	file(filepath.c_str());
	t_text			line("");
	size_t			equalPos(0);
	t_text			key("");
	t_text			value("");

	// Load defaults
	config.fontPath = "FiraCode-Regular.ttf";
	config.fontBoldPath = "FiraCode-Bold.ttf";
	config.fontSize = 18;
	config.fontSpacing = 1.0f;
	config.verticalStride = 24;
	config.startX = 20;
	config.startY = 25;
	config.tabSize = 4;
	config.cursorTrail = 1;
	config.cursorTrailDecayMin = 0.1f;
	config.cursorTrailDecayMax = 0.4f;
	config.cursorTrailThreshold = 2.0f;
	config.cursorTrailDecaySpeed = 3.0f;
	config.bgColor = parseHexColor("#000000e6");
	config.textColor = parseHexColor("#cdd6f4ff");
	config.commentColor = parseHexColor("#9a9996ff");
	config.keywordColor = parseHexColor("#e0569fff");
	config.typeColor = parseHexColor("#5bc8afff");
	config.stringColor = parseHexColor("#ffa348ff");
	config.cursorColor = parseHexColor("#f5c2e7ff");
	config.selectionColor = parseHexColor("#cdd6f428");
	config.paddingVertical = 60;
	config.paddingHorizontal = 20;

	if (!file.is_open())
	{
		writeDefaultConfig(filepath);
		return;
	}

	while (std::getline(file, line))
	{
		line = trim(line);
		if (line.empty() || line[0] == '#')
			continue;
		equalPos = line.find('=');
		if (equalPos == std::string::npos)
			continue;
		key = trim(line.substr(0, equalPos));
		value = trim(line.substr(equalPos + 1));

		if (key == "font")
			config.fontPath = value;
		else if (key == "font_bold")
			config.fontBoldPath = value;
		else if (key == "font_size")
			config.fontSize = std::stoi(value);
		else if (key == "font_spacing")
			config.fontSpacing = std::stof(value);
		else if (key == "vertical_stride")
			config.verticalStride = std::stoi(value);
		else if (key == "start_x")
			config.startX = std::stoi(value);
		else if (key == "start_y")
			config.startY = std::stoi(value);
		else if (key == "tab_size")
			config.tabSize = std::stoi(value);
		else if (key == "cursor_trail")
			config.cursorTrail = std::stoi(value);
		else if (key == "cursor_trail_decay_min")
			config.cursorTrailDecayMin = std::stof(value);
		else if (key == "cursor_trail_decay_max")
			config.cursorTrailDecayMax = std::stof(value);
		else if (key == "cursor_trail_threshold")
			config.cursorTrailThreshold = std::stof(value);
		else if (key == "cursor_trail_decay_speed")
			config.cursorTrailDecaySpeed = std::stof(value);
		else if (key == "bg_color")
			config.bgColor = parseHexColor(value);
		else if (key == "text_color")
			config.textColor = parseHexColor(value);
		else if (key == "comment_color")
			config.commentColor = parseHexColor(value);
		else if (key == "keyword_color")
			config.keywordColor = parseHexColor(value);
		else if (key == "type_color")
			config.typeColor = parseHexColor(value);
		else if (key == "string_color")
			config.stringColor = parseHexColor(value);
		else if (key == "cursor_color")
			config.cursorColor = parseHexColor(value);
		else if (key == "selection_color")
			config.selectionColor = parseHexColor(value);
		else if (key == "padding_vertical")
			config.paddingVertical = std::stoi(value);
		else if (key == "padding_horizontal")
			config.paddingHorizontal = std::stoi(value);
	}
	file.close();
}
