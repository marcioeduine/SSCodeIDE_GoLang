/* ************************************************************************** */
/* */
/* :::      ::::::::   */
/* sscode.cpp                                         :+:      :+:    :+:   */
/* +:+ +:+         +:+     */
/* By: Ser Superior <marcioeduine@gmail.com>       +#+  +:+       +#+        */
/* +#+#+#+#+#+   +#+           */
/* Created: 2026/07/02 01:38:25 by Ser Superior          #+#    #+#             */
/* Updated: 2026/07/02 07:10:00 by Ser Superior         ###   ########.fr       */
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
    MODE_FILE_MANAGER
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
    DEL_KEY
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

// Estado Global do Sistema
struct termios          orig_termios;
std::vector<FileBuffer> open_files;
int                     active_file_idx = 0;
e_editorMode            current_mode = MODE_EDITOR;
bool                    drawer_open = false;
int                     screen_rows = 0;
int                     screen_cols = 0;

std::vector<std::string> clipboard;
std::vector<std::string> file_list;
int                      selected_file_idx = 0;

const char* keywords[] = { "switch", "if", "else", "while", "for", "break", "return", "define", "include", "and", "or", "not", "xor" };
const char* datatypes[] = { "int", "char", "void", "struct", "class", "bool", "char32_t", "size_t" };

// --- CONFIGURAÇÃO DA PALETA TRUE COLOR (RGB 24-BITS) ---
#define RGB_BG_DARK   "\x1b[48;2;30;30;46m"
#define RGB_TAB_BG    "\x1b[48;2;24;24;37m"
#define RGB_TAB_ACT   "\x1b[48;2;137;180;250;38;2;17;17;27;1m"
#define RGB_TEXT      "\x1b[38;2;205;214;244m"
#define RGB_KEYWORD   "\x1b[38;2;137;220;235m"
#define RGB_DATATYPE  "\x1b[38;2;166;227;161m"
#define RGB_STRING    "\x1b[38;2;249;226;175m"
#define RGB_COMMENT   "\x1b[38;2;108;112;134m"
#define RGB_PREPROC   "\x1b[38;2;245;194;231m"
#define RGB_NUMBER    "\x1b[38;2;2fab;242m"
#define RGB_GUTTER    "\x1b[38;2;88;91;112m"
#define RGB_RESET     "\x1b[0m"

// --- FUNÇÕES DE SUPORTE MANIPULAÇÃO UTF-8 ---

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

std::string BuildHorizontalLine(int width) {
    std::string line = "";
    for (int i = 0; i < width; ++i) {
        line += "─"; // Aqui passa como string literal literal, o que é perfeitamente válido
    }
    return line;
}

std::string JoinUtf8Chars(const std::vector<std::string>& chars) {
    std::string out = "";
    for (size_t i = 0; i < chars.size(); ++i) out += chars[i];
    return out;
}

// --- ENGINE DE GESTÃO DO TERMINAL ---

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
    if (filename.find(" ") == 0 or filename.find("󰈚 ") == 0) clean_name = filename.substr(4);
    if (!clean_name.empty() and clean_name[clean_name.length() - 1] == '/')
        clean_name = clean_name.substr(0, clean_name.length() - 1);

    for (size_t i = 0; i < open_files.size(); ++i) {
        if (open_files[i].name == clean_name) {
            active_file_idx = static_cast<int>(i);
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
    int edit_window_height = screen_rows - 3;
    if (edit_window_height < 1) edit_window_height = 1;

    if (cur.cursor_y < cur.row_offset) cur.row_offset = cur.cursor_y;
    if (cur.cursor_y >= cur.row_offset + edit_window_height) cur.row_offset = cur.cursor_y - edit_window_height + 1;
}

// --- LEXER / PARSER DE SINTAXE REALISTA ---
void RenderHighlightedLine(const std::string& line)
{
    std::vector<std::string> chars = SplitUtf8String(line);
    std::string current_word = "";
    bool in_string = false;

    if (!chars.empty() and chars[0] == "#") {
        std::cout << RGB_PREPROC << line << RGB_RESET;
        return;
    }

    for (size_t j = 0; j < chars.size(); ++j) {
        std::string c = chars[j];

        if (!in_string and j + 1 < chars.size() and c == "/" and chars[j + 1] == "/") {
            std::cout << RGB_COMMENT;
            for (size_t rem = j; rem < chars.size(); ++rem) std::cout << chars[rem];
            std::cout << RGB_RESET;
            return;
        }

        if (c == "\"") {
            if (in_string) {
                current_word += c;
                std::cout << RGB_STRING << current_word << RGB_RESET;
                current_word = "";
                in_string = false;
            } else {
                std::cout << RGB_TEXT << current_word << RGB_RESET;
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

            if (c == "\t") std::cout << "    ";
            else std::cout << RGB_TEXT << c << RGB_RESET;
            current_word = "";
        } else {
            current_word += c;
        }
    }
    if (is_in_list(current_word, keywords, 13)) std::cout << RGB_KEYWORD << current_word << RGB_RESET;
    else if (is_in_list(current_word, datatypes, 8)) std::cout << RGB_DATATYPE << current_word << RGB_RESET;
    else std::cout << RGB_TEXT << current_word << RGB_RESET;
}

// --- PIPELINE DE RENDERIZAÇÃO DE JANELA (VIEWPORT FIXA) ---
void RefreshScreen(void)
{
    GetTerminalSize();
    ScrollEditor();

    std::cout << "\x1b[?25l\x1b[H" << RGB_BG_DARK;

    if (current_mode == MODE_FILE_MANAGER) {
        std::cout << "\x1b[38;2;137;180;250;1m  GESTOR DE FICHEIROS\x1b[0m (j/k: navegar, Enter: abrir)\x1b[K\r\n";
        std::cout << RGB_GUTTER << "────────────────────────────────────────────────────────────" << RGB_RESET << "\x1b[K\r\n";
        for (int i = 0; i < screen_rows - 3; ++i) {
            if (i < static_cast<int>(file_list.size())) {
                if (i == selected_file_idx) std::cout << " \x1b[38;2;166;227;161m➔ " << file_list[i] << RGB_RESET << "\x1b[K\r\n";
                else std::cout << "   " << file_list[i] << "\x1b[K\r\n";
            } else std::cout << "\x1b[K\r\n";
        }
        std::cout << "\x1b[?25h" << std::flush;
        return;
    }

    // 1. HEADER / TABLINE SUPERIOR
    std::cout << RGB_TAB_BG << " ";
    for (size_t i = 0; i < open_files.size(); ++i) {
        std::string dirty_flag = open_files[i].is_dirty ? " \x1b[31m[+]\x1b[0m" : "";
        std::string tab_name = open_files[i].name.empty() ? "*Sem Nome*" : open_files[i].name;
        
        if (static_cast<int>(i) == active_file_idx) {
            std::cout << RGB_TAB_ACT << " " << tab_name << dirty_flag << " " << RGB_TAB_BG;
        } else {
            std::cout << RGB_GUTTER << "│ " << RGB_TEXT << tab_name << dirty_flag << " ";
        }
    }
    std::cout << "\x1b[K" << RGB_RESET << "\r\n";

    FileBuffer& cur = open_files[active_file_idx];
    int visual_cursor_x = 0;
    int drawer_width = drawer_open ? 24 : 0;
    int line_number_width = 6;
    int edit_window_height = screen_rows - 3;

    // 2. CONTEÚDO PRINCIPAL (VIEWPORT)
    for (int i = 0; i < edit_window_height; ++i)
    {
        int file_line_idx = i + cur.row_offset;

        if (file_line_idx >= static_cast<int>(cur.lines.size())) {
            if (drawer_open) std::cout << "                  " << RGB_GUTTER << "│" << RGB_RESET << " ";
            std::cout << RGB_GUTTER << "~\x1b[K" << RGB_RESET << "\r\n";
            continue;
        }

        if (drawer_open) {
            if (i < static_cast<int>(open_files.size())) {
                std::string name = open_files[i].name.empty() ? "*Sem Nome*" : open_files[i].name;
                if (name.length() > 16) name = name.substr(0, 14) + "..";
                if (i == active_file_idx) std::cout << "\x1b[38;2;166;227;161m " << name << RGB_RESET;
                else std::cout << "  " << name;
                int spaces = 18 - name.length();
                for (int s = 0; s < spaces; ++s) std::cout << " ";
                std::cout << RGB_GUTTER << "│" << RGB_RESET << " ";
            } else {
                std::cout << "                  " << RGB_GUTTER << "│" << RGB_RESET << " ";
            }
        }

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

        RenderHighlightedLine(cur.lines[file_line_idx]);
        std::cout << "\x1b[K\r\n";
    }

    // 3. FOOTER / STATUSLINE ESTILO LAZYVIM
    std::cout << RGB_GUTTER << BuildHorizontalLine(screen_cols) << RGB_RESET << "\r\n";
    std::cout << "\x1b[48;2;49;50;68m\x1b[38;2;17;17;27;1m NORMAL " << RGB_TAB_BG << " \x1b[38;2;205;214;244m" 
              << (cur.name.empty() ? "[Sem Nome]" : cur.name) << (cur.is_dirty ? " \x1b[31m[+]" : "")
              << " \x1b[38;2;108;112;134m│\x1b[38;2;137;220;235m Ln " << (cur.cursor_y + 1) << ", Col " << (cur.cursor_x + 1)
              << " \x1b[38;2;108;112;134m│\x1b[38;2;249;226;175m UTF-8 " << RGB_RESET << "\x1b[K";

    int physical_cursor_y = (cur.cursor_y - cur.row_offset) + 2; 
    std::cout << "\x1b[" << physical_cursor_y << ";" << (visual_cursor_x + 1) << "H" << "\x1b[?25h" << std::flush;
}

char32_t    ReadKey(void)
{
    char    c = 0;
    if (read(STDIN_FILENO, &c, 1) == -1) exit(1);

    if (c == '\x1b') {
        char seq[4];
        if (read(STDIN_FILENO, &seq[0], 1) != 1) return '\x1b';
        if (read(STDIN_FILENO, &seq[1], 1) != 1) return '\x1b';

        if (seq[0] == '[') {
            if (seq[1] >= '0' and seq[1] <= '9') {
                if (read(STDIN_FILENO, &seq[2], 1) != 1) return '\x1b';
                if (seq[2] == '~') {
                    switch (seq[1]) {
                        case '1': return HOME_KEY;
                        case '3': return DEL_KEY;
                        case '4': return END_KEY;
                        case '5': return PAGE_UP;
                        case '6': return PAGE_DOWN;
                    }
                }
            } else {
                switch (seq[1]) {
                    case 'A': return ARROW_UP;
                    case 'B': return ARROW_DOWN;
                    case 'C': return ARROW_RIGHT;
                    case 'D': return ARROW_LEFT;
                    case 'H': return HOME_KEY;
                    case 'F': return END_KEY;
                }
            }
        }
        return '\x1b';
    }
    return (c);
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

// --- INTEGRAÇÃO DO MODELO DE IA ASSISTENTE ---
void    InvokeAIAssistant(void)
{
    FileBuffer& cur = open_files[active_file_idx];
    if (cur.lines.empty() or cur.cursor_y >= static_cast<int>(cur.lines.size())) return;

    std::string prompt = cur.lines[cur.cursor_y];
    if (prompt.empty() or prompt.find_first_not_of(" \t\r\n") == std::string::npos) return;

    std::string escaped_prompt = "";
    for (size_t i = 0; i < prompt.length(); ++i) {
        if (prompt[i] == '\'') escaped_prompt += "'\\''";
        else escaped_prompt += prompt[i];
    }

    std::string cmd = "./ss_ai_bridge.py '" + escaped_prompt + "'";
    FILE* pipe = popen(cmd.c_str(), "r");
    if (not pipe) return;

    SaveUndoState(cur);

    char buffer[512];
    int insert_y = cur.cursor_y + 1;
    std::string ai_line = "";

    while (fgets(buffer, sizeof(buffer), pipe) != NULL) {
        std::string chunk(buffer);
        for (size_t i = 0; i < chunk.length(); ++i) {
            if (chunk[i] == '\n' or chunk[i] == '\r') {
                cur.lines.insert(cur.lines.begin() + insert_y, ai_line);
                insert_y++;
                ai_line = "";
            } else {
                ai_line += chunk[i];
            }
        }
    }
    if (not ai_line.empty()) cur.lines.insert(cur.lines.begin() + insert_y, ai_line);

    pclose(pipe);
    cur.is_dirty = true;
    cur.cursor_y = insert_y - 1;
    cur.cursor_x = 0;
}

char32_t    ProcessKeyPress(char32_t c)
{
    if (c == QUIT_COMMAND) return (QUIT_COMMAND);

    if (c == NEXT_TAB_COMMAND and current_mode == MODE_EDITOR) {
        if (not open_files.empty()) active_file_idx = (active_file_idx + 1) % open_files.size();
        return (c);
    }
    if (c == PREV_TAB_COMMAND and current_mode == MODE_EDITOR) {
        if (not open_files.empty()) active_file_idx = (active_file_idx - 1 + open_files.size()) % open_files.size();
        return (c);
    }

    if (c == AI_COMMAND and current_mode == MODE_EDITOR) {
        InvokeAIAssistant();
        return (c);
    }

    if (c == TOGGLE_MODE) {
        if (current_mode == MODE_EDITOR) { LoadDirectoryFiles(); current_mode = MODE_FILE_MANAGER; }
        else current_mode = MODE_EDITOR;
        return (c);
    }

    if (c == TOGGLE_DRAWER) {
        drawer_open = !drawer_open;
        return (c);
    }

    if (current_mode == MODE_FILE_MANAGER) {
        if (c == 'k' or c == ARROW_UP) { if (selected_file_idx > 0) selected_file_idx--; }
        else if (c == 'j' or c == ARROW_DOWN) { if (selected_file_idx < static_cast<int>(file_list.size() - 1)) selected_file_idx++; }
        else if (c == '\r') {
            if (!file_list.empty()) { OpenFileBuffer(file_list[selected_file_idx]); current_mode = MODE_EDITOR; }
        }
        return (c);
    }

    FileBuffer& cur = open_files[active_file_idx];
    std::vector<std::string> chars = SplitUtf8String(cur.lines[cur.cursor_y]);

    if (c == SAVE_COMMAND) { SaveFile(); return (c); }

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
        utf8_payload += static_cast<char>(c);
        int extra_bytes = GetUtf8CharLength(static_cast<unsigned char>(c)) - 1;
        
        for (int b = 0; b < extra_bytes; ++b) {
            char next_b = 0;
            if (read(STDIN_FILENO, &next_b, 1) == 1) utf8_payload += next_b;
        }
        InsertUtf8Char(utf8_payload);
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
        if (ProcessKeyPress(c) == QUIT_COMMAND) break;
    }
    return (0);
}
