#include "ss_code_ide.hpp"

float	SnapToPixel(float coord, float scale)
{
	return (std::round(coord * scale) / scale);
}

Vector2	SnapToPixel(Vector2 pos, float scale)
{
	Vector2	result;

	return (result.x = SnapToPixel(pos.x, scale), result.y = SnapToPixel(pos.y, scale), result);
}

void	drawTitleBarButtons(Vector2 mousePos, int windowWidth)
{
	// Close (Red), Minimize (Yellow), Maximize (Green) placed at the top-right
	float	closeX(static_cast<float>(windowWidth - 35));
	float	minimizeX(static_cast<float>(windowWidth - 55));
	float	maximizeX(static_cast<float>(windowWidth - 75));
	float	y(35.0f);
	float	radius(6.0f);
	Color	redColor(GetColor(0xff5f56ff));
	Color	yellowColor(GetColor(0xffbd2eff));
	Color	greenColor(GetColor(0x27c93fff));
	Color	symbolColor(GetColor(0x000000a0)); // subtle dark icon color
	bool	isHoverClose(CheckCollisionPointCircle(mousePos, makeVector2(closeX, y), radius + 2.0f));
	bool	isHoverMinimize(CheckCollisionPointCircle(mousePos, makeVector2(minimizeX, y), radius + 2.0f));
	bool	isHoverMaximize(CheckCollisionPointCircle(mousePos, makeVector2(maximizeX, y), radius + 2.0f));

	// Draw base circles
	DrawCircle(maximizeX, y, radius, greenColor);
	DrawCircle(minimizeX, y, radius, yellowColor);
	DrawCircle(closeX, y, radius, redColor);

	// Maximize Symbol (box outline or plus)
	if (isHoverMaximize)
		DrawRectangleLines(static_cast<int>(maximizeX - 3), static_cast<int>(y - 3), 6, 6, symbolColor);

	// Minimize Symbol (horizontal line)
	if (isHoverMinimize)
		DrawLine(static_cast<int>(minimizeX - 3), static_cast<int>(y), static_cast<int>(minimizeX + 3), static_cast<int>(y), symbolColor);

	// Close Symbol (diagonal cross)
	if (isHoverClose)
	{
		DrawLine(static_cast<int>(closeX - 3), static_cast<int>(y - 3), static_cast<int>(closeX + 3), static_cast<int>(y + 3), symbolColor);
		DrawLine(static_cast<int>(closeX - 3), static_cast<int>(y + 3), static_cast<int>(closeX + 3), static_cast<int>(y - 3), symbolColor);
	}
}

void	drawBottomDialog(Font textFont, int fontSize, float fontSpacing, const t_text& dialogInput, DialogState dialogState, Color cursorColour, Color textColour, int windowHeight, int windowWidth)
{
	Vector2	pos(makeVector2(20.0f, static_cast<float>(windowHeight - 65)));
	t_text	dialogPrompt("");

	DrawRectangleRounded(Rectangle{pos.x, pos.y, static_cast<float>(windowWidth - 40), 45.0f}, 0.2f, 8, GetColor(0x11111bfa));
	DrawRectangleRoundedLines(Rectangle{pos.x, pos.y, static_cast<float>(windowWidth - 40), 45.0f}, 0.2f, 8, cursorColour);
	dialogPrompt = (dialogState == STATE_NONE) ? "" : ((dialogState == STATE_OPEN) ? "Open File: " : "Save As: ");
	dialogPrompt += dialogInput;
	if (textFont.texture.id != 0)
		DrawTextEx(textFont, dialogPrompt.c_str(), makeVector2(35.0f, static_cast<float>(windowHeight - 53)), static_cast<float>(fontSize), fontSpacing, textColour);
	else
		DrawText(dialogPrompt.c_str(), 35, windowHeight - 53, fontSize, textColour);
}
