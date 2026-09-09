/* ************************************************************************** */
/* */
/* :::      ::::::::   */
/* sscode.cpp                                         :+:      :+:    :+:   */
/* +:+ +:+         +:+     */
/* By: Ser Superior <marcioeduine@gmail.com>       +#+  +:+       +#+        */
/* +#+#+#+#+#+   +#+           */
/* Created: 2026/07/02 01:38:25 by Ser Superior          #+#    #+#             */
/* Updated: 2026/07/02 16:20:00 by Ser Superior         ###   ########.fr       */
/* */
/* ************************************************************************** */

#include <termios.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <dirent.h>
#include <algorithm>
#include <cctype>

#define CTRL_KEY(k) ((k) & 0x1f)
#define QUIT_COMMAND     CTRL_KEY('q')
#define SAVE_COMMAND     CTRL_KEY('s')
#define TOGGLE_MODE      CTRL_KEY('o')
#define TOGGLE_DRAWER    CTRL_KEY('b')
#define UNDO_COMMAND     CTRL_KEY('z')
#define COPY_COMMAND     CTRL_KEY('c')
#define PASTE_COMMAND    CTRL_KEY('v')
#define SELECT_ALL       CTRL_KEY('a')
#define PREV_TAB_COMMAND CTRL_KEY('h')
#define NEXT_TAB_COMMAND CTRL_KEY('l')
#define AI_COMMAND       CTRL_KEY('g')

enum e_editorMode {
    MODE_EDITOR,
    MODE_FILE_MANAGER,
    MODE_AI_PROMPT
};

enum e_editorKey {
    ARROW_LEFT = 1000,
    ARROW_RIGHT,
    ARROW_UP,
    ARROW_DOWN,
    PAGE_UP,
    PAGE_DOWN,
    HOME_KEY,
    END_KEY,
    DEL_KEY,
    MOUSE_EVENT_IGNORE
};

struct FileBuffer {
    std::vector<std::string> lines;
    std::vector<std::vector<std::string> > undo_stack;
    std::string              name;
    int                      cursor_x;
    int                      cursor_y;
    int                      row_offset;
    bool                     is_dirty;
};

// --- ESTADO GLOBAL ---
struct termios          orig_termios;
std::vector<FileBuffer> open_files;
int                     active_file_idx = 0;
e_editorMode            current_mode = MODE_EDITOR;
bool                    drawer_open = false;
int                     screen_rows = 0;
int                     screen_cols = 0;

// --- SUBSISTEMA DA JANELA DE IA ---
bool                     ai_panel_open = false;
std::vector<std::string> ai_response_lines;
std::string              ai_current_input = "";
int                      ai_prompt_cursor = 0;
std::string              ai_full_response = "";
bool                     should_quit = false;

void LoadDirectoryFiles(void);
void SaveFile(void);
void WrapText(const std::string& text, int max_width, std::vector<std::string>& output);

std::vector<std::string> clipboard;
std::vector<std::string> file_list;
int                      selected_file_idx = 0;

const char* keywords[] = { "switch", "if", "else", "while", "for", "break", "return", "define", "include", "and", "or", "not", "xor" };
const char* datatypes[] = { "int", "char", "void", "struct", "class", "bool", "char32_t", "size_t" };

// --- PALETA DE CORES TRUE COLOR (RGB 24-BITS) ---
#define RGB_BG_DARK        "\x1b[48;2;13;17;23m"
#define RGB_TAB_BG         "\x1b[48;2;22;27;34m"
#define RGB_TAB_ACT        "\x1b[48;2;47;129;247;38;2;255;255;255;1m"
#define RGB_TEXT           "\x1b[38;2;230;237;243m"
#define RGB_KEYWORD        "\x1b[38;2;121;192;255m"
#define RGB_DATATYPE       "\x1b[38;2;166;227;161m"
#define RGB_STRING         "\x1b[38;2;201;209;217m"
#define RGB_COMMENT        "\x1b[38;2;139;148;158m"
#define RGB_PREPROC        "\x1b[38;2;191;135;255m"
#define RGB_NUMBER         "\x1b[38;2;255;166;87m"
#define RGB_GUTTER         "\x1b[38;2;110;118;129m"
#define RGB_AI_PANEL       "\x1b[48;2;16;21;29m"
#define RGB_AI_TEXT        "\x1b[38;2;201;209;217m"
#define RGB_HEADER_BG      "\x1b[48;2;9;14;21m"
#define RGB_HEADER_ACCENT  "\x1b[38;2;88;166;255;1m"
#define RGB_STATUS_BG      "\x1b[48;2;22;27;34m"
#define RGB_STATUS_TEXT    "\x1b[38;2;139;148;158m"
#define RGB_RESET          "\x1b[0m"

// --- MANIPULAÇÃO UTF-8 ---
int GetUtf8CharLength(unsigned char c) {
    if ((c & 0x80) == 0) return 1;
    if ((c & 0xE0) == 0xC0) return 2;
    if ((c & 0xF0) == 0xE0) return 3;
    if ((c & 0xF8) == 0xF0) return 4;
    return 1;
}

std::vector<std::string> SplitUtf8String(const std::string& str) {
    std::vector<std::string> chars;
    size_t i = 0;
    while (i < str.length()) {
        int len = GetUtf8CharLength(static_cast<unsigned char>(str[i]));
        if (i + len <= str.length()) {
            chars.push_back(str.substr(i, len));
            i += len;
        } else {
            chars.push_back(str.substr(i, 1));
            i++;
        }
    }
    return chars;
}

std::string JoinUtf8Chars(const std::vector<std::string>& chars) {
    std::string out = "";
    for (size_t i = 0; i < chars.size(); ++i) out += chars[i];
    return out;
}

size_t GetUtf8Width(const std::string& str) {
    size_t count = 0;
    size_t i = 0;
    while (i < str.length()) {
        int len = GetUtf8CharLength(static_cast<unsigned char>(str[i]));
        count++;
        i += len;
    }
    return count;
}

bool IsUtf8LeadByte(unsigned char b) {
    return ((b >= 0xC2 and b <= 0xDF) or (b >= 0xE0 and b <= 0xEF) or (b >= 0xF0 and b <= 0xF4));
}

bool IsUtf8ContinuationByte(unsigned char b) {
    return (b >= 0x80 and b <= 0xBF);
}

// --- ENGINE DO TERMINAL ---
void GetTerminalSize(void) {
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 or ws.ws_col == 0) {
        screen_rows = 24;
        screen_cols = 80;
    } else {
        screen_rows = ws.ws_row;
        screen_cols = ws.ws_col;
    }
}

void disableRawMode(void) {
    std::cout << "\x1b[?1049l" << RGB_RESET << std::flush;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

void enableRawMode(void) {
    tcgetattr(STDIN_FILENO, &orig_termios);
    atexit(disableRawMode);
    std::cout << "\x1b[?1049h\x1b[H" << std::flush;

    struct termios raw = orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON | ISIG | IEXTEN);
    raw.c_iflag &= ~(IXON | ICRNL | BRKINT | INPCK | ISTRIP);
    raw.c_oflag &= ~(OPOST);
    
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;

    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
    GetTerminalSize();
}

void SaveUndoState(FileBuffer& fb) {
    if (fb.undo_stack.size() > 50) fb.undo_stack.erase(fb.undo_stack.begin());
    fb.undo_stack.push_back(fb.lines);
    fb.is_dirty = true;
}

void OpenFileBuffer(const std::string& filename) {
    std::string clean_name = filename;
    size_t icon_sep = clean_name.find(' ');
    std::string icon_prefix = "";
    if (icon_sep != std::string::npos) {
        icon_prefix = clean_name.substr(0, icon_sep);
        if (icon_prefix == "" or icon_prefix == "󰈚")
            clean_name = clean_name.substr(icon_sep + 1);
    }
    if (!clean_name.empty() and clean_name[clean_name.length() - 1] == '/')
        clean_name = clean_name.substr(0, clean_name.length() - 1);

    for (size_t i = 0; i < open_files.size(); ++i) {
        if (open_files[i].name == clean_name) {
            active_file_idx = static_cast<int>(i);
            if (open_files[i].lines.empty()) open_files[i].lines.push_back("");
            if (open_files[i].cursor_y >= static_cast<int>(open_files[i].lines.size()))
                open_files[i].cursor_y = static_cast<int>(open_files[i].lines.size() - 1);
            if (open_files[i].cursor_y < 0) open_files[i].cursor_y = 0;
            if (open_files[i].row_offset > open_files[i].cursor_y) open_files[i].row_offset = open_files[i].cursor_y;
            if (open_files[i].row_offset < 0) open_files[i].row_offset = 0;
            return;
        }
    }

    FileBuffer fb;
    fb.name = clean_name;
    fb.cursor_x = 0;
    fb.cursor_y = 0;
    fb.row_offset = 0;
    fb.is_dirty = false;

    std::ifstream file(clean_name.c_str());
    if (file.is_open()) {
        std::string line;
        while (std::getline(file, line)) fb.lines.push_back(line);
        file.close();
    }
    if (fb.lines.empty()) fb.lines.push_back("");

    open_files.push_back(fb);
    active_file_idx = static_cast<int>(open_files.size() - 1);
}

void InitializeEditor(int argc, char **argv) {
    if (argc >= 2) {
        for (int i = 1; i < argc; ++i) OpenFileBuffer(argv[i]);
    } else {
        OpenFileBuffer("");
    }
}

void LoadDirectoryFiles(void) {
    file_list.clear();
    DIR *dir = opendir(".");
    if (dir == NULL) return;
    struct dirent *entity;
    while ((entity = readdir(dir)) != NULL) {
        std::string name = entity->d_name;
        if (name != "." and name != "..") {
            if (entity->d_type == DT_DIR) file_list.push_back(" " + name + "/");
            else file_list.push_back("󰈚 " + name);
        }
    }
    closedir(dir);
    std::sort(file_list.begin(), file_list.end());
}

void SaveFile(void) {
    FileBuffer& cur = open_files[active_file_idx];
    if (cur.name.empty()) cur.name = "out.txt";
    std::ofstream file(cur.name.c_str());
    if (file.is_open()) {
        for (size_t i = 0; i < cur.lines.size(); ++i) file << cur.lines[i] << "\n";
        file.close();
        cur.is_dirty = false;
    }
}

bool is_in_list(const std::string& word, const char* list[], int size) {
    for (int i = 0; i < size; ++i) if (word == list[i]) return true;
    return false;
}

void ScrollEditor(void) {
    FileBuffer& cur = open_files[active_file_idx];
    int edit_window_height = screen_rows - 4;
    if (edit_window_height < 1) edit_window_height = 1;

    if (cur.cursor_y < cur.row_offset) cur.row_offset = cur.cursor_y;
    if (cur.cursor_y >= cur.row_offset + edit_window_height) cur.row_offset = cur.cursor_y - edit_window_height + 1;
}

std::string BuildHorizontalLine(int width) {
    std::string line = "";
    for (int i = 0; i < width; ++i) line += "─";
    return line;
}

std::string TrimCopy(const std::string& s) {
    size_t start = 0;
    size_t end = 0;

    if (s.empty()) return "";
    start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

std::string SanitizeForUi(const std::string& raw) {
    std::string clean = "";
    size_t i = 0;
    unsigned char b = 0;

    while (i < raw.length()) {
        b = static_cast<unsigned char>(raw[i]);
        if (b == 0x1B) {
            if (i + 1 < raw.length() and raw[i + 1] == '[') {
                i += 2;
                while (i < raw.length()) {
                    unsigned char f = static_cast<unsigned char>(raw[i]);
                    if (f >= 0x40 and f <= 0x7E) { i++; break; }
                    i++;
                }
                continue;
            }
            if (i + 1 < raw.length() and raw[i + 1] == ']') {
                i += 2;
                while (i < raw.length() and raw[i] != '\a') i++;
                if (i < raw.length()) i++;
                continue;
            }
            i++;
            continue;
        }
        if (raw[i] == '\r') { i++; continue; }
        if (raw[i] == '\t') { clean += "    "; i++; continue; }
        if (raw[i] == '\n') { clean += '\n'; i++; continue; }
        if (b < 32 or b == 127) { i++; continue; }
        clean += raw[i];
        i++;
    }
    return clean;
}

void SetAiMessage(const std::string& msg) {
    int effective_ai_text_width = screen_cols - 4;
    std::string clean_msg = SanitizeForUi(msg);

    if (effective_ai_text_width < 10) effective_ai_text_width = 10;
    ai_response_lines.clear();
    WrapText(clean_msg, effective_ai_text_width, ai_response_lines);
    if (ai_response_lines.empty()) ai_response_lines.push_back("(sem conteúdo)");
    ai_panel_open = true;
}

bool HandlePromptCommand(const std::string& prompt) {
    std::string line = TrimCopy(prompt);
    std::string cmd = "";
    std::string arg = "";
    size_t sp = 0;

    if (line.empty() or line[0] != '/') return false;
    sp = line.find(' ');
    if (sp == std::string::npos) cmd = line.substr(1);
    else {
        cmd = line.substr(1, sp - 1);
        arg = TrimCopy(line.substr(sp + 1));
    }
    std::transform(cmd.begin(), cmd.end(), cmd.begin(), [](unsigned char ch){ return static_cast<char>(std::tolower(ch)); });

    if (cmd == "help") {
        SetAiMessage("Comandos:\n/help\n/editor\n/files\n/drawer\n/save\n/open <ficheiro>\n/clear\n/quit");
        return true;
    }
    if (cmd == "editor") {
        current_mode = MODE_EDITOR;
        SetAiMessage("Modo editor activo.");
        return true;
    }
    if (cmd == "clear") {
        std::string clean_cmd = "./ss_ai_bridge.py '__CLEAR_CONTEXT__' > /dev/null 2>&1";
        system(clean_cmd.c_str());
        ai_response_lines.clear();
        ai_panel_open = false;
        return true;
    }
    if (cmd == "files") {
        if (current_mode == MODE_FILE_MANAGER) {
            current_mode = MODE_EDITOR;
            drawer_open = false;
            SetAiMessage("Modo ficheiros fechado.");
        } else {
            LoadDirectoryFiles();
            selected_file_idx = 0;
            current_mode = MODE_FILE_MANAGER;
            drawer_open = true;
            SetAiMessage("Modo ficheiros activo.");
        }
        return true;
    }
    if (cmd == "drawer") {
        return HandlePromptCommand("/files");
    }
    if (cmd == "save") {
        if (!open_files.empty()) SaveFile();
        SetAiMessage("Ficheiro guardado.");
        return true;
    }
    if (cmd == "open") {
        if (arg.empty()) {
            SetAiMessage("Uso: /open <ficheiro>");
            return true;
        }
        OpenFileBuffer(arg);
        current_mode = MODE_EDITOR;
        SetAiMessage("Ficheiro aberto: " + arg);
        return true;
    }
    if (cmd == "quit" or cmd == "exit") {
        should_quit = true;
        return true;
    }
    SetAiMessage("Comando desconhecido: /" + cmd + "\nUsa /help.");
    return true;
}

std::string FitText(const std::string& text, int width) {
    std::string out = "";

    if (width <= 0) return "";
    if (static_cast<int>(text.length()) <= width) {
        out = text;
        return out;
    }
    if (width <= 3) return text.substr(0, width);
    out = text.substr(0, width - 3) + "...";
    return out;
}

// --- ALGORITMO DE WORD WRAP DINÂMICO ---
void WrapText(const std::string& text, int max_width, std::vector<std::string>& output) {
    std::vector<std::string> words;
    std::string current_word = "";
    
    for (size_t i = 0; i < text.length(); ++i) {
        if (text[i] == '\n') {
            if (!current_word.empty()) { words.push_back(current_word); current_word = ""; }
            words.push_back("\n");
        } else if (text[i] == ' ') {
            if (!current_word.empty()) { words.push_back(current_word); current_word = ""; }
            words.push_back(" ");
        } else {
            current_word += text[i];
        }
    }
    if (!current_word.empty()) words.push_back(current_word);

    std::string current_line = "";
    for (size_t i = 0; i < words.size(); ++i) {
        if (words[i] == "\n") {
            output.push_back(current_line);
            current_line = "";
        } else {
            if (current_line.length() + words[i].length() > static_cast<size_t>(max_width)) {
                if (!current_line.empty()) output.push_back(current_line);
                current_line = (words[i] == " ") ? "" : words[i];
            } else {
                current_line += words[i];
            }
        }
    }
    if (!current_line.empty()) output.push_back(current_line);
}

// --- LEXER DE SINTAXE REALISTA ---
void RenderHighlightedLine(const std::string& line, int max_width)
{
    std::vector<std::string> chars = SplitUtf8String(line);
    std::string current_word = "";
    bool in_string = false;
    int current_printed_width = 0;

    if (!chars.empty() and chars[0] == "#") {
        std::cout << RGB_PREPROC << (line.length() > static_cast<size_t>(max_width) ? line.substr(0, max_width) : line) << RGB_RESET;
        return;
    }

    for (size_t j = 0; j < chars.size(); ++j) {
        if (current_printed_width >= max_width) break;
        std::string c = chars[j];

        if (!in_string and j + 1 < chars.size() and c == "/" and chars[j + 1] == "/") {
            std::cout << RGB_COMMENT;
            for (size_t rem = j; rem < chars.size() and current_printed_width < max_width; ++rem) {
                std::cout << chars[rem];
                current_printed_width++;
            }
            std::cout << RGB_RESET;
            return;
        }

        if (c == "\"") {
            if (in_string) {
                current_word += c;
                std::cout << RGB_STRING << current_word << RGB_RESET;
                current_printed_width += SplitUtf8String(current_word).size();
                current_word = "";
                in_string = false;
            } else {
                std::cout << RGB_TEXT << current_word << RGB_RESET;
                current_printed_width += SplitUtf8String(current_word).size();
                current_word = "\"";
                in_string = true;
            }
            continue;
        }

        if (in_string) {
            current_word += c;
            continue;
        }

        if (c == " " or c == "(" or c == ")" or c == "{" or c == "}" or c == ";" or c == "," or c == "<" or c == ">" or c == "\t") {
            if (is_in_list(current_word, keywords, 13)) std::cout << RGB_KEYWORD << current_word << RGB_RESET;
            else if (is_in_list(current_word, datatypes, 8)) std::cout << RGB_DATATYPE << current_word << RGB_RESET;
            else if (current_word.find_first_not_of("0123456789") == std::string::npos and not current_word.empty())
                std::cout << RGB_NUMBER << current_word << RGB_RESET;
            else std::cout << RGB_TEXT << current_word << RGB_RESET;

            current_printed_width += SplitUtf8String(current_word).size();

            if (c == "\t") { std::cout << "    "; current_printed_width += 4; }
            else { std::cout << RGB_TEXT << c << RGB_RESET; current_printed_width++; }
            current_word = "";
        } else {
            current_word += c;
        }
    }
    if (current_printed_width < max_width) {
        if (is_in_list(current_word, keywords, 13)) std::cout << RGB_KEYWORD << current_word << RGB_RESET;
        else if (is_in_list(current_word, datatypes, 8)) std::cout << RGB_DATATYPE << current_word << RGB_RESET;
        else std::cout << RGB_TEXT << current_word << RGB_RESET;
        current_printed_width += SplitUtf8String(current_word).size();
    }

    for (int fill = current_printed_width; fill < max_width; ++fill) std::cout << " ";
}

// --- EXECUÇÃO DO PROMPT DA IA ---
void    ExecuteAIPrompt(void)
{
    if (ai_current_input.empty() or ai_current_input.find_first_not_of(" \t\r\n") == std::string::npos) {
        current_mode = MODE_EDITOR;
        return;
    }
    if (HandlePromptCommand(ai_current_input)) {
        // O HandlePromptCommand já ajustou o modo (ex: para FILE_MANAGER). 
        // Não forçamos MODE_EDITOR aqui se o comando foi de ficheiros.
        ai_current_input = "";
        return;
    }

    std::string escaped_prompt = "";
    for (size_t i = 0; i < ai_current_input.length(); ++i) {
        if (ai_current_input[i] == '\'') escaped_prompt += "'\\''";
        else escaped_prompt += ai_current_input[i];
    }

    std::cout << "\x1b[" << screen_rows << ";1H\x1b[48;2;235;160;0m\x1b[38;2;0;0;0;1m 󱚥 A PROCESSAR IA COM CONTEXTO... \x1b[0m\x1b[K" << std::flush;

    std::string cmd = "./ss_ai_bridge.py '" + escaped_prompt + "' 2>&1";
    FILE* pipe = popen(cmd.c_str(), "r");
    if (not pipe) return;

    ai_response_lines.clear();
    char buffer[512];
    std::string full_response = "";

    while (fgets(buffer, sizeof(buffer), pipe) != NULL) {
        full_response += buffer;
    }
    pclose(pipe);
    full_response = SanitizeForUi(full_response);
    if (TrimCopy(full_response).empty())
        full_response = "// Sem resposta do backend IA. Verifica Ollama/llama.cpp.";

    int effective_ai_text_width = screen_cols - 4;
    if (effective_ai_text_width < 10) effective_ai_text_width = 10;
    WrapText(full_response, effective_ai_text_width, ai_response_lines);
    if (ai_response_lines.empty()) ai_response_lines.push_back("(sem conteúdo)");

    ai_panel_open = true;
    current_mode = MODE_EDITOR;
    ai_current_input = "";
}

// --- PIPELINE DE RENDERIZAÇÃO ---
void RefreshScreen(void)
{
    GetTerminalSize();
    ScrollEditor();

    std::cout << "\x1b[?25l\x1b[H" << RGB_BG_DARK;

    if (current_mode == MODE_FILE_MANAGER && !drawer_open) {
        drawer_open = true;
    }

    // 1. HEADER + TABLINE NO ESTILO COPILOT CLI
    std::string top_left = " SSCodeIDE ";
    std::string top_buttons = " [Save:^S] [Open:^O] [Drawer:^B] [AI:^G] ";
    std::string top_right = (ai_panel_open ? "AI ON" : "AI OFF");
    top_right += drawer_open ? " • DRAWER ON" : " • DRAWER OFF";
    int top_fill = screen_cols - static_cast<int>(top_left.length() + top_buttons.length() + top_right.length());
    if (top_fill < 1) top_fill = 1;
    std::cout << RGB_HEADER_BG << RGB_HEADER_ACCENT << top_left << RGB_TEXT << top_buttons;
    for (int t = 0; t < top_fill; ++t) std::cout << " ";
    std::cout << RGB_STATUS_TEXT << top_right << RGB_RESET << "\x1b[K\r\n";

    std::cout << RGB_TAB_BG << " ";
    for (size_t i = 0; i < open_files.size(); ++i) {
        std::string dirty_flag = open_files[i].is_dirty ? " \x1b[31m[+]\x1b[0m" : "";
        std::string tab_name = open_files[i].name.empty() ? "*Sem Nome*" : open_files[i].name;
        tab_name = FitText(tab_name, 24);
        
        if (static_cast<int>(i) == active_file_idx) std::cout << RGB_TAB_ACT << " " << tab_name << dirty_flag << " " << RGB_TAB_BG;
        else std::cout << RGB_GUTTER << "│ " << RGB_TEXT << tab_name << dirty_flag << " ";
    }
    std::cout << "\x1b[K" << RGB_RESET << "\r\n";

    FileBuffer& cur = open_files[active_file_idx];
    if (cur.lines.empty()) cur.lines.push_back("");
    if (cur.row_offset < 0) cur.row_offset = 0;
    if (cur.row_offset >= static_cast<int>(cur.lines.size()))
        cur.row_offset = static_cast<int>(cur.lines.size() - 1);
    if (cur.cursor_y < 0) cur.cursor_y = 0;
    if (cur.cursor_y >= static_cast<int>(cur.lines.size()))
        cur.cursor_y = static_cast<int>(cur.lines.size() - 1);
    int visual_cursor_x = 0;
    int drawer_width = drawer_open ? 24 : 0;
    int line_number_width = 7;
    int ai_panel_height = ai_panel_open ? 8 : 0;
    int edit_window_height = screen_rows - 4 - ai_panel_height;

    int available_cols = screen_cols - drawer_width;
    int code_window_width = available_cols;
    int effective_code_text_width = code_window_width - line_number_width - 1; // -1 para scrollbar
    if (effective_code_text_width < 8) effective_code_text_width = 8;

    // SCROLLBAR CALCULATIONS
    int total_lines = cur.lines.size();
    int scroll_handle_size = (edit_window_height * edit_window_height) / (total_lines > 0 ? total_lines : 1);
    if (scroll_handle_size < 1) scroll_handle_size = 1;
    if (scroll_handle_size > edit_window_height) scroll_handle_size = edit_window_height;
    int scroll_handle_pos = 0;
    if (total_lines > edit_window_height) {
        scroll_handle_pos = (cur.row_offset * (edit_window_height - scroll_handle_size)) / (total_lines - edit_window_height);
    }

    // 2. VIEWPORTS LATERALIS
    for (int i = 0; i < edit_window_height; ++i)
    {
        int file_line_idx = i + cur.row_offset;
        bool is_handle = (i >= scroll_handle_pos && i < scroll_handle_pos + scroll_handle_size);
        std::string scroll_char = is_handle ? "█" : "│";

        if (drawer_open) {
            if (i < static_cast<int>(file_list.size())) {
                std::string entry = file_list[i];
                if (entry.length() > 19) entry = entry.substr(0, 17) + "..";
                int text_width = GetUtf8Width(entry);
                if (current_mode == MODE_FILE_MANAGER && i == selected_file_idx) {
                    std::cout << RGB_TAB_BG << RGB_HEADER_ACCENT << "> " << RGB_TEXT << entry << RGB_RESET;
                    text_width += 2;
                } else {
                    std::cout << "  " << RGB_STATUS_TEXT << entry << RGB_RESET;
                    text_width += 2;
                }
                int spaces = 22 - text_width;
                for (int s = 0; s < spaces; ++s) std::cout << " ";
                std::cout << RGB_GUTTER << "│" << RGB_RESET << " ";
            } else {
                std::cout << "                      " << RGB_GUTTER << "│" << RGB_RESET << " ";
            }
        }

        // JANELA DE CÓDIGO
        if (file_line_idx >= static_cast<int>(cur.lines.size())) {
            std::cout << RGB_GUTTER << "~\x1b[0m";
            for (int space = 1; space < effective_code_text_width + line_number_width; ++space) std::cout << " ";
            std::cout << RGB_GUTTER << scroll_char << RGB_RESET;
        } else {
            std::cout << RGB_GUTTER;
            if (file_line_idx + 1 < 10) std::cout << "   " << (file_line_idx + 1) << " │ ";
            else if (file_line_idx + 1 < 100) std::cout << "  " << (file_line_idx + 1) << " │ ";
            else std::cout << " " << (file_line_idx + 1) << " │ ";
            std::cout << RGB_RESET;

            if (file_line_idx == cur.cursor_y) {
                std::vector<std::string> l_chars = SplitUtf8String(cur.lines[file_line_idx]);
                visual_cursor_x = drawer_width + line_number_width;
                for (int k = 0; k < cur.cursor_x and k < static_cast<int>(l_chars.size()); ++k) {
                    if (l_chars[k] == "\t") visual_cursor_x += 4; else visual_cursor_x += 1;
                }
            }
            RenderHighlightedLine(cur.lines[file_line_idx], effective_code_text_width);
            std::cout << RGB_GUTTER << scroll_char << RGB_RESET;
        }

        std::cout << "\x1b[K\r\n";
    }

    // DRAW AI PANEL AT THE BOTTOM
    if (ai_panel_open) {
        std::cout << RGB_GUTTER << BuildHorizontalLine(drawer_width) << (drawer_width > 0 ? "┴" : "") 
                  << BuildHorizontalLine(screen_cols - drawer_width - (drawer_width > 0 ? 1 : 0)) << RGB_RESET << "\r\n";
        
        for (int i = 0; i < ai_panel_height - 1; ++i) {
            std::cout << RGB_AI_PANEL << " ";
            if (i == 0) {
                std::string panel_title = "SSCodeIDE - Chat";
                std::cout << RGB_HEADER_ACCENT << panel_title;
                for (int fill = panel_title.length(); fill < screen_cols - 2; ++fill) std::cout << " ";
            } else if (i - 1 < static_cast<int>(ai_response_lines.size())) {
                std::string ai_line = ai_response_lines[i - 1];
                std::cout << RGB_AI_TEXT << ai_line;
                int text_w = GetUtf8Width(ai_line);
                for (int fill = text_w; fill < screen_cols - 2; ++fill) std::cout << " ";
            } else {
                for (int fill = 0; fill < screen_cols - 2; ++fill) std::cout << " ";
            }
            std::cout << " " << RGB_RESET << "\x1b[K\r\n";
        }
    }

    // 3. BARRA INFERIOR / INTERFAÇO DE PROMPT
    if (current_mode != MODE_AI_PROMPT) {
        std::cout << RGB_GUTTER << BuildHorizontalLine(screen_cols) << RGB_RESET << "\r\n";
        std::string file_name = cur.name.empty() ? "[Sem Nome]" : cur.name;
        std::string mode = "NORMAL";
        std::string pos = "Ln " + std::to_string(cur.cursor_y + 1) + ", Col " + std::to_string(cur.cursor_x + 1);
        std::string hints = "/help /editor /files /drawer /save /open /clear /quit";
        std::string status = " " + mode + " • " + file_name + (cur.is_dirty ? " [+]" : "") + " • " + pos + " • " + hints;
        status = FitText(status, screen_cols);
        std::cout << RGB_STATUS_BG << RGB_STATUS_TEXT << status << RGB_RESET << "\x1b[K";
    }

    if (current_mode == MODE_AI_PROMPT) {
        // Floating centered box like LazyVim
        int box_width = 60;
        int start_y = screen_rows / 2 - 1;
        if (start_y < 1) start_y = 1;
        int start_x = (screen_cols - box_width) / 2;
        if (start_x < 1) start_x = 1;

        std::string title = " PROMPT ";
        std::cout << "\x1b[" << start_y << ";" << start_x << "H" << RGB_HEADER_BG << RGB_HEADER_ACCENT << "╭" << title << BuildHorizontalLine(box_width - 2 - title.length()) << "╮" << RGB_RESET;
        
        std::string content = " ❯ " + ai_current_input;
        std::string fitted_content = FitText(content, box_width - 2);
        std::cout << "\x1b[" << start_y + 1 << ";" << start_x << "H" << RGB_HEADER_BG << RGB_HEADER_ACCENT << "│" << RGB_TEXT << fitted_content;
        
        int fill = box_width - 2 - GetUtf8Width(fitted_content);
        if (fill < 0) fill = 0;
        for (int f = 0; f < fill; ++f) std::cout << " ";
        std::cout << RGB_HEADER_ACCENT << "│" << RGB_RESET;
        
        std::cout << "\x1b[" << start_y + 2 << ";" << start_x << "H" << RGB_HEADER_BG << RGB_HEADER_ACCENT << "╰" << BuildHorizontalLine(box_width - 2) << "╯" << RGB_RESET;

        int prompt_cursor_x = start_x + 1 + GetUtf8Width(" ❯ ") + GetUtf8Width(ai_current_input);
        if (prompt_cursor_x > start_x + box_width - 2) prompt_cursor_x = start_x + box_width - 2;
        std::cout << "\x1b[" << start_y + 1 << ";" << prompt_cursor_x << "H\x1b[?25h" << std::flush;
    } else {
        int physical_cursor_y = (cur.cursor_y - cur.row_offset) + 3; 
        int physical_cursor_x = visual_cursor_x + 1;
        if (physical_cursor_y < 3) physical_cursor_y = 3;
        if (physical_cursor_y > screen_rows - 1) physical_cursor_y = screen_rows - 1;
        if (physical_cursor_x < 1) physical_cursor_x = 1;
        if (physical_cursor_x > screen_cols) physical_cursor_x = screen_cols;
        std::cout << "\x1b[" << physical_cursor_y << ";" << physical_cursor_x << "H" << "\x1b[?25h" << std::flush;
    }
}

char32_t    ReadKey(void)
{
    char    c = 0;
    char    seq_char = 0;
    char    mouse_data[3];
    std::string seq = "";
    char final_char = 0;
    int max_escape_bytes = 64;
    int escape_bytes_read = 0;
    std::string number_part = "";
    int code = 0;
    size_t semi = 0;
    size_t idx = 0;
    bool all_digits = true;

    if (read(STDIN_FILENO, &c, 1) == -1) exit(1);

    if (c == '\x1b') {
        if (read(STDIN_FILENO, &seq_char, 1) != 1) return '\x1b';

        // ESC O A/B/C/D (alguns terminais em modo aplicação)
        if (seq_char == 'O') {
            if (read(STDIN_FILENO, &seq_char, 1) != 1) return MOUSE_EVENT_IGNORE;
            if (seq_char == 'A') return ARROW_UP;
            if (seq_char == 'B') return ARROW_DOWN;
            if (seq_char == 'C') return ARROW_RIGHT;
            if (seq_char == 'D') return ARROW_LEFT;
            if (seq_char == 'H') return HOME_KEY;
            if (seq_char == 'F') return END_KEY;
            return MOUSE_EVENT_IGNORE;
        }

        if (seq_char != '[') return MOUSE_EVENT_IGNORE;

        if (read(STDIN_FILENO, &seq_char, 1) != 1) return MOUSE_EVENT_IGNORE;

        // Mouse X10: ESC [ M Cb Cx Cy
        if (seq_char == 'M') {
            read(STDIN_FILENO, &mouse_data[0], 1);
            read(STDIN_FILENO, &mouse_data[1], 1);
            read(STDIN_FILENO, &mouse_data[2], 1);
            return MOUSE_EVENT_IGNORE;
        }

        seq = "";
        seq += seq_char;
        while (escape_bytes_read < max_escape_bytes) {
            final_char = seq[seq.length() - 1];
            if ((final_char >= 'A' and final_char <= 'Z') or (final_char >= 'a' and final_char <= 'z') or final_char == '~')
                break;
            if (read(STDIN_FILENO, &seq_char, 1) != 1) break;
            seq += seq_char;
            escape_bytes_read++;
        }

        if (seq.empty()) return MOUSE_EVENT_IGNORE;
        final_char = seq[seq.length() - 1];

        // Mouse SGR: ESC [ <...m ou ESC [ <...M
        if (seq[0] == '<') return MOUSE_EVENT_IGNORE;

        if (final_char == 'A') return ARROW_UP;
        if (final_char == 'B') return ARROW_DOWN;
        if (final_char == 'C') return ARROW_RIGHT;
        if (final_char == 'D') return ARROW_LEFT;
        if (final_char == 'H') return HOME_KEY;
        if (final_char == 'F') return END_KEY;

        if (final_char == '~') {
            number_part = seq.substr(0, seq.length() - 1);
            semi = number_part.find(';');
            if (semi != std::string::npos)
                number_part = number_part.substr(0, semi);
            if (number_part.empty()) return MOUSE_EVENT_IGNORE;
            all_digits = true;
            for (idx = 0; idx < number_part.length(); ++idx) {
                if (number_part[idx] < '0' or number_part[idx] > '9') {
                    all_digits = false;
                    break;
                }
            }
            if (!all_digits) return MOUSE_EVENT_IGNORE;
            code = std::atoi(number_part.c_str());
            if (code == 1 or code == 7) return HOME_KEY;
            if (code == 3) return DEL_KEY;
            if (code == 4 or code == 8) return END_KEY;
            if (code == 5) return PAGE_UP;
            if (code == 6) return PAGE_DOWN;
        }
        return MOUSE_EVENT_IGNORE;
    }
    return (static_cast<unsigned char>(c));
}

void    InsertUtf8Char(const std::string& utf8_char)
{
    FileBuffer& cur = open_files[active_file_idx];
    SaveUndoState(cur);
    std::vector<std::string> chars = SplitUtf8String(cur.lines[cur.cursor_y]);
    chars.insert(chars.begin() + cur.cursor_x, utf8_char);
    cur.lines[cur.cursor_y] = JoinUtf8Chars(chars);
    cur.cursor_x++;
}

void    DeleteChar(void)
{
    FileBuffer& cur = open_files[active_file_idx];
    SaveUndoState(cur);
    std::vector<std::string> chars = SplitUtf8String(cur.lines[cur.cursor_y]);

    if (cur.cursor_x > 0) {
        chars.erase(chars.begin() + cur.cursor_x - 1);
        cur.lines[cur.cursor_y] = JoinUtf8Chars(chars);
        cur.cursor_x--;
    } else if (cur.cursor_y > 0) {
        cur.cursor_x = SplitUtf8String(cur.lines[cur.cursor_y - 1]).size();
        cur.lines[cur.cursor_y - 1] += cur.lines[cur.cursor_y];
        cur.lines.erase(cur.lines.begin() + cur.cursor_y);
        cur.cursor_y--;
    }
}

char32_t    ProcessKeyPress(char32_t c)
{
    if (c == MOUSE_EVENT_IGNORE) return (c);

    // Entrada do prompt de comandos estilo CLI: "/" abre o prompt em qualquer modo
    if (c == '/') {
        current_mode = MODE_AI_PROMPT;
        ai_current_input = "/";
        return (c);
    }

    // MODO PROMPT DA IA
    if (current_mode == MODE_AI_PROMPT) {
        if (c == '\r') {
            ExecuteAIPrompt();
        } else if (c == 127 or c == CTRL_KEY('h')) {
            if (not ai_current_input.empty()) {
                std::vector<std::string> ai_chars = SplitUtf8String(ai_current_input);
                if (!ai_chars.empty()) {
                    ai_chars.pop_back();
                    ai_current_input = JoinUtf8Chars(ai_chars);
                }
            }
        } else if (c == '\x1b') {
            current_mode = MODE_EDITOR;
        } else if (c >= 32 and c <= 255) {
            std::string utf8_payload = "";
            int extra_bytes = 0;
            unsigned char first_byte = static_cast<unsigned char>(c);
            unsigned char cont_byte = 0;
            char next_b = 0;
            bool valid_utf8 = true;

            if (first_byte >= 32 and first_byte <= 126) {
                ai_current_input += static_cast<char>(first_byte);
                return (c);
            }
            if (!IsUtf8LeadByte(first_byte))
                return (MOUSE_EVENT_IGNORE);

            utf8_payload += static_cast<char>(first_byte);
            extra_bytes = GetUtf8CharLength(first_byte) - 1;
            for (int b = 0; b < extra_bytes; ++b) {
                if (read(STDIN_FILENO, &next_b, 1) != 1) {
                    valid_utf8 = false;
                    break;
                }
                cont_byte = static_cast<unsigned char>(next_b);
                if (!IsUtf8ContinuationByte(cont_byte)) {
                    valid_utf8 = false;
                    break;
                }
                utf8_payload += next_b;
            }
            if (!valid_utf8)
                return (MOUSE_EVENT_IGNORE);
            ai_current_input += utf8_payload;
        }
        return (c);
    }

    // MODO GESTOR DE FICHEIROS
    if (current_mode == MODE_FILE_MANAGER) {
        if (c == 'k' or c == ARROW_UP) { if (selected_file_idx > 0) selected_file_idx--; }
        else if (c == 'j' or c == ARROW_DOWN) { if (selected_file_idx < static_cast<int>(file_list.size() - 1)) selected_file_idx++; }
        else if (c == '\r') {
            if (!file_list.empty()) { OpenFileBuffer(file_list[selected_file_idx]); current_mode = MODE_EDITOR; }
        }
        return (c);
    }

    if (c == NEXT_TAB_COMMAND) {
        if (not open_files.empty()) active_file_idx = (active_file_idx + 1) % open_files.size();
        return (c);
    }
    if (c == PREV_TAB_COMMAND) {
        if (not open_files.empty()) active_file_idx = (active_file_idx - 1 + open_files.size()) % open_files.size();
        return (c);
    }

    FileBuffer& cur = open_files[active_file_idx];
    std::vector<std::string> chars = SplitUtf8String(cur.lines[cur.cursor_y]);

    if (c == UNDO_COMMAND) {
        if (!cur.undo_stack.empty()) {
            cur.lines = cur.undo_stack.back();
            cur.undo_stack.pop_back();
            if (cur.cursor_y >= static_cast<int>(cur.lines.size())) cur.cursor_y = cur.lines.size() - 1;
            int max_x = SplitUtf8String(cur.lines[cur.cursor_y]).size();
            if (cur.cursor_x > max_x) cur.cursor_x = max_x;
            if (cur.undo_stack.empty()) cur.is_dirty = false;
        }
        return (c);
    }

    if (c == COPY_COMMAND) {
        clipboard.clear();
        clipboard.push_back(cur.lines[cur.cursor_y]);
        return (c);
    }

    if (c == PASTE_COMMAND) {
        if (!clipboard.empty()) {
            SaveUndoState(cur);
            for (size_t i = 0; i < clipboard.size(); ++i) {
                cur.lines.insert(cur.lines.begin() + cur.cursor_y + 1, clipboard[i]);
                cur.cursor_y++;
            }
            cur.cursor_x = 0;
        }
        return (c);
    }

    if (c == SELECT_ALL) {
        cur.cursor_y = cur.lines.size() - 1;
        cur.cursor_x = SplitUtf8String(cur.lines[cur.cursor_y]).size();
        return (c);
    }

    if ((c == ARROW_LEFT or c == 'h') and cur.cursor_x > 0) cur.cursor_x--;
    else if ((c == ARROW_RIGHT or c == 'l') and cur.cursor_x < static_cast<int>(chars.size())) cur.cursor_x++;
    else if ((c == ARROW_UP or c == 'k') and cur.cursor_y > 0) {
        cur.cursor_y--;
        int max_x = SplitUtf8String(cur.lines[cur.cursor_y]).size();
        if (cur.cursor_x > max_x) cur.cursor_x = max_x;
    }
    else if ((c == ARROW_DOWN or c == 'j') and cur.cursor_y < static_cast<int>(cur.lines.size() - 1)) {
        cur.cursor_y++;
        int max_x = SplitUtf8String(cur.lines[cur.cursor_y]).size();
        if (cur.cursor_x > max_x) cur.cursor_x = max_x;
    }
    else if (c == HOME_KEY) cur.cursor_x = 0;
    else if (c == END_KEY) cur.cursor_x = chars.size();
    else if (c == DEL_KEY) {
        if (cur.cursor_x < static_cast<int>(chars.size())) {
            SaveUndoState(cur);
            chars.erase(chars.begin() + cur.cursor_x);
            cur.lines[cur.cursor_y] = JoinUtf8Chars(chars);
        } else if (cur.cursor_x == static_cast<int>(chars.size()) and cur.cursor_y < static_cast<int>(cur.lines.size() - 1)) {
            SaveUndoState(cur);
            cur.lines[cur.cursor_y] += cur.lines[cur.cursor_y + 1];
            cur.lines.erase(cur.lines.begin() + cur.cursor_y + 1);
        }
    }
    else if (c == '\t') { InsertUtf8Char("\t"); }
    else if (c == '\r') {
        SaveUndoState(cur);
        std::string next_line = JoinUtf8Chars(std::vector<std::string>(chars.begin() + cur.cursor_x, chars.end()));
        cur.lines[cur.cursor_y] = JoinUtf8Chars(std::vector<std::string>(chars.begin(), chars.begin() + cur.cursor_x));
        cur.lines.insert(cur.lines.begin() + cur.cursor_y + 1, next_line);
        cur.cursor_y++;
        cur.cursor_x = 0;
    }
    else if (c == 127) DeleteChar();
    else if (c >= 32) {
        std::string utf8_payload = "";
        unsigned char first_byte = static_cast<unsigned char>(c);
        int extra_bytes = 0;
        unsigned char cont_byte = 0;
        bool valid_utf8 = true;
        
        if (first_byte >= 32 and first_byte <= 126) {
            utf8_payload += static_cast<char>(first_byte);
            InsertUtf8Char(utf8_payload);
            return (c);
        }

        if (!IsUtf8LeadByte(first_byte))
            return (MOUSE_EVENT_IGNORE);

        utf8_payload += static_cast<char>(first_byte);
        extra_bytes = GetUtf8CharLength(first_byte) - 1;
        for (int b = 0; b < extra_bytes; ++b) {
            char next_b = 0;
            if (read(STDIN_FILENO, &next_b, 1) != 1) {
                valid_utf8 = false;
                break;
            }
            cont_byte = static_cast<unsigned char>(next_b);
            if (!IsUtf8ContinuationByte(cont_byte)) {
                valid_utf8 = false;
                break;
            }
            utf8_payload += next_b;
        }
        if (!valid_utf8)
            return (MOUSE_EVENT_IGNORE);
        InsertUtf8Char(utf8_payload);
        
        // Wrap text at 80 characters
        if (c == ' ') {
            std::vector<std::string> cur_chars = SplitUtf8String(cur.lines[cur.cursor_y]);
            if (cur_chars.size() > 80) {
                int break_pos = -1;
                for (int k = 80; k >= 0; --k) {
                    if (k < static_cast<int>(cur_chars.size()) && cur_chars[k] == " ") {
                        break_pos = k;
                        break;
                    }
                }
                if (break_pos != -1) {
                    std::string next_line = JoinUtf8Chars(std::vector<std::string>(cur_chars.begin() + break_pos + 1, cur_chars.end()));
                    cur.lines[cur.cursor_y] = JoinUtf8Chars(std::vector<std::string>(cur_chars.begin(), cur_chars.begin() + break_pos));
                    cur.lines.insert(cur.lines.begin() + cur.cursor_y + 1, next_line);
                    if (cur.cursor_x > break_pos) {
                        cur.cursor_y++;
                        cur.cursor_x -= (break_pos + 1);
                    }
                }
            }
        }
    }

    return (c);
}

int    main(int argc, char **argv)
{
    enableRawMode();
    InitializeEditor(argc, argv);
    
    while (true)
    {
        RefreshScreen();
        char32_t c = ReadKey();
        if (ProcessKeyPress(c) == QUIT_COMMAND or should_quit) break;
    }
    return (0);
}
