#include "ss_code_ide.hpp"
#include <ctime>
#include <cmath>
#include <cstdlib>

size_t	findWordBoundaryLeft(const t_text& text, size_t col)
{
	size_t	i(col);

	if (i == 0)
		return (0);
	i--;
	if (std::isalnum(text[i]) || text[i] == '_')
		while (i > 0 && (std::isalnum(text[i - 1]) || text[i - 1] == '_'))
			i--;
	else if (std::isspace(text[i]))
		while (i > 0 && std::isspace(text[i - 1]))
			i--;
	else
		while (i > 0 && !std::isalnum(text[i - 1]) && text[i - 1] != '_' && !std::isspace(text[i - 1]))
			i--;
	return (i);
}

size_t	findWordBoundaryRight(const t_text& text, size_t col)
{
	size_t	i(col);
	size_t	len(text.length());

	if (i >= len)
		return (len);
	if (std::isalnum(text[i]) || text[i] == '_')
		while (i < len && (std::isalnum(text[i]) || text[i] == '_'))
			i++;
	else if (std::isspace(text[i]))
		while (i < len && std::isspace(text[i]))
			i++;
	else
		while (i < len && !std::isalnum(text[i]) && text[i] != '_' && !std::isspace(text[i]))
			i++;
	return (i);
}

void	getSelectionBounds(size_t startLine, size_t startCol, size_t endLine, size_t endCol, size_t& minL, size_t& minC, size_t& maxL, size_t& maxC)
{
	if (startLine < endLine)
	{
		minL = startLine;
		minC = startCol;
		maxL = endLine;
		maxC = endCol;
	}
	else if (startLine > endLine)
	{
		minL = endLine;
		minC = endCol;
		maxL = startLine;
		maxC = startCol;
	}
	else
	{
		minL = startLine;
		maxL = startLine;
		if (startCol < endCol)
		{
			minC = startCol;
			maxC = endCol;
		}
		else
		{
			minC = endCol;
			maxC = startCol;
		}
	}
}

void	deleteSelectedText(std::vector<t_text>& lines, size_t& cursorLine, size_t& cursorCol, size_t startLine, size_t startCol, size_t endLine, size_t endCol)
{
	size_t	minL(0);
	size_t	minC(0);
	size_t	maxL(0);
	size_t	maxC(0);

	getSelectionBounds(startLine, startCol, endLine, endCol, minL, minC, maxL, maxC);
	if (minL == maxL)
		lines[minL] = lines[minL].substr(0, minC) + lines[minL].substr(maxC);
	else
	{
		lines[minL] = lines[minL].substr(0, minC) + lines[maxL].substr(maxC);
		lines.erase(lines.begin() + minL + 1, lines.begin() + maxL + 1);
	}
	cursorLine = minL;
	cursorCol = minC;
}

t_text	getSelectedText(const std::vector<t_text>& lines, size_t startLine, size_t startCol, size_t endLine, size_t endCol)
{
	size_t	minL(0);
	size_t	minC(0);
	size_t	maxL(0);
	size_t	maxC(0);
	t_text	result("");
	size_t	l(0);

	getSelectionBounds(startLine, startCol, endLine, endCol, minL, minC, maxL, maxC);
	if (minL == maxL)
		return (lines[minL].substr(minC, maxC - minC));
	result = lines[minL].substr(minC) + "\n";
	for (l = minL + 1; l < maxL; ++l)
		result += lines[l] + "\n";
	result += lines[maxL].substr(0, maxC);
	return (result);
}

void	handleSelectionKey(bool shiftDown, bool& selectionActive, size_t& startLine, size_t& startCol, size_t& endLine, size_t& endCol, size_t cursorLine, size_t cursorCol)
{
	if (shiftDown)
	{
		if (!selectionActive)
		{
			selectionActive = true;
			startLine = cursorLine;
			startCol = cursorCol;
		}
		endLine = cursorLine;
		endCol = cursorCol;
	}
	else
		selectionActive = false;
}

void	addCursorTrailPoints(std::vector<TrailPoint>& trail, Vector2 start, Vector2 end, int steps, float width, bool isHorizontal)
{
	int		step(0);
	float	t(0.0f);
	Vector2	pos(makeVector2(0.0f, 0.0f));

	(void)isHorizontal;
	for (step = 1; step < steps; ++step)
	{
		t = static_cast<float>(step) / static_cast<float>(steps);
		pos.x = start.x + (end.x - start.x) * t;
		pos.y = start.y + (end.y - start.y) * t;
		trail.push_back(TrailPoint{pos, width, t});
	}
}

static t_text	getCurrentDateTime(void)
{
	time_t		now(time(NULL));
	tm			tstruct(*localtime(&now));
	char		buf[80];

	strftime(buf, sizeof(buf), "%Y/%m/%d %H:%M:%S", &tstruct);
	return (t_text(buf));
}

static t_text	formatFilename(const t_text& filename, size_t maxLength)
{
	t_text	fn(filename);
	int		padLen(0);

	if (fn.length() > maxLength)
		fn = fn.substr(0, maxLength - 3) + "...";
	padLen = static_cast<int>(maxLength) - static_cast<int>(fn.length());
	if (padLen < 0)
		padLen = 0;
	return (fn + std::string(padLen, ' '));
}

static t_text	padLine(const t_text& leftText, const t_text& logoPart, size_t targetRightWidth, const t_text& lc, const t_text& rc)
{
	size_t	rcCalcLen(rc.empty() ? 0 : (rc.back() == ' ' ? rc.length() - 1 : rc.length()));
	int		spacesAfterLogo(static_cast<int>(targetRightWidth) - static_cast<int>(logoPart.length()) - static_cast<int>(rcCalcLen));
	t_text	rightPart("");
	int		leftSpaceAvailable(0);
	t_text	leftTextTruncated("");
	int		padLen(0);
	t_text	result("");

	if (spacesAfterLogo < 0)
		spacesAfterLogo = 0;
	rightPart = logoPart + std::string(spacesAfterLogo, ' ') + rc;
	leftSpaceAvailable = 80 - (static_cast<int>(rightPart.length()) - (rc.length() - rcCalcLen)) - static_cast<int>(lc.length());
	leftTextTruncated = leftText;
	if (static_cast<int>(leftTextTruncated.length()) > leftSpaceAvailable)
		leftTextTruncated = leftTextTruncated.substr(0, leftSpaceAvailable);
	padLen = leftSpaceAvailable - static_cast<int>(leftTextTruncated.length());
	if (padLen < 0)
		padLen = 0;
	result = lc + leftTextTruncated + std::string(padLen, ' ') + rightPart;
	return (result);
}

void	insertOrUpdateHeader(std::vector<t_text>& lines, const t_text& filepath, size_t& cursorLine)
{
	t_text	filename("stdin");
	size_t	slashPos(0);
	t_text	lc("# ");
	t_text	rc(" # ");
	size_t	maxCheck(0);
	bool	hasHeader(false);
	size_t	i(0);
	t_text	currentTime(getCurrentDateTime());
	t_text	user("Ser Superior");
	t_text	email("<marcioeduine@gmail.com>");
	t_text	leftText("");
	t_text	updatedLine("");
	t_text	logo[7];
	size_t	internalAsterisks(0);
	t_text	border("");
	t_text	filenameFormatted("");
	t_text	prefixFilename("");
	t_text	prefixBy("");
	t_text	prefixCreated("");
	t_text	prefixUpdated("");
	std::vector<t_text>	header;

	logo[0] = "::::::::   ::::::::";
	logo[1] = ":+:    :+: :+:    :+:";
	logo[2] = "+:+        +:+";
	logo[3] = "+#++:++#++ +#++:++#++";
	logo[4] = "+#+        +#+";
	logo[5] = "#+#    #+# #+#    #+#";
	logo[6] = "########   ########";

	if (!filepath.empty())
	{
		slashPos = filepath.find_last_of("/\\");
		if (slashPos != std::string::npos)
			filename = filepath.substr(slashPos + 1);
		else
			filename = filepath;
	}

	if (filename.find("Makefile") != std::string::npos || filename.find(".py") != std::string::npos || filename.find(".sh") != std::string::npos || filename.find(".fish") != std::string::npos)
	{
		lc = "# ";
		rc = " # ";
		internalAsterisks = 76;
		prefixFilename = "    ";
		prefixBy = "    ";
		prefixCreated = "    ";
		prefixUpdated = "    ";
	}
	else
	{
		lc = "/* ";
		rc = " */";
		internalAsterisks = 74;
		prefixFilename = "    ";
		prefixBy = "   ";
		prefixCreated = "   ";
		prefixUpdated = "   ";
	}

	maxCheck = (lines.size() < 12) ? lines.size() : 12;
	for (i = 0; i < maxCheck; ++i)
		if (lines[i].find("Created:") != std::string::npos)
			hasHeader = true;

	if (hasHeader)
	{
		for (i = 0; i < maxCheck; ++i)
		{
			if (lines[i].find("Updated:") != std::string::npos)
			{
				leftText = prefixUpdated + "Updated: " + currentTime + " by " + user;
				updatedLine = padLine(leftText, logo[6], 29, lc, rc);
				lines[i] = updatedLine;
				break;
			}
		}
	}
	else
	{
		border = lc + std::string(internalAsterisks, '*') + rc;
		filenameFormatted = formatFilename(filename, 42);

		header.push_back(border);
		header.push_back(padLine("", "", 0, lc, rc));
		header.push_back(padLine("", logo[0], 23, lc, rc));
		header.push_back(padLine(prefixFilename + filenameFormatted, logo[1], 25, lc, rc));
		header.push_back(padLine("", logo[2], 26, lc, rc));
		header.push_back(padLine(prefixBy + "By: " + user + " " + email, logo[3], 27, lc, rc));
		header.push_back(padLine("", logo[4], 21, lc, rc));
		header.push_back(padLine(prefixCreated + "Created: " + currentTime + " by " + user, logo[5], 29, lc, rc));
		header.push_back(padLine(prefixUpdated + "Updated: " + currentTime + " by " + user, logo[6], 29, lc, rc));
		header.push_back(padLine("", "", 0, lc, rc));
		header.push_back(border);

		lines.insert(lines.begin(), header.begin(), header.end());
		cursorLine += 11;
	}
}

float	calculateTextWidth(Font font, Font fontBold, const t_text& text, int tabSize, float fontSize, float fontSpacing, size_t colLimit, Color keywordColor, Color typeColor, Color stringColor, Color commentColor, Color textColor)
{
	(void)fontBold;
	(void)keywordColor;
	(void)typeColor;
	(void)stringColor;
	(void)commentColor;
	(void)textColor;

	float		charWidth = MeasureTextEx(font, " ", fontSize, fontSpacing).x;
	float		currentX = 0.0f;
	const char*	ptr = text.c_str();
	size_t		byteIdx = 0;

	while (*ptr != '\0' && byteIdx < colLimit)
	{
		int bytesProcessed = 0;
		int codepoint = GetCodepointNext(ptr, &bytesProcessed);
		if (bytesProcessed == 0)
			break;

		if (codepoint == '\t')
		{
			float tabWidth = charWidth * tabSize;
			int currentTabStop = static_cast<int>(currentX / tabWidth);
			currentX = (currentTabStop + 1) * tabWidth;
		}
		else
		{
			currentX += charWidth;
		}
		ptr += bytesProcessed;
		byteIdx += bytesProcessed;
	}
	return currentX;
}
