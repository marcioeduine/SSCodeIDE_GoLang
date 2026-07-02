#ifndef SS_CODE_IDE_HPP
#define SS_CODE_IDE_HPP

#include <raylib.h>
#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <cmath>
#include <thread>
#include <mutex>
#include <atomic>
#include <filesystem>
#include <set>
#include <cctype>
#include <unistd.h>
#include <pty.h>
#include <utmp.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <sstream>

typedef std::string t_text;

struct	Token {
	t_text	text;
	Color	color;
	bool	isBold;
};

struct	TrailPoint {
	Vector2	pos;
	float	width;
	float	alpha;
};

enum	DialogState {
	STATE_NONE,
	STATE_OPEN,
	STATE_SAVE_AS
};

struct	Config {
	t_text	fontPath;
	t_text	fontBoldPath;
	int		fontSize;
	float	fontSpacing;
	int		verticalStride;
	int		startX;
	int		startY;
	int		tabSize;
	int		cursorTrail;
	float	cursorTrailDecayMin;
	float	cursorTrailDecayMax;
	float	cursorTrailThreshold;
	float	cursorTrailDecaySpeed;
	Color	bgColor;
	Color	textColor;
	Color	commentColor;
	Color	keywordColor;
	Color	typeColor;
	Color	stringColor;
	Color	cursorColor;
	Color	selectionColor;
	int		paddingVertical;
	int		paddingHorizontal;
};

struct	HistoryState {
	std::vector<t_text>	lines;
	size_t				cursorLine;
	size_t				cursorCol;
};

inline	Vector2 makeVector2(float x, float y)
{
	Vector2	tmp;

	return (tmp.x = x, tmp.y = y, tmp);
}

const int	WINDOW_WIDTH(950);
const int	WINDOW_HEIGHT(650);

// UI Prototypes
float	SnapToPixel(float coord, float scale);
Vector2	SnapToPixel(Vector2 pos, float scale);
void	drawTitleBarButtons(Vector2 mousePos, int windowWidth);
void	drawBottomDialog(Font textFont, int fontSize, float fontSpacing, const t_text& dialogInput, DialogState dialogState, Color cursorColour, Color textColour, int windowHeight, int windowWidth);

// Lexer Prototypes
std::vector<Token>	tokenize(const std::string& line, Color keywordColour, Color typeColour, Color stringColour, Color commentColour, Color textColour);

// Config Prototypes
void	loadConfig(Config& config, const t_text& filepath);

// Editor Prototypes
size_t	findWordBoundaryLeft(const t_text& text, size_t col);
size_t	findWordBoundaryRight(const t_text& text, size_t col);
void	getSelectionBounds(size_t startLine, size_t startCol, size_t endLine, size_t endCol, size_t& minL, size_t& minC, size_t& maxL, size_t& maxC);
void	deleteSelectedText(std::vector<t_text>& lines, size_t& cursorLine, size_t& cursorCol, size_t startLine, size_t startCol, size_t endLine, size_t endCol);
t_text	getSelectedText(const std::vector<t_text>& lines, size_t startLine, size_t startCol, size_t endLine, size_t endCol);
void	handleSelectionKey(bool shiftDown, bool& selectionActive, size_t& startLine, size_t& startCol, size_t& endLine, size_t& endCol, size_t cursorLine, size_t cursorCol);
void	addCursorTrailPoints(std::vector<TrailPoint>& trail, Vector2 start, Vector2 end, int steps, float width = 2.0f, bool isHorizontal = false);
void	insertOrUpdateHeader(std::vector<t_text>& lines, const t_text& filepath, size_t& cursorLine);
float	calculateTextWidth(Font font, Font fontBold, const t_text& text, int tabSize, float fontSize, float fontSpacing, size_t colLimit, Color keywordColor, Color typeColor, Color stringColor, Color commentColor, Color textColor);

#endif
