#include "ss_code_ide.hpp"

namespace fs = std::filesystem;

// PTY State variables
static int			masterFd(-1);
static pid_t		ptyPid(-1);

struct TermCell {
	char	c;
	Color	fg;
};

static std::vector<std::vector<TermCell>>	termGrid;
static int			termRows(24);
static int			termCols(80);
static int			termCursorRow(0);
static int			termCursorCol(0);
static Color		termActiveFg(WHITE);

enum AnsiState {
	ANSI_STATE_NORMAL,
	ANSI_STATE_ESC,
	ANSI_STATE_CSI
};

static AnsiState	termState(ANSI_STATE_NORMAL);
static t_text		csiParams("");

// AI Assistant state variables
struct ChatMessage {
	t_text	role;
	t_text	text;
};

static std::vector<ChatMessage>	chatHistory;
static std::mutex				chatMutex;
static std::atomic<bool>		isAiThinking(false);
static std::atomic<bool>		isAudioTranscribing(false);
static bool						isRecording(false);
static bool						isAiPanelOpen(false);
static bool						isAiFocused(false);
static t_text					aiInputPrompt("");
static float					aiScrollY(0.0f);

static void	scrollTerminal(void)
{
	for (int r = 0; r < termRows - 1; ++r)
		termGrid[r] = termGrid[r + 1];
	termGrid[termRows - 1].assign(termCols, TermCell{' ', WHITE});
}

static void	handleCsiCommand(char cmd, const t_text& params)
{
	if (cmd == 'm')
	{
		if (params.empty() || params == "0")
			termActiveFg = WHITE;
		else
		{
			std::stringstream	ss(params);
			t_text				item("");
			while (std::getline(ss, item, ';'))
			{
				if (item.empty())
					continue;
				try
				{
					int code = std::stoi(item);
					if (code == 0)
						termActiveFg = WHITE;
					else if (code >= 30 && code <= 37)
					{
						switch (code)
						{
							case 30: termActiveFg = BLACK; break;
							case 31: termActiveFg = RED; break;
							case 32: termActiveFg = GREEN; break;
							case 33: termActiveFg = YELLOW; break;
							case 34: termActiveFg = BLUE; break;
							case 35: termActiveFg = MAGENTA; break;
							case 36: termActiveFg = SKYBLUE; break;
							case 37: termActiveFg = WHITE; break;
						}
					}
					else if (code >= 90 && code <= 97)
					{
						switch (code)
						{
							case 90: termActiveFg = GRAY; break;
							case 91: termActiveFg = RED; break;
							case 92: termActiveFg = GREEN; break;
							case 93: termActiveFg = YELLOW; break;
							case 94: termActiveFg = BLUE; break;
							case 95: termActiveFg = MAGENTA; break;
							case 96: termActiveFg = SKYBLUE; break;
							case 97: termActiveFg = WHITE; break;
						}
					}
				}
				catch (...) {}
			}
		}
	}
	else if (cmd == 'J')
	{
		if (params == "2" || params.empty())
		{
			for (int r = 0; r < termRows; ++r)
				termGrid[r].assign(termCols, TermCell{' ', WHITE});
			termCursorRow = 0;
			termCursorCol = 0;
		}
	}
	else if (cmd == 'H' || cmd == 'f')
	{
		int		r(1);
		int		c(1);
		size_t	semi(params.find(';'));
		if (semi != std::string::npos)
		{
			try
			{
				r = std::stoi(params.substr(0, semi));
				c = std::stoi(params.substr(semi + 1));
			}
			catch (...) {}
		}
		else if (!params.empty())
		{
			try
			{
				r = std::stoi(params);
			}
			catch (...) {}
		}
		termCursorRow = std::max(0, std::min(termRows - 1, r - 1));
		termCursorCol = std::max(0, std::min(termCols - 1, c - 1));
	}
	else if (cmd == 'A')
	{
		int count = params.empty() ? 1 : std::stoi(params);
		termCursorRow = std::max(0, termCursorRow - count);
	}
	else if (cmd == 'B')
	{
		int count = params.empty() ? 1 : std::stoi(params);
		termCursorRow = std::min(termRows - 1, termCursorRow + count);
	}
	else if (cmd == 'C')
	{
		int count = params.empty() ? 1 : std::stoi(params);
		termCursorCol = std::min(termCols - 1, termCursorCol + count);
	}
	else if (cmd == 'D')
	{
		int count = params.empty() ? 1 : std::stoi(params);
		termCursorCol = std::max(0, termCursorCol - count);
	}
}

static void	processChar(char c)
{
	if (termState == ANSI_STATE_NORMAL)
	{
		if (c == '\x1b')
			termState = ANSI_STATE_ESC;
		else if (c == '\n')
		{
			termCursorRow++;
			if (termCursorRow >= termRows)
			{
				scrollTerminal();
				termCursorRow = termRows - 1;
			}
		}
		else if (c == '\r')
			termCursorCol = 0;
		else if (c == '\t')
		{
			termCursorCol = (termCursorCol / 8 + 1) * 8;
			if (termCursorCol >= termCols)
				termCursorCol = termCols - 1;
		}
		else if (c == '\b' || c == 127)
		{
			if (termCursorCol > 0)
				termCursorCol--;
		}
		else if (c >= 32 && c <= 126)
		{
			if (termCursorRow >= 0 && termCursorRow < termRows && termCursorCol >= 0 && termCursorCol < termCols)
			{
				termGrid[termCursorRow][termCursorCol].c = c;
				termGrid[termCursorRow][termCursorCol].fg = termActiveFg;
			}
			termCursorCol++;
			if (termCursorCol >= termCols)
			{
				termCursorCol = 0;
				termCursorRow++;
				if (termCursorRow >= termRows)
				{
					scrollTerminal();
					termCursorRow = termRows - 1;
				}
			}
		}
	}
	else if (termState == ANSI_STATE_ESC)
	{
		if (c == '[')
		{
			termState = ANSI_STATE_CSI;
			csiParams = "";
		}
		else
			termState = ANSI_STATE_NORMAL;
	}
	else if (termState == ANSI_STATE_CSI)
	{
		if (std::isalpha(c) || c == '~')
		{
			handleCsiCommand(c, csiParams);
			termState = ANSI_STATE_NORMAL;
		}
		else
			csiParams += c;
	}
}

static void	resizeTermGrid(int rows, int cols)
{
	termRows = rows;
	termCols = cols;
	termGrid.resize(rows);
	for (int r = 0; r < rows; ++r)
		termGrid[r].resize(cols, TermCell{' ', WHITE});
	if (termCursorRow >= rows)
		termCursorRow = rows - 1;
	if (termCursorCol >= cols)
		termCursorCol = cols - 1;
}

static void	initPty(int rows, int cols)
{
	struct winsize	ws;
	t_text			shellPath("/bin/bash");
	char*			envShell(getenv("SHELL"));

	if (envShell)
		shellPath = envShell;
	
	ws.ws_row = rows;
	ws.ws_col = cols;
	ws.ws_xpixel = 0;
	ws.ws_ypixel = 0;
	
	ptyPid = forkpty(&masterFd, NULL, NULL, &ws);
	if (ptyPid == 0)
	{
		setenv("TERM", "xterm-color", 1);
		execl(shellPath.c_str(), "bash", "--login", NULL);
		exit(1);
	}
	int flags = fcntl(masterFd, F_GETFL, 0);
	fcntl(masterFd, F_SETFL, flags | O_NONBLOCK);
}

static void	saveUndoState(std::vector<HistoryState>& undoStack, std::vector<HistoryState>& redoStack, const std::vector<t_text>& lines, size_t cursorLine, size_t cursorCol)
{
	HistoryState	state;

	state.lines = lines;
	state.cursorLine = cursorLine;
	state.cursorCol = cursorCol;
	undoStack.push_back(state);
	if (undoStack.size() > 100)
		undoStack.erase(undoStack.begin());
	redoStack.clear();
}

static std::set<t_text>	indexProjectWords(void)
{
	std::set<t_text>	words;
	t_text				ext("");
	t_text				content("");
	t_text				word("");
	char				c(0);
	size_t				i(0);

	try
	{
		for (const auto& entry : fs::recursive_directory_iterator("."))
		{
			if (entry.is_regular_file())
			{
				ext = entry.path().extension().string();
				if (ext == ".cpp" || ext == ".hpp" || ext == ".h" || ext == ".c" || ext == ".ss")
				{
					std::ifstream	file(entry.path().string());
					if (file.is_open())
					{
						content = t_text((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
						file.close();
						
						word = "";
						for (i = 0; i < content.length(); ++i)
						{
							c = content[i];
							if (std::isalnum(c) || c == '_')
								word += c;
							else
							{
								if (word.length() >= 3 && !std::isdigit(word[0]))
									words.insert(word);
								word = "";
							}
						}
						if (word.length() >= 3 && !std::isdigit(word[0]))
							words.insert(word);
					}
				}
			}
		}
	}
	catch (...) {}
	
	words.insert("iostream");
	words.insert("vector");
	words.insert("string");
	words.insert("fstream");
	words.insert("cmath");
	words.insert("cout");
	words.insert("endl");
	words.insert("main");
	words.insert("return");
	return (words);
}

// Text wrapping utility functions
static std::vector<t_text>	wrapLine(Font font, const t_text& text, float maxWidth, float fontSize, float spacing)
{
	std::vector<t_text>	lines;
	t_text				currentLine("");
	std::stringstream	ss(text);
	t_text				word("");

	while (ss >> word)
	{
		t_text testLine = currentLine.empty() ? word : currentLine + " " + word;
		float w = MeasureTextEx(font, testLine.c_str(), fontSize, spacing).x;
		if (w > maxWidth)
		{
			if (!currentLine.empty())
				lines.push_back(currentLine);
			currentLine = word;
		}
		else
			currentLine = testLine;
	}
	if (!currentLine.empty())
		lines.push_back(currentLine);
	if (lines.empty())
		lines.push_back("");
	return (lines);
}

static std::vector<t_text>	wrapText(Font font, const t_text& text, float maxWidth, float fontSize, float spacing)
{
	std::vector<t_text>	result;
	std::stringstream	ss(text);
	t_text				item("");

	while (std::getline(ss, item, '\n'))
	{
		std::vector<t_text> wrapped = wrapLine(font, item, maxWidth, fontSize, spacing);
		result.insert(result.end(), wrapped.begin(), wrapped.end());
	}
	return (result);
}

// AI prompt assembler ChatML
static t_text	buildChatPrompt(const std::vector<ChatMessage>& history, const t_text& currentPrompt)
{
	t_text	p("<|im_start|>system\nYou are a helpful C++ coding assistant integrated in the SSCodeIDE. Keep answers extremely short and concise.<|im_end|>\n");
	for (const auto& msg : history)
		p += "<|im_start|>" + msg.role + "\n" + msg.text + "<|im_end|>\n";
	if (!currentPrompt.empty())
		p += "<|im_start|>user\n" + currentPrompt + "<|im_end|>\n";
	p += "<|im_start|>assistant\n";
	return (p);
}

// Background LLM worker thread
static void	aiWorker(t_text userPrompt)
{
	t_text	fullPrompt("");
	{
		std::lock_guard<std::mutex>	lock(chatMutex);
		fullPrompt = buildChatPrompt(chatHistory, userPrompt);
		chatHistory.push_back(ChatMessage{"user", userPrompt});
		chatHistory.push_back(ChatMessage{"assistant", ""});
	}

	t_text	promptPath("/tmp/llama_prompt_" + std::to_string(getpid()) + ".txt");
	std::ofstream	pfile(promptPath.c_str());
	if (pfile.is_open())
	{
		pfile << fullPrompt;
		pfile.close();
	}

	t_text	execCmd("third_party/llama.cpp/build/bin/llama-cli -m models/qwen-0.5b-coder.gguf -f " + promptPath + " -n 256 --temp 0.2 -c 512 --no-display-prompt -r \"<|im_end|>\" 2>/dev/null");
	FILE*	pipe(popen(execCmd.c_str(), "r"));
	if (!pipe)
	{
		std::lock_guard<std::mutex>	lock(chatMutex);
		chatHistory.pop_back();
		chatHistory.push_back(ChatMessage{"assistant", "[Error running AI inference]"});
		isAiThinking = false;
		unlink(promptPath.c_str());
		return;
	}

	char	buffer[128];
	t_text	streamedText("");
	while (fgets(buffer, sizeof(buffer), pipe) != NULL)
	{
		streamedText += buffer;
		size_t tagPos = streamedText.find("<|im_end|>");
		if (tagPos != std::string::npos)
			streamedText = streamedText.substr(0, tagPos);
		{
			std::lock_guard<std::mutex>	lock(chatMutex);
			chatHistory.back().text = streamedText;
		}
	}
	pclose(pipe);
	unlink(promptPath.c_str());
	isAiThinking = false;

	// Perform TTS using spd-say in Portuguese explicitly
	t_text	speakText("");
	{
		std::lock_guard<std::mutex>	lock(chatMutex);
		speakText = chatHistory.back().text;
	}
	if (!speakText.empty())
	{
		t_text	cleaned("");
		for (char c : speakText)
		{
			if (c != '`' && c != '*' && c != '#' && c != '"' && c != '\'')
				cleaned.push_back(c);
		}
		t_text	ttsCmd("spd-say -l pt \"" + cleaned + "\" &");
		system(ttsCmd.c_str());
	}
}

// Background Whisper worker thread
static void	whisperWorker(void)
{
	t_text	execCmd("third_party/whisper.cpp/build/bin/whisper-cli -m models/ggml-tiny.bin -f /tmp/voice.wav -nt 2>/dev/null");
	FILE*	pipe(popen(execCmd.c_str(), "r"));
	if (!pipe)
	{
		isAudioTranscribing = false;
		unlink("/tmp/voice.wav");
		return;
	}

	char	buffer[256];
	t_text	transcribedText("");
	while (fgets(buffer, sizeof(buffer), pipe) != NULL)
		transcribedText += buffer;
	pclose(pipe);
	unlink("/tmp/voice.wav");

	size_t	first = transcribedText.find_first_not_of(" \t\n\r");
	size_t	last = transcribedText.find_last_not_of(" \t\n\r");
	if (first != std::string::npos && last != std::string::npos)
		transcribedText = transcribedText.substr(first, last - first + 1);
	else
		transcribedText = "";

	if (!transcribedText.empty())
	{
		std::lock_guard<std::mutex>	lock(chatMutex);
		if (!aiInputPrompt.empty())
			aiInputPrompt += " ";
		aiInputPrompt += transcribedText;
	}
	isAudioTranscribing = false;
}

int	main(int argc, char* argv[])
{
	Config				config;
	Font				textFont(GetFontDefault());
	Font				textFontBold(GetFontDefault());
	Vector2				dpiScale(makeVector2(1.0f, 1.0f));
	float				scale(1.0f);
	bool				isCustomFontLoaded(false);
	bool				isBoldFontLoaded(false);
	std::vector<t_text>	lines;
	size_t				cursorLine(0);
	size_t				cursorCol(0);
	int					frameCounter(0);
	int					character(0);
	size_t				i(0);
	Vector2				drawPos(makeVector2(0.0f, 0.0f));
	t_text				remainingText("");
	t_text				currentSubStr("");
	float				scrollOffsetY(0.0f);
	std::vector<TrailPoint>	cursorTrail;
	bool				isDragging(false);
	Vector2				dragStart(makeVector2(0.0f, 0.0f));
	float				wheel(0.0f);
	Vector2				mousePos(makeVector2(0.0f, 0.0f));
	int					clickedLine(0);
	t_text				lineText("");
	int					closestCol(0);
	float				minDistance(999999.0f);
	size_t				col(0);
	t_text				sub("");
	float				width(0.0f);
	float				charX(0.0f);
	float				dist(0.0f);
	float				cursorMinY(0.0f);
	float				cursorMaxY(0.0f);
	t_text				currentCursorLine("");
	t_text				cursorPrefix("");
	float				cursorTargetX(0.0f);
	float				cursorTargetY(0.0f);
	t_text				prevLineText("");
	float				prevCursorX(0.0f);
	float				prevCursorY(0.0f);
	std::vector<TrailPoint>::iterator	it;
	t_text				line("");
	float				currentY(0.0f);
	std::vector<Token>	tokens;
	float				currentX(0.0f);
	Vector2				snappedDrawPos(makeVector2(0.0f, 0.0f));
	float				tpY(0.0f);
	Vector2				pos(makeVector2(0.0f, 0.0f));
	float				cY(0.0f);
	float				maxScroll(0.0f);
	float				adjustedMouseY(0.0f);
	std::ifstream		file;
	bool				cursorMoved(false);
	size_t				prevLine(0);
	size_t				prevCol(0);
	size_t				tokenIndex(0);
	Token				token;
	size_t				trailIndex(0);
	TrailPoint			tp;
	Color				trailColor(GetColor(0x00000000));
	float				blinkTimer(0.0f);
	bool				blinkShow(true);

	bool				selectionActive(false);
	size_t				selectionStartLine(0);
	size_t				selectionStartCol(0);
	size_t				selectionEndLine(0);
	size_t				selectionEndCol(0);
	size_t				minL(0);
	size_t				minC(0);
	size_t				maxL(0);
	size_t				maxC(0);
	t_text				copiedText("");
	t_text				pastedText("");
	size_t				pasteLine(0);
	size_t				pasteCol(0);
	size_t				lineIndex(0);
	size_t				newlinePos(0);
	t_text				pasteLinePart("");

	DialogState			dialogState(STATE_NONE);
	t_text				dialogInput("");
	t_text				currentFilePath("");
	std::ofstream		outFile;
	int					charInput(0);
	size_t				indentLine(0);
	bool				isCtrlPressed(false);
	bool				isShiftPressed(false);
	float				circleRadius(6.0f);

	bool				isMaximized(false);
	int					prevWidth(WINDOW_WIDTH);
	int					prevHeight(WINDOW_HEIGHT);
	Vector2				prevPos(makeVector2(100.0f, 100.0f));

	int					activeRepeatKey(0);
	float				keyRepeatTimer(0.0f);
	bool				triggerKeyRepeat(false);

	std::vector<HistoryState>	undoStack;
	std::vector<HistoryState>	redoStack;
	float				lastTypeTime(0.0f);
	bool				isWordInProgress(false);

	// Embedded Terminal state split variables
	bool				isTerminalOpen(false);
	bool				isEditorFocused(true);

	// Autocomplete structures
	std::set<t_text>	projectDictionary(indexProjectWords());
	std::vector<t_text>	suggestions;
	int					activeSuggestion(0);
	bool				autocompleteActive(false);
	t_text				currentPrefix("");
	size_t				prefixStartCol(0);
	size_t				sIdx(0);
	t_text				selectedSug("");
	int					fontSize(18);
	float				fontSpacing(1.0f);
	int					verticalStride(24);

	// Terminal launching geometry options
	int					initX(-1);
	int					initY(-1);
	int					initW(WINDOW_WIDTH);
	int					initH(WINDOW_HEIGHT);
	t_text				filePath("");

	if (argc >= 5)
	{
		try
		{
			initX = std::stoi(argv[1]);
			initY = std::stoi(argv[2]);
			initW = std::stoi(argv[3]);
			initH = std::stoi(argv[4]);
			if (argc > 5)
				filePath = argv[5];
		}
		catch (...)
		{
			filePath = argv[1];
		}
	}
	else if (argc > 1)
	{
		filePath = argv[1];
	}

	loadConfig(config, "config.ss");
	fontSize = config.fontSize;
	fontSpacing = config.fontSpacing;
	verticalStride = config.verticalStride;

	SetConfigFlags(FLAG_WINDOW_UNDECORATED | FLAG_MSAA_4X_HINT | FLAG_WINDOW_HIGHDPI | FLAG_WINDOW_TRANSPARENT);
	InitWindow(initW, initH, "SSCodeIDE");
	SetTargetFPS(60);
	SetExitKey(KEY_NULL);

	if (initX >= 0 && initY >= 0)
	{
		SetWindowPosition(initX, initY);
		SetWindowState(FLAG_WINDOW_TOPMOST);
	}

	dpiScale = GetWindowScaleDPI();
	if (dpiScale.x > 0.0f)
		scale = dpiScale.x;

	// Build codepoints array for Latin-1 Supplement to support Portuguese accented characters
	std::vector<int> codepoints;
	for (int cp = 32; cp < 256; ++cp)
		codepoints.push_back(cp);

	textFont = LoadFontEx(config.fontPath.c_str(), fontSize * scale, codepoints.data(), codepoints.size());
	isCustomFontLoaded = (textFont.texture.id > 0) && (textFont.texture.id != GetFontDefault().texture.id);
	if (!isCustomFontLoaded)
		std::cout << "[ERROR] " << config.fontPath << " failed to load! Default fallback font active.\n";
	else
		SetTextureFilter(textFont.texture, TEXTURE_FILTER_BILINEAR);

	textFontBold = LoadFontEx(config.fontBoldPath.c_str(), fontSize * scale, codepoints.data(), codepoints.size());
	isBoldFontLoaded = (textFontBold.texture.id > 0) && (textFontBold.texture.id != GetFontDefault().texture.id);
	if (!isBoldFontLoaded)
		std::cout << "[ERROR] " << config.fontBoldPath << " failed to load!\n";
	else
		SetTextureFilter(textFontBold.texture, TEXTURE_FILTER_BILINEAR);

	if (!filePath.empty())
	{
		currentFilePath = filePath;
		file.open(currentFilePath);
		if (file.is_open())
		{
			lines.clear();
			while (std::getline(file, lineText))
				lines.push_back(lineText);
			file.close();
		}
	}

	if (lines.empty())
	{
		lines.push_back("class FileHandler {");
		lines.push_back("public:");
		lines.push_back("    // Fira Code font loaded successfully by default.");
		lines.push_back("};");
	}

	if (cursorLine >= lines.size())
		cursorLine = lines.size() - 1;
	if (cursorCol > lines[cursorLine].length())
		cursorCol = lines[cursorLine].length();

	resizeTermGrid(24, 80);

	// Pre-fill welcome message in Chat History
	chatHistory.push_back(ChatMessage{"assistant", "Olá! Sou o teu assistente de IA local. Podes usar F2 para abrir/fechar este painel, e F3 para falar no microfone!"});

	while (not WindowShouldClose())
	{
		frameCounter++;
		prevLine = cursorLine;
		prevCol = cursorCol;
		cursorMoved = false;
		isCtrlPressed = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);
		isShiftPressed = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);

		dpiScale = GetWindowScaleDPI();
		if (dpiScale.x > 0.0f)
			scale = dpiScale.x;

		// Calculate UI layouts and panel divisions dynamically (AI is floating on top)
		float	editorStartX = config.paddingHorizontal;
		float	editorEndX = GetScreenWidth();

		if (isTerminalOpen)
		{
			editorStartX = GetScreenWidth() / 2.0f + config.paddingHorizontal;
			editorEndX = GetScreenWidth();
		}

		float charWidth = MeasureTextEx(textFont, " ", static_cast<float>(fontSize), fontSpacing).x;

		// Resize terminal grid on dimensions updates
		if (isTerminalOpen)
		{
			int		newCols = static_cast<int>((GetScreenWidth() / 2.0f - 30) / charWidth);
			int		newRows = static_cast<int>((GetScreenHeight() - config.paddingVertical - 20) / verticalStride);
			if (newCols < 10) newCols = 10;
			if (newRows < 5) newRows = 5;
			
			if (newCols != termCols || newRows != termRows)
			{
				resizeTermGrid(newRows, newCols);
				if (masterFd >= 0)
				{
					struct winsize	ws;
					ws.ws_row = newRows;
					ws.ws_col = newCols;
					ws.ws_xpixel = 0;
					ws.ws_ypixel = 0;
					ioctl(masterFd, TIOCSWINSZ, &ws);
				}
			}
			if (masterFd < 0)
				initPty(termRows, termCols);
		}

		// Non-blocking read PTY process buffer
		if (masterFd >= 0)
		{
			char	buf[4096];
			int		n(read(masterFd, buf, sizeof(buf)));
			if (n > 0)
			{
				for (int charIdx = 0; charIdx < n; ++charIdx)
					processChar(buf[charIdx]);
			}
		}

		// Toggle AI Panel
		if (IsKeyPressed(KEY_F2))
		{
			isAiPanelOpen = !isAiPanelOpen;
			autocompleteActive = false;
			if (isAiPanelOpen)
			{
				isAiFocused = true;
				isEditorFocused = false;
			}
			else
			{
				isAiFocused = false;
				isEditorFocused = true;
			}
		}

		// Toggle Recording (F3)
		if (isAiPanelOpen && IsKeyPressed(KEY_F3))
		{
			if (isRecording)
			{
				system("kill -9 $(cat /tmp/parecord_pid) 2>/dev/null && rm -f /tmp/parecord_pid");
				isRecording = false;
				isAudioTranscribing = true;
				std::thread(whisperWorker).detach();
			}
			else if (!isAiThinking && !isAudioTranscribing)
			{
				unlink("/tmp/voice.wav");
				system("parecord --rate=16000 --format=s16le --channels=1 /tmp/voice.wav & echo $! > /tmp/parecord_pid");
				isRecording = true;
			}
		}

		// Handle Autocomplete selection block
		if (autocompleteActive && isEditorFocused)
		{
			if (IsKeyPressed(KEY_ESCAPE))
			{
				autocompleteActive = false;
				suggestions.clear();
			}
			else if (IsKeyPressed(KEY_DOWN))
			{
				activeSuggestion = (activeSuggestion + 1) % suggestions.size();
				cursorMoved = true;
			}
			else if (IsKeyPressed(KEY_UP))
			{
				activeSuggestion = (activeSuggestion - 1 + suggestions.size()) % suggestions.size();
				cursorMoved = true;
			}
			else if (IsKeyPressed(KEY_TAB) || IsKeyPressed(KEY_ENTER))
			{
				saveUndoState(undoStack, redoStack, lines, cursorLine, cursorCol);
				selectedSug = suggestions[activeSuggestion];
				lines[cursorLine] = lines[cursorLine].substr(0, prefixStartCol) + selectedSug + lines[cursorLine].substr(cursorCol);
				cursorCol = prefixStartCol + selectedSug.length();
				autocompleteActive = false;
				suggestions.clear();
				cursorMoved = true;
			}
		}

		// Toggle Terminal
		if (isCtrlPressed && IsKeyPressed(KEY_T))
		{
			isTerminalOpen = !isTerminalOpen;
			autocompleteActive = false;
			if (isTerminalOpen)
			{
				isEditorFocused = false;
				isAiFocused = false;
			}
			else
				isEditorFocused = true;
		}

		triggerKeyRepeat = false;
		if (!autocompleteActive && (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_UP) || IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_BACKSPACE) || IsKeyDown(KEY_DELETE) || IsKeyDown(KEY_ENTER) || IsKeyDown(KEY_TAB)))
		{
			int currentDownKey(0);
			if (IsKeyDown(KEY_LEFT)) currentDownKey = KEY_LEFT;
			else if (IsKeyDown(KEY_RIGHT)) currentDownKey = KEY_RIGHT;
			else if (IsKeyDown(KEY_UP)) currentDownKey = KEY_UP;
			else if (IsKeyDown(KEY_DOWN)) currentDownKey = KEY_DOWN;
			else if (IsKeyDown(KEY_BACKSPACE)) currentDownKey = KEY_BACKSPACE;
			else if (IsKeyDown(KEY_DELETE)) currentDownKey = KEY_DELETE;
			else if (IsKeyDown(KEY_ENTER)) currentDownKey = KEY_ENTER;
			else if (IsKeyDown(KEY_TAB)) currentDownKey = KEY_TAB;

			if (currentDownKey == activeRepeatKey)
			{
				keyRepeatTimer += GetFrameTime();
				if (keyRepeatTimer >= 0.4f)
				{
					triggerKeyRepeat = true;
					keyRepeatTimer = 0.4f - 0.05f;
				}
			}
			else
			{
				activeRepeatKey = currentDownKey;
				keyRepeatTimer = 0.0f;
			}
		}
		else
		{
			activeRepeatKey = 0;
			keyRepeatTimer = 0.0f;
		}

		// Route Keyboards
		if (isAiPanelOpen && isAiFocused)
		{
			int c = GetCharPressed();
			while (c > 0)
			{
				if (c >= 32 && c <= 255)
					aiInputPrompt.push_back(static_cast<char>(c));
				c = GetCharPressed();
			}
			if (IsKeyPressed(KEY_BACKSPACE) || (activeRepeatKey == KEY_BACKSPACE && triggerKeyRepeat))
			{
				if (!aiInputPrompt.empty())
					aiInputPrompt.pop_back();
			}
			if (IsKeyPressed(KEY_ENTER))
			{
				if (!aiInputPrompt.empty() && !isAiThinking && !isAudioTranscribing)
				{
					isAiThinking = true;
					std::thread(aiWorker, aiInputPrompt).detach();
					aiInputPrompt = "";
				}
			}
		}
		else if (!isEditorFocused && isTerminalOpen && !isAiFocused)
		{
			int c = GetCharPressed();
			while (c > 0)
			{
				char ch = static_cast<char>(c);
				write(masterFd, &ch, 1);
				c = GetCharPressed();
			}

			char outChar = 0;
			if (IsKeyPressed(KEY_ENTER) || (activeRepeatKey == KEY_ENTER && triggerKeyRepeat))
			{
				outChar = '\r';
				write(masterFd, &outChar, 1);
			}
			else if (IsKeyPressed(KEY_BACKSPACE) || (activeRepeatKey == KEY_BACKSPACE && triggerKeyRepeat))
			{
				outChar = '\x7f';
				write(masterFd, &outChar, 1);
			}
			else if (IsKeyPressed(KEY_TAB) || (activeRepeatKey == KEY_TAB && triggerKeyRepeat))
			{
				outChar = '\t';
				write(masterFd, &outChar, 1);
			}
			else if (IsKeyPressed(KEY_ESCAPE))
			{
				outChar = '\x1b';
				write(masterFd, &outChar, 1);
			}
			else if (IsKeyPressed(KEY_UP) || (activeRepeatKey == KEY_UP && triggerKeyRepeat))
			{
				write(masterFd, "\x1b[A", 3);
			}
			else if (IsKeyPressed(KEY_DOWN) || (activeRepeatKey == KEY_DOWN && triggerKeyRepeat))
			{
				write(masterFd, "\x1b[B", 3);
			}
			else if (IsKeyPressed(KEY_RIGHT) || (activeRepeatKey == KEY_RIGHT && triggerKeyRepeat))
			{
				write(masterFd, "\x1b[C", 3);
			}
			else if (IsKeyPressed(KEY_LEFT) || (activeRepeatKey == KEY_LEFT && triggerKeyRepeat))
			{
				write(masterFd, "\x1b[D", 3);
			}
			else if (isCtrlPressed)
			{
				char ctrlVal = 0;
				if (IsKeyPressed(KEY_C)) ctrlVal = '\x03';
				else if (IsKeyPressed(KEY_D)) ctrlVal = '\x04';
				else if (IsKeyPressed(KEY_Z)) ctrlVal = '\x1a';
				else if (IsKeyPressed(KEY_L)) ctrlVal = '\x0c';
				
				if (ctrlVal != 0)
					write(masterFd, &ctrlVal, 1);
			}
		}
		else if (dialogState != STATE_NONE)
		{
			charInput = GetCharPressed();
			while (charInput > 0)
			{
				if (charInput >= 32 && charInput <= 255)
					dialogInput.push_back(static_cast<char>(charInput));
				charInput = GetCharPressed();
			}

			if (IsKeyPressed(KEY_BACKSPACE))
				if (!dialogInput.empty())
					dialogInput.pop_back();

			if (IsKeyPressed(KEY_ESCAPE))
				dialogState = STATE_NONE;

			if (IsKeyPressed(KEY_ENTER))
			{
				if (dialogState == STATE_OPEN)
				{
					file.open(dialogInput);
					if (file.is_open())
					{
						lines.clear();
						while (std::getline(file, lineText))
							lines.push_back(lineText);
						file.close();
						currentFilePath = dialogInput;
						cursorLine = 0;
						cursorCol = 0;
						selectionActive = false;
						undoStack.clear();
						redoStack.clear();
						projectDictionary = indexProjectWords();
					}
				}
				else if (dialogState == STATE_SAVE_AS)
				{
					outFile.open(dialogInput);
					if (outFile.is_open())
					{
						for (lineIndex = 0; lineIndex < lines.size(); ++lineIndex)
							outFile << lines[lineIndex] << "\n";
						outFile.close();
						currentFilePath = dialogInput;
						projectDictionary = indexProjectWords();
					}
				}
				dialogState = STATE_NONE;
			}
		}
		else
		{
			if (isCtrlPressed && IsKeyPressed(KEY_Z))
			{
				if (isShiftPressed)
				{
					if (!redoStack.empty())
					{
						HistoryState rState = redoStack.back();
						redoStack.pop_back();
						
						HistoryState uState;
						uState.lines = lines;
						uState.cursorLine = cursorLine;
						uState.cursorCol = cursorCol;
						undoStack.push_back(uState);
						
						lines = rState.lines;
						cursorLine = rState.cursorLine;
						cursorCol = rState.cursorCol;
						cursorMoved = true;
						selectionActive = false;
						autocompleteActive = false;
					}
				}
				else
				{
					if (!undoStack.empty())
					{
						HistoryState uState = undoStack.back();
						undoStack.pop_back();
						
						HistoryState rState;
						rState.lines = lines;
						rState.cursorLine = cursorLine;
						rState.cursorCol = cursorCol;
						redoStack.push_back(rState);
						
						lines = uState.lines;
						cursorLine = uState.cursorLine;
						cursorCol = uState.cursorCol;
						cursorMoved = true;
						selectionActive = false;
						autocompleteActive = false;
					}
				}
			}
			else if (IsKeyPressed(KEY_F1))
			{
				saveUndoState(undoStack, redoStack, lines, cursorLine, cursorCol);
				insertOrUpdateHeader(lines, currentFilePath, cursorLine);
				cursorMoved = true;
			}
			else if (isCtrlPressed && IsKeyPressed(KEY_O))
			{
				dialogState = STATE_OPEN;
				dialogInput = "";
			}
			else if (isCtrlPressed && IsKeyPressed(KEY_S))
			{
				if (isShiftPressed)
				{
					dialogState = STATE_SAVE_AS;
					dialogInput = currentFilePath;
				}
				else if (!currentFilePath.empty())
				{
					outFile.open(currentFilePath);
					if (outFile.is_open())
					{
						for (lineIndex = 0; lineIndex < lines.size(); ++lineIndex)
							outFile << lines[lineIndex] << "\n";
						outFile.close();
					}
				}
				else
				{
					dialogState = STATE_SAVE_AS;
					dialogInput = "";
				}
			}
			else if (isCtrlPressed && IsKeyPressed(KEY_C))
			{
				if (selectionActive)
					SetClipboardText(getSelectedText(lines, selectionStartLine, selectionStartCol, selectionEndLine, selectionEndCol).c_str());
			}
			else if (isCtrlPressed && IsKeyPressed(KEY_X))
			{
				if (selectionActive)
				{
					saveUndoState(undoStack, redoStack, lines, cursorLine, cursorCol);
					SetClipboardText(getSelectedText(lines, selectionStartLine, selectionStartCol, selectionEndLine, selectionEndCol).c_str());
					deleteSelectedText(lines, cursorLine, cursorCol, selectionStartLine, selectionStartCol, selectionEndLine, selectionEndCol);
					selectionActive = false;
					cursorMoved = true;
				}
			}
			else if (isCtrlPressed && IsKeyPressed(KEY_V))
			{
				saveUndoState(undoStack, redoStack, lines, cursorLine, cursorCol);
				if (selectionActive)
				{
					deleteSelectedText(lines, cursorLine, cursorCol, selectionStartLine, selectionStartCol, selectionEndLine, selectionEndCol);
					selectionActive = false;
				}
				pastedText = GetClipboardText();
				if (!pastedText.empty())
				{
					pasteLine = cursorLine;
					pasteCol = cursorCol;
					newlinePos = pastedText.find('\n');
					while (newlinePos != std::string::npos)
					{
						pasteLinePart = pastedText.substr(0, newlinePos);
						if (pasteLinePart.length() > 0 && pasteLinePart.back() == '\r')
							pasteLinePart.pop_back();

						remainingText = lines[pasteLine].substr(pasteCol);
						lines[pasteLine] = lines[pasteLine].substr(0, pasteCol) + pasteLinePart;
						lines.insert(lines.begin() + pasteLine + 1, remainingText);

						pasteLine++;
						pasteCol = 0;
						pastedText = pastedText.substr(newlinePos + 1);
						newlinePos = pastedText.find('\n');
					}
					lines[pasteLine].insert(pasteCol, pastedText);
					cursorLine = pasteLine;
					cursorCol = pasteCol + pastedText.length();
					cursorMoved = true;
				}
			}
			else if (isCtrlPressed && IsKeyPressed(KEY_A))
			{
				selectionActive = true;
				selectionStartLine = 0;
				selectionStartCol = 0;
				selectionEndLine = lines.size() - 1;
				selectionEndCol = lines.back().length();
				cursorLine = selectionEndLine;
				cursorCol = selectionEndCol;
				cursorMoved = true;
			}
			else
			{
				character = GetCharPressed();
				if (character > 0)
				{
					float nowTime(static_cast<float>(GetTime()));
					if (!isWordInProgress || nowTime - lastTypeTime > 0.5f || character == ' ' || character == '\t')
					{
						saveUndoState(undoStack, redoStack, lines, cursorLine, cursorCol);
						isWordInProgress = true;
					}
					lastTypeTime = nowTime;
				}
				while (character > 0)
				{
					if (character >= 32 && character <= 255)
					{
						if (selectionActive)
						{
							deleteSelectedText(lines, cursorLine, cursorCol, selectionStartLine, selectionStartCol, selectionEndLine, selectionEndCol);
							selectionActive = false;
						}
						if (character >= 128)
						{
							std::string utf8char("");
							utf8char.push_back(static_cast<char>(0xC0 | (character >> 6)));
							utf8char.push_back(static_cast<char>(0x80 | (character & 0x3F)));
							lines[cursorLine].insert(cursorCol, utf8char);
							cursorCol += 2;
						}
						else
						{
							lines[cursorLine].insert(cursorCol++, 1, static_cast<char>(character));
						}
						cursorMoved = true;
					}
					character = GetCharPressed();
				}

				if (IsKeyPressed(KEY_ENTER))
				{
					saveUndoState(undoStack, redoStack, lines, cursorLine, cursorCol);
					if (selectionActive)
					{
						deleteSelectedText(lines, cursorLine, cursorCol, selectionStartLine, selectionStartCol, selectionEndLine, selectionEndCol);
						selectionActive = false;
					}
					remainingText = lines[cursorLine].substr(cursorCol);
					lines[cursorLine] = lines[cursorLine].substr(0, cursorCol);
					lines.insert(lines.begin() + ++cursorLine, remainingText);
					cursorCol = 0;
					cursorMoved = true;
					isWordInProgress = false;
				}

				if (IsKeyPressed(KEY_TAB))
				{
					saveUndoState(undoStack, redoStack, lines, cursorLine, cursorCol);
					if (selectionActive)
					{
						getSelectionBounds(selectionStartLine, selectionStartCol, selectionEndLine, selectionEndCol, minL, minC, maxL, maxC);
						for (indentLine = minL; indentLine <= maxL; ++indentLine)
						{
							if (isShiftPressed)
							{
								if (lines[indentLine].rfind("\t", 0) == 0)
									lines[indentLine] = lines[indentLine].substr(1);
								else if (lines[indentLine].rfind("    ", 0) == 0)
									lines[indentLine] = lines[indentLine].substr(4);
							}
							else
								lines[indentLine].insert(0, "\t");
						}
						if (selectionStartCol > 0 && !isShiftPressed)
							selectionStartCol++;
						if (selectionEndCol > 0 && !isShiftPressed)
							selectionEndCol++;
						cursorCol = selectionEndCol;
						cursorMoved = true;
					}
					else
					{
						if (isShiftPressed)
						{
							if (lines[cursorLine].rfind("\t", 0) == 0)
							{
								lines[cursorLine] = lines[cursorLine].substr(1);
								if (cursorCol > 0)
									cursorCol--;
								cursorMoved = true;
							}
						}
						else
						{
							lines[cursorLine].insert(cursorCol++, "\t");
							cursorMoved = true;
						}
					}
					isWordInProgress = false;
				}

				if (IsKeyPressed(KEY_BACKSPACE) || (activeRepeatKey == KEY_BACKSPACE && triggerKeyRepeat))
				{
					if (IsKeyPressed(KEY_BACKSPACE))
					{
						saveUndoState(undoStack, redoStack, lines, cursorLine, cursorCol);
						isWordInProgress = false;
					}
					if (selectionActive)
					{
						deleteSelectedText(lines, cursorLine, cursorCol, selectionStartLine, selectionStartCol, selectionEndLine, selectionEndCol);
						selectionActive = false;
						cursorMoved = true;
					}
					else if (cursorCol > 0)
					{
						size_t eraseLen = 1;
						if (cursorCol >= 2)
						{
							unsigned char c1 = static_cast<unsigned char>(lines[cursorLine][cursorCol - 2]);
							unsigned char c2 = static_cast<unsigned char>(lines[cursorLine][cursorCol - 1]);
							if (c1 >= 0xC0 && c1 <= 0xDF && c2 >= 0x80 && c2 <= 0xBF)
								eraseLen = 2;
						}
						lines[cursorLine].erase(cursorCol - eraseLen, eraseLen);
						cursorCol -= eraseLen;
						cursorMoved = true;
					}
					else if (cursorLine > 0)
					{
						cursorCol = lines[cursorLine - 1].length();
						lines[cursorLine - 1] += lines[cursorLine];
						lines.erase(lines.begin() + cursorLine--);
						cursorMoved = true;
					}
				}

				if (IsKeyPressed(KEY_DELETE) || (activeRepeatKey == KEY_DELETE && triggerKeyRepeat))
				{
					if (IsKeyPressed(KEY_DELETE))
					{
						saveUndoState(undoStack, redoStack, lines, cursorLine, cursorCol);
						isWordInProgress = false;
					}
					if (selectionActive)
					{
						deleteSelectedText(lines, cursorLine, cursorCol, selectionStartLine, selectionStartCol, selectionEndLine, selectionEndCol);
						selectionActive = false;
						cursorMoved = true;
					}
					else if (cursorCol < lines[cursorLine].length())
					{
						size_t eraseLen = 1;
						if (cursorCol + 1 < lines[cursorLine].length())
						{
							unsigned char c1 = static_cast<unsigned char>(lines[cursorLine][cursorCol]);
							unsigned char c2 = static_cast<unsigned char>(lines[cursorLine][cursorCol + 1]);
							if (c1 >= 0xC0 && c1 <= 0xDF && c2 >= 0x80 && c2 <= 0xBF)
								eraseLen = 2;
						}
						lines[cursorLine].erase(cursorCol, eraseLen);
						cursorMoved = true;
					}
					else if (cursorLine < lines.size() - 1)
					{
						lines[cursorLine] += lines[cursorLine + 1];
						lines.erase(lines.begin() + cursorLine + 1);
						cursorMoved = true;
					}
				}

				if (IsKeyPressed(KEY_LEFT) || (activeRepeatKey == KEY_LEFT && triggerKeyRepeat))
				{
					if (isCtrlPressed)
						cursorCol = findWordBoundaryLeft(lines[cursorLine], cursorCol);
					else if (cursorCol > 0)
					{
						size_t step = 1;
						if (cursorCol >= 2)
						{
							unsigned char c1 = static_cast<unsigned char>(lines[cursorLine][cursorCol - 2]);
							unsigned char c2 = static_cast<unsigned char>(lines[cursorLine][cursorCol - 1]);
							if (c1 >= 0xC0 && c1 <= 0xDF && c2 >= 0x80 && c2 <= 0xBF)
								step = 2;
						}
						cursorCol -= step;
					}
					else if (cursorLine > 0)
						cursorCol = lines[--cursorLine].length();
					handleSelectionKey(isShiftPressed, selectionActive, selectionStartLine, selectionStartCol, selectionEndLine, selectionEndCol, cursorLine, cursorCol);
					cursorMoved = true;
				}

				if (IsKeyPressed(KEY_RIGHT) || (activeRepeatKey == KEY_RIGHT && triggerKeyRepeat))
				{
					if (isCtrlPressed)
						cursorCol = findWordBoundaryRight(lines[cursorLine], cursorCol);
					else if (cursorCol < lines[cursorLine].length())
					{
						size_t step = 1;
						if (cursorCol + 1 < lines[cursorLine].length())
						{
							unsigned char c1 = static_cast<unsigned char>(lines[cursorLine][cursorCol]);
							unsigned char c2 = static_cast<unsigned char>(lines[cursorLine][cursorCol + 1]);
							if (c1 >= 0xC0 && c1 <= 0xDF && c2 >= 0x80 && c2 <= 0xBF)
								step = 2;
						}
						cursorCol += step;
					}
					else if (cursorLine < lines.size() - 1)
					{
						cursorLine++;
						cursorCol = 0;
					}
					handleSelectionKey(isShiftPressed, selectionActive, selectionStartLine, selectionStartCol, selectionEndLine, selectionEndCol, cursorLine, cursorCol);
					cursorMoved = true;
				}

				if ((IsKeyPressed(KEY_UP) || (activeRepeatKey == KEY_UP && triggerKeyRepeat)) && cursorLine > 0)
				{
					cursorLine--;
					if (cursorCol > lines[cursorLine].length())
						cursorCol = lines[cursorLine].length();
					handleSelectionKey(isShiftPressed, selectionActive, selectionStartLine, selectionStartCol, selectionEndLine, selectionEndCol, cursorLine, cursorCol);
					cursorMoved = true;
				}

				if ((IsKeyPressed(KEY_DOWN) || (activeRepeatKey == KEY_DOWN && triggerKeyRepeat)) && cursorLine < lines.size() - 1)
				{
					cursorLine++;
					if (cursorCol > lines[cursorLine].length())
						cursorCol = lines[cursorLine].length();
					handleSelectionKey(isShiftPressed, selectionActive, selectionStartLine, selectionStartCol, selectionEndLine, selectionEndCol, cursorLine, cursorCol);
					cursorMoved = true;
				}
			}

			// Generate suggestions if typing
			if (cursorMoved && !isCtrlPressed && !selectionActive)
			{
				lineText = lines[cursorLine];
				prefixStartCol = cursorCol;
				while (prefixStartCol > 0 && (std::isalnum(lineText[prefixStartCol - 1]) || lineText[prefixStartCol - 1] == '_'))
					prefixStartCol--;
				currentPrefix = lineText.substr(prefixStartCol, cursorCol - prefixStartCol);
				if (currentPrefix.length() >= 2)
				{
					suggestions.clear();
					for (const auto& w : projectDictionary)
					{
						if (w != currentPrefix && w.rfind(currentPrefix, 0) == 0)
						{
							suggestions.push_back(w);
							if (suggestions.size() >= 7)
								break;
						}
					}
					if (!suggestions.empty())
					{
						autocompleteActive = true;
						activeSuggestion = 0;
					}
					else
						autocompleteActive = false;
				}
				else
					autocompleteActive = false;
			}
		}

		if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
		{
			mousePos = GetMousePosition();
			
			// Click checks on top-right titlebar controls
			if (CheckCollisionPointCircle(mousePos, makeVector2(static_cast<float>(GetScreenWidth() - 35), 35.0f), circleRadius + 2.0f))
				break;
			if (CheckCollisionPointCircle(mousePos, makeVector2(static_cast<float>(GetScreenWidth() - 55), 35.0f), circleRadius + 2.0f))
				MinimizeWindow();
			if (CheckCollisionPointCircle(mousePos, makeVector2(static_cast<float>(GetScreenWidth() - 75), 35.0f), circleRadius + 2.0f))
			{
				if (isMaximized)
				{
					SetWindowState(FLAG_WINDOW_HIDDEN);
					SetWindowSize(prevWidth, prevHeight);
					SetWindowPosition(static_cast<int>(prevPos.x), static_cast<int>(prevPos.y));
					ClearWindowState(FLAG_WINDOW_HIDDEN);
					isMaximized = false;
				}
				else
				{
					int monitorVal(GetCurrentMonitor());
					prevWidth = GetScreenWidth();
					prevHeight = GetScreenHeight();
					prevPos = GetWindowPosition();
					SetWindowState(FLAG_WINDOW_HIDDEN);
					SetWindowPosition(0, 0);
					SetWindowSize(GetMonitorWidth(monitorVal), GetMonitorHeight(monitorVal));
					ClearWindowState(FLAG_WINDOW_HIDDEN);
					isMaximized = true;
				}
			}

			// Custom window drag check
			if (mousePos.y < 60.0f && mousePos.x < static_cast<float>(GetScreenWidth() - 90))
			{
				if (!isMaximized)
				{
					dragStart = GetMousePosition();
					isDragging = true;
				}
			}
			else
			{
				// Handle focus clicking on splits (Terminal / Editor / AI Panel)
				float	floatX = GetScreenWidth() - 420.0f;
				float	floatY = 80.0f;
				float	floatW = 400.0f;
				float	floatH = GetScreenHeight() - 160.0f;

				if (isAiPanelOpen && mousePos.x >= floatX && mousePos.x <= floatX + floatW &&
					mousePos.y >= floatY && mousePos.y <= floatY + floatH)
				{
					isAiFocused = true;
					isEditorFocused = false;
				}
				else if (isTerminalOpen && mousePos.x < GetScreenWidth() / 2.0f)
				{
					isEditorFocused = false;
					isAiFocused = false;
				}
				else
				{
					isEditorFocused = true;
					isAiFocused = false;
				}

				// Click triggers on buttons in the AI Panel
				if (isAiPanelOpen && isAiFocused)
				{
					// Click "F3 / Speak" button
					if (mousePos.y >= floatY + floatH - 120.0f && mousePos.y <= floatY + floatH - 95.0f && mousePos.x >= floatX + 10.0f && mousePos.x <= floatX + 180.0f)
					{
						if (isRecording)
						{
							system("kill -9 $(cat /tmp/parecord_pid) 2>/dev/null && rm -f /tmp/parecord_pid");
							isRecording = false;
							isAudioTranscribing = true;
							std::thread(whisperWorker).detach();
						}
						else if (!isAiThinking && !isAudioTranscribing)
						{
							unlink("/tmp/voice.wav");
							system("parecord --rate=16000 --format=s16le --channels=1 /tmp/voice.wav & echo $! > /tmp/parecord_pid");
							isRecording = true;
						}
					}
					// Click "Analyze Code" button
					else if (mousePos.y >= floatY + floatH - 120.0f && mousePos.y <= floatY + floatH - 95.0f && mousePos.x >= floatX + 210.0f && mousePos.x <= floatX + 390.0f)
					{
						if (!isAiThinking)
						{
							t_text selectedText = "";
							if (selectionActive)
								selectedText = getSelectedText(lines, selectionStartLine, selectionStartCol, selectionEndLine, selectionEndCol);
							else
							{
								for (const auto& l : lines)
									selectedText += l + "\n";
							}
							isAiThinking = true;
							std::thread(aiWorker, "Explica brevemente o seguinte código em português:\n\n" + selectedText).detach();
						}
					}
					// Click "Clear Chat" button
					else if (mousePos.y >= floatY + floatH - 40.0f && mousePos.y <= floatY + floatH - 15.0f && mousePos.x >= floatX + 10.0f && mousePos.x <= floatX + 150.0f)
					{
						std::lock_guard<std::mutex>	lock(chatMutex);
						chatHistory.clear();
						chatHistory.push_back(ChatMessage{"assistant", "Histórico de conversação limpo. Como posso ajudar?"});
					}
				}

				if (isEditorFocused)
				{
					adjustedMouseY = mousePos.y + scrollOffsetY;
					clickedLine = (adjustedMouseY - config.paddingVertical + verticalStride / 2.0f) / verticalStride;
					if (clickedLine < 0)
						clickedLine = 0;
					if (clickedLine >= static_cast<int>(lines.size()))
						clickedLine = lines.size() - 1;

					cursorLine = clickedLine;
					lineText = lines[clickedLine];
					closestCol = 0;
					minDistance = 999999.0f;
					
					for (col = 0; col <= lineText.length(); ++col)
					{
						width = calculateTextWidth(textFont, textFontBold, lineText, config.tabSize, fontSize, fontSpacing, col, config.keywordColor, config.typeColor, config.stringColor, config.commentColor, config.textColor);
						charX = editorStartX + width;
						dist = std::abs(mousePos.x - charX);
						if (dist < minDistance)
						{
							minDistance = dist;
							closestCol = col;
						}
					}
					cursorCol = closestCol;

					if (isShiftPressed)
					{
						if (!selectionActive)
						{
							selectionActive = true;
							selectionStartLine = prevLine;
							selectionStartCol = prevCol;
						}
						selectionEndLine = cursorLine;
						selectionEndCol = cursorCol;
					}
					else
					{
						selectionActive = true;
						selectionStartLine = cursorLine;
						selectionStartCol = cursorCol;
						selectionEndLine = cursorLine;
						selectionEndCol = cursorCol;
					}
					cursorMoved = true;
					autocompleteActive = false;
				}
			}
		}

		if (isDragging)
		{
			if (IsMouseButtonDown(MOUSE_BUTTON_LEFT))
			{
				mousePos = GetMousePosition();
				SetWindowPosition(static_cast<int>(GetWindowPosition().x + (mousePos.x - dragStart.x)), static_cast<int>(GetWindowPosition().y + (mousePos.y - dragStart.y)));
			}
			else
				isDragging = false;
		}
		else if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && mousePos.y >= 60.0f && isEditorFocused)
		{
			mousePos = GetMousePosition();
			adjustedMouseY = mousePos.y + scrollOffsetY;
			clickedLine = (adjustedMouseY - config.paddingVertical + verticalStride / 2.0f) / verticalStride;
			if (clickedLine < 0)
				clickedLine = 0;
			if (clickedLine >= static_cast<int>(lines.size()))
				clickedLine = lines.size() - 1;

			cursorLine = clickedLine;
			lineText = lines[clickedLine];
			closestCol = 0;
			minDistance = 999999.0f;
			
			for (col = 0; col <= lineText.length(); ++col)
			{
				width = calculateTextWidth(textFont, textFontBold, lineText, config.tabSize, fontSize, fontSpacing, col, config.keywordColor, config.typeColor, config.stringColor, config.commentColor, config.textColor);
				charX = editorStartX + width;
				dist = std::abs(mousePos.x - charX);
				if (dist < minDistance)
				{
					minDistance = dist;
					closestCol = col;
				}
			}
			cursorCol = closestCol;

			if (cursorLine != selectionStartLine || cursorCol != selectionStartCol)
			{
				selectionActive = true;
				selectionEndLine = cursorLine;
				selectionEndCol = cursorCol;
			}
			cursorMoved = true;
		}

		wheel = GetMouseWheelMove();
		if (wheel != 0.0f)
		{
			float	floatX = GetScreenWidth() - 420.0f;
			float	floatY = 80.0f;
			float	floatW = 400.0f;
			float	floatH = GetScreenHeight() - 160.0f;

			if (isAiPanelOpen && mousePos.x >= floatX && mousePos.x <= floatX + floatW &&
				mousePos.y >= floatY && mousePos.y <= floatY + floatH)
			{
				aiScrollY -= wheel * 30.0f;
				if (aiScrollY < 0.0f)
					aiScrollY = 0.0f;
			}
			else
			{
				scrollOffsetY -= wheel * 30.0f;
				if (scrollOffsetY < 0.0f)
					scrollOffsetY = 0.0f;
				maxScroll = (lines.size() * verticalStride) - (GetScreenHeight() - config.paddingVertical - 50.0f);
				if (maxScroll < 0.0f)
					maxScroll = 0.0f;
				if (scrollOffsetY > maxScroll)
					scrollOffsetY = maxScroll;
			}
		}

		cursorMinY = config.paddingVertical + cursorLine * verticalStride;
		cursorMaxY = cursorMinY + verticalStride;
		if (cursorMinY - scrollOffsetY < config.paddingVertical)
			scrollOffsetY = cursorMinY - config.paddingVertical;
		if (cursorMaxY - scrollOffsetY > GetScreenHeight() - 50.0f)
			scrollOffsetY = cursorMaxY - (GetScreenHeight() - 50.0f);

		currentCursorLine = lines[cursorLine];
		cursorTargetX = editorStartX + calculateTextWidth(textFont, textFontBold, currentCursorLine, config.tabSize, fontSize, fontSpacing, cursorCol, config.keywordColor, config.typeColor, config.stringColor, config.commentColor, config.textColor);
		cursorTargetY = config.paddingVertical + cursorLine * verticalStride;

		if (cursorMoved || prevLine != cursorLine || prevCol != cursorCol)
		{
			blinkTimer = 0.0f;
			blinkShow = true;

			prevLineText = lines[prevLine];
			prevCursorX = editorStartX + calculateTextWidth(textFont, textFontBold, prevLineText, config.tabSize, fontSize, fontSpacing, prevCol, config.keywordColor, config.typeColor, config.stringColor, config.commentColor, config.textColor);
			prevCursorY = config.paddingVertical + prevLine * verticalStride;

			if (prevCursorX != cursorTargetX || prevCursorY != cursorTargetY)
			{
				if (isCtrlPressed)
					addCursorTrailPoints(cursorTrail, makeVector2(prevCursorX, prevCursorY), makeVector2(cursorTargetX, cursorTargetY), 8, 2.0f, true);
				else
					cursorTrail.push_back(TrailPoint{makeVector2(prevCursorX, prevCursorY), 2.0f, 1.0f});
			}
		}

		if (selectionActive && selectionStartLine == selectionEndLine && selectionStartCol == selectionEndCol)
			selectionActive = false;

		blinkTimer += GetFrameTime();
		if (blinkTimer >= 0.5f)
		{
			blinkTimer = 0.0f;
			blinkShow = !blinkShow;
		}

		for (it = cursorTrail.begin(); it != cursorTrail.end(); )
		{
			it->alpha -= GetFrameTime() * config.cursorTrailDecaySpeed;
			if (it->alpha <= 0.0f)
				it = cursorTrail.erase(it);
			else
				++it;
		}

		BeginDrawing();
		ClearBackground(config.bgColor);

		mousePos = GetMousePosition();
		drawTitleBarButtons(mousePos, GetScreenWidth());

		// Draw split panel line
		if (isTerminalOpen)
			DrawLine(GetScreenWidth() / 2, config.paddingVertical, GetScreenWidth() / 2, GetScreenHeight(), config.cursorColor);

		// Render code lines
		for (i = 0; i < lines.size(); ++i)
		{
			line = lines[i];
			currentY = config.paddingVertical + i * verticalStride - scrollOffsetY;

			if (currentY + verticalStride < 50.0f || currentY > GetScreenHeight())
				continue;

			if (selectionActive && isEditorFocused)
			{
				getSelectionBounds(selectionStartLine, selectionStartCol, selectionEndLine, selectionEndCol, minL, minC, maxL, maxC);
				if (i >= minL && i <= maxL)
				{
					if (i == minL && i == maxL)
					{
						currentX = editorStartX + calculateTextWidth(textFont, textFontBold, line, config.tabSize, fontSize, fontSpacing, minC, config.keywordColor, config.typeColor, config.stringColor, config.commentColor, config.textColor);
						width = calculateTextWidth(textFont, textFontBold, line, config.tabSize, fontSize, fontSpacing, maxC, config.keywordColor, config.typeColor, config.stringColor, config.commentColor, config.textColor) - (currentX - editorStartX);
					}
					else if (i == minL)
					{
						currentX = editorStartX + calculateTextWidth(textFont, textFontBold, line, config.tabSize, fontSize, fontSpacing, minC, config.keywordColor, config.typeColor, config.stringColor, config.commentColor, config.textColor);
						width = calculateTextWidth(textFont, textFontBold, line, config.tabSize, fontSize, fontSpacing, line.length(), config.keywordColor, config.typeColor, config.stringColor, config.commentColor, config.textColor) - (currentX - editorStartX);
					}
					else if (i == maxL)
					{
						currentX = editorStartX;
						width = calculateTextWidth(textFont, textFontBold, line, config.tabSize, fontSize, fontSpacing, maxC, config.keywordColor, config.typeColor, config.stringColor, config.commentColor, config.textColor);
					}
					else
					{
						currentX = editorStartX;
						width = calculateTextWidth(textFont, textFontBold, line, config.tabSize, fontSize, fontSpacing, line.length(), config.keywordColor, config.typeColor, config.stringColor, config.commentColor, config.textColor);
					}
					
					// Clip selection within editor screen boundaries
					if (currentX < editorStartX)
					{
						width -= (editorStartX - currentX);
						currentX = editorStartX;
					}
					if (currentX + width > editorEndX)
						width = editorEndX - currentX;

					if (width > 0.0f)
					{
						pos = SnapToPixel(makeVector2(currentX, currentY), scale);
						DrawRectangleRec(Rectangle{pos.x, pos.y, width, static_cast<float>(verticalStride)}, config.selectionColor);
					}
				}
			}

			tokens = tokenize(line, config.keywordColor, config.typeColor, config.stringColor, config.commentColor, config.textColor);
			currentX = editorStartX;

			for (tokenIndex = 0; tokenIndex < tokens.size(); ++tokenIndex)
			{
				token = tokens[tokenIndex];
				if (token.text.empty())
					continue;

				bool boundaryReached = false;
				const char* ptr = token.text.c_str();
				while (*ptr != '\0')
				{
					if (currentX > editorEndX - 20.0f)
					{
						boundaryReached = true;
						break;
					}

					int bytesProcessed = 0;
					int codepoint = GetCodepointNext(ptr, &bytesProcessed);
					if (bytesProcessed == 0)
						break;

					Font activeFont = token.isBold ? textFontBold : textFont;
					if (codepoint == '\t')
					{
						float tabWidth = charWidth * config.tabSize;
						float relativeX = currentX - editorStartX;
						int currentTabStop = static_cast<int>(relativeX / tabWidth);
						currentX = editorStartX + (currentTabStop + 1) * tabWidth;
					}
					else
					{
						drawPos = makeVector2(currentX, currentY);
						snappedDrawPos = SnapToPixel(drawPos, scale);
						
						if (activeFont.texture.id != 0)
							DrawTextCodepoint(activeFont, codepoint, snappedDrawPos, static_cast<float>(fontSize), token.color);
						else
						{
							std::string singleChar(1, static_cast<char>(codepoint));
							DrawText(singleChar.c_str(), snappedDrawPos.x, snappedDrawPos.y, fontSize, token.color);
						}
						
						currentX += charWidth;
					}
					ptr += bytesProcessed;
				}
				if (boundaryReached)
					break;
			}
		}

		// Draw Cursor Trail (only if editor is focused)
		if (isEditorFocused)
		{
			for (trailIndex = 0; trailIndex < cursorTrail.size(); ++trailIndex)
			{
				tp = cursorTrail[trailIndex];
				trailColor = config.cursorColor;
				trailColor.a = static_cast<unsigned char>(tp.alpha * 255.0f);
				tpY = tp.pos.y - scrollOffsetY;
				if (tpY + fontSize >= 50.0f && tpY <= GetScreenHeight() && tp.pos.x >= editorStartX && tp.pos.x <= editorEndX)
				{
					pos = SnapToPixel(makeVector2(tp.pos.x, tpY), scale);
					DrawRectangleRec(Rectangle{pos.x, pos.y, tp.width, static_cast<float>(fontSize)}, trailColor);
				}
			}

			if (blinkShow && cursorTargetX >= editorStartX && cursorTargetX <= editorEndX)
			{
				cY = cursorTargetY - scrollOffsetY;
				if (cY + fontSize >= 50.0f && cY <= GetScreenHeight())
				{
					pos = SnapToPixel(makeVector2(cursorTargetX, cY), scale);
					DrawRectangleRec(Rectangle{pos.x, pos.y, 2.0f, static_cast<float>(fontSize)}, config.cursorColor);
				}
			}
		}

		// Draw Autocomplete Popup (only if editor is focused)
		if (autocompleteActive && !suggestions.empty() && isEditorFocused && cursorTargetX >= editorStartX && cursorTargetX <= editorEndX - 200.0f)
		{
			float popupX = cursorTargetX;
			float popupY = cursorTargetY - scrollOffsetY + verticalStride;
			float popupWidth = 200.0f;
			float popupHeight = suggestions.size() * 20.0f + 6.0f;
			
			DrawRectangleRounded(Rectangle{popupX, popupY, popupWidth, popupHeight}, 0.1f, 4, GetColor(0x1e1e2eff));
			DrawRectangleRoundedLines(Rectangle{popupX, popupY, popupWidth, popupHeight}, 0.1f, 4, config.cursorColor);
			
			for (sIdx = 0; sIdx < suggestions.size(); ++sIdx)
			{
				float sugY = popupY + 3.0f + sIdx * 20.0f;
				if (static_cast<int>(sIdx) == activeSuggestion)
				{
					DrawRectangleRec(Rectangle{popupX + 3.0f, sugY, popupWidth - 6.0f, 18.0f}, config.selectionColor);
				}
				if (isCustomFontLoaded)
					DrawTextEx(textFont, suggestions[sIdx].c_str(), makeVector2(popupX + 8.0f, sugY + 2.0f), static_cast<float>(fontSize - 2), fontSpacing, config.textColor);
				else
					DrawText(suggestions[sIdx].c_str(), static_cast<int>(popupX + 8.0f), static_cast<int>(sugY + 2.0f), fontSize - 2, config.textColor);
			}
		}

		// Draw Embedded PTY Terminal on the Left Split
		if (isTerminalOpen)
		{
			float	termX(15.0f);
			
			for (int r = 0; r < termRows; ++r)
			{
				float termY = config.paddingVertical + r * verticalStride;
				for (int c = 0; c < termCols; ++c)
				{
					if (termGrid[r][c].c != ' ')
					{
						std::string cellStr(1, termGrid[r][c].c);
						drawPos = makeVector2(termX + c * charWidth, termY);
						snappedDrawPos = SnapToPixel(drawPos, scale);
						if (isCustomFontLoaded)
							DrawTextEx(textFont, cellStr.c_str(), snappedDrawPos, static_cast<float>(fontSize), fontSpacing, termGrid[r][c].fg);
						else
							DrawText(cellStr.c_str(), snappedDrawPos.x, snappedDrawPos.y, fontSize, termGrid[r][c].fg);
					}
				}
			}

			// Render Terminal Cursor (only if terminal is focused)
			if (!isEditorFocused && !isAiFocused && blinkShow)
			{
				float curX = termX + termCursorCol * charWidth;
				float curY = config.paddingVertical + termCursorRow * verticalStride;
				pos = SnapToPixel(makeVector2(curX, curY), scale);
				DrawRectangleRec(Rectangle{pos.x, pos.y, charWidth, static_cast<float>(fontSize)}, config.cursorColor);
			}
		}

		// Draw AI Panel in a Floating Box
		if (isAiPanelOpen)
		{
			float	floatX = GetScreenWidth() - 420.0f;
			float	floatY = 80.0f;
			float	floatW = 400.0f;
			float	floatH = GetScreenHeight() - 160.0f;

			// Draw Glassmorphic Card Background & Border
			DrawRectangleRounded(Rectangle{floatX, floatY, floatW, floatH}, 0.05f, 4, GetColor(0x13131ffa));
			DrawRectangleRoundedLines(Rectangle{floatX, floatY, floatW, floatH}, 0.05f, 4, config.cursorColor);

			// Draw AI Header/Title Bar
			DrawRectangleRounded(Rectangle{floatX + 1.0f, floatY + 1.0f, floatW - 2.0f, 35.0f}, 0.05f, 4, GetColor(0x1a1a26ff));
			if (isCustomFontLoaded)
				DrawTextEx(textFont, "AI Assistant (F2)", makeVector2(floatX + 15.0f, floatY + 8.0f), 15.0f, fontSpacing, config.textColor);
			else
				DrawText("AI Assistant (F2)", floatX + 15, floatY + 8, 15, config.textColor);

			// Draw Chat Messages Logs
			float chatY = floatY + 45.0f - aiScrollY;
			float chatMaxWidth = floatW - 20.0f;
			
			{
				std::lock_guard<std::mutex>	lock(chatMutex);
				for (const auto& msg : chatHistory)
				{
					Color roleColor = msg.role == "user" ? PINK : SKYBLUE;
					t_text roleLabel = msg.role == "user" ? "USER:" : "ASSISTANT:";
					
					if (chatY + 20.0f >= floatY + 40.0f && chatY <= floatY + floatH - 170.0f)
					{
						if (isCustomFontLoaded)
							DrawTextEx(textFontBold, roleLabel.c_str(), makeVector2(floatX + 15.0f, chatY), 13.0f, fontSpacing, roleColor);
						else
							DrawText(roleLabel.c_str(), floatX + 15, chatY, 13, roleColor);
					}
					chatY += 16.0f;

					std::vector<t_text> wrapped = wrapText(textFont, msg.text, chatMaxWidth - 10.0f, 13.0f, fontSpacing);
					for (const auto& wl : wrapped)
					{
						if (chatY + 20.0f >= floatY + 40.0f && chatY <= floatY + floatH - 170.0f)
						{
							if (isCustomFontLoaded)
								DrawTextEx(textFont, wl.c_str(), makeVector2(floatX + 20.0f, chatY), 13.0f, fontSpacing, config.textColor);
							else
								DrawText(wl.c_str(), floatX + 20, chatY, 13, config.textColor);
						}
						chatY += 16.0f;
					}
					chatY += 12.0f;
				}
			}

			// Draw input bar background & controls box at the bottom of the card
			DrawRectangleRec(Rectangle{floatX + 1.0f, floatY + floatH - 160.0f, floatW - 2.0f, 158.0f}, GetColor(0x13131ffa));
			DrawLine(floatX, floatY + floatH - 160.0f, floatX + floatW, floatY + floatH - 160.0f, config.cursorColor);

			// Draw F3 Speak Voice Button
			Color voiceBtnColor = isRecording ? RED : GetColor(0x2a2a3eff);
			DrawRectangleRounded(Rectangle{floatX + 10.0f, floatY + floatH - 120.0f, 170.0f, 25.0f}, 0.2f, 4, voiceBtnColor);
			t_text voiceBtnTxt = isRecording ? "[ STOP REC ]" : "F3 / Speak Voice";
			if (isCustomFontLoaded)
				DrawTextEx(textFont, voiceBtnTxt.c_str(), makeVector2(floatX + 22.0f, floatY + floatH - 114.0f), 12.0f, fontSpacing, WHITE);
			else
				DrawText(voiceBtnTxt.c_str(), floatX + 22, floatY + floatH - 114, 12, WHITE);

			// Draw Code Analyze Button
			DrawRectangleRounded(Rectangle{floatX + 210.0f, floatY + floatH - 120.0f, 180.0f, 25.0f}, 0.2f, 4, GetColor(0x2a2a3eff));
			t_text analyzeTxt = selectionActive ? "Analyze Selected" : "Explain File";
			if (isCustomFontLoaded)
				DrawTextEx(textFont, analyzeTxt.c_str(), makeVector2(floatX + 222.0f, floatY + floatH - 114.0f), 12.0f, fontSpacing, WHITE);
			else
				DrawText(analyzeTxt.c_str(), floatX + 222, floatY + floatH - 114, 12, WHITE);

			// Draw Input Box outline
			Color inputOutlineColor = isAiFocused ? config.cursorColor : GetColor(0x3a3a4eff);
			DrawRectangleRounded(Rectangle{floatX + 10.0f, floatY + floatH - 80.0f, 380.0f, 30.0f}, 0.15f, 4, GetColor(0x090910ff));
			DrawRectangleRoundedLines(Rectangle{floatX + 10.0f, floatY + floatH - 80.0f, 380.0f, 30.0f}, 0.15f, 4, inputOutlineColor);

			t_text placeholder = "";
			if (isAiThinking)
				placeholder = "[ AI is thinking... ]";
			else if (isAudioTranscribing)
				placeholder = "[ Transcribing voice... ]";
			else if (isRecording)
				placeholder = "[ Recording micro... ]";
			else if (aiInputPrompt.empty())
				placeholder = "Pergunte ao Assistente (Enter)...";
			else
				placeholder = aiInputPrompt;

			Color promptColor = (aiInputPrompt.empty() || isAiThinking || isAudioTranscribing || isRecording) ? GRAY : config.textColor;
			if (isCustomFontLoaded)
				DrawTextEx(textFont, placeholder.c_str(), makeVector2(floatX + 18.0f, floatY + floatH - 72.0f), 13.0f, fontSpacing, promptColor);
			else
				DrawText(placeholder.c_str(), floatX + 18, floatY + floatH - 72, 13, promptColor);

			// Clear Chat History Button
			DrawRectangleRounded(Rectangle{floatX + 10.0f, floatY + floatH - 40.0f, 140.0f, 25.0f}, 0.2f, 4, GetColor(0x221a2aff));
			if (isCustomFontLoaded)
				DrawTextEx(textFont, "Clear Chat History", makeVector2(floatX + 18.0f, floatY + floatH - 34.0f), 11.0f, fontSpacing, PINK);
			else
				DrawText("Clear Chat History", floatX + 18, floatY + floatH - 34, 11, PINK);
		}

		if (dialogState != STATE_NONE)
			drawBottomDialog(textFont, fontSize, fontSpacing, dialogInput, dialogState, config.cursorColor, config.textColor, GetScreenHeight(), GetScreenWidth());

		EndDrawing();
	}

	if (textFont.texture.id != 0)
		UnloadFont(textFont);
	if (textFontBold.texture.id != 0)
		UnloadFont(textFontBold);

	// Close PTY session securely
	if (masterFd >= 0)
		close(masterFd);

	return (CloseWindow(), 0);
}
