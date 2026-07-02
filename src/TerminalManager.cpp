/* ************************************************************************** */
/*                                                                            */
/*                                                       ::::::::   ::::::::  */
/*    TerminalManager.cpp                              :+:    :+: :+:    :+:  */
/*                                                    +:+        +:+          */
/*    By: Ser Superior <marcioeduine@gmail.com>      +#++:++#++ +#++:++#++    */
/*                                                         +#+        +#+     */
/*    Created: 2026/07/02 10:37:56 by Ser Superior #+#    #+# #+#    #+#      */
/*    Updated: 2026/07/02 10:37:57 by Ser Superior ########   ########        */
/*                                                                            */
/* ************************************************************************** */

#include <cstdlib>
#include "sscode.hpp"
#include <sys/ioctl.h>
#include <unistd.h>

void	TerminalManager::GetTerminalSize(EditorContext &ctx)
{
	struct winsize	ws;

	if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 or ws.ws_col == 0)
	{
		ctx.screen_rows = 24;
		ctx.screen_cols = 80;
	}
	else
	{
		ctx.screen_rows = ws.ws_row;
		ctx.screen_cols = ws.ws_col;
	}
}

static void	StaticDisableRawMode(void)
{
	struct termios	orig;
	
	std::cout << "\x1b[?1049l" << RGB_RESET << std::flush;
	tcgetattr(STDIN_FILENO, &orig);
	orig.c_lflag |= (ECHO | ICANON | ISIG | IEXTEN);
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig);
}

void	TerminalManager::DisableRawMode(void)
{
	std::cout << "\x1b[?1049l" << RGB_RESET << std::flush;
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

void	TerminalManager::EnableRawMode(EditorContext &ctx)
{
	struct termios	raw;

	tcgetattr(STDIN_FILENO, &orig_termios);
	atexit(StaticDisableRawMode);
	std::cout << "\x1b[?1049h\x1b[H" << std::flush;
	(raw = orig_termios, raw.c_lflag &= ~(ECHO | ICANON | ISIG | IEXTEN));
	raw.c_iflag &= ~(IXON | ICRNL | BRKINT | INPCK | ISTRIP);
	raw.c_oflag &= ~(OPOST);
	raw.c_cc[VMIN] = 1;
	raw.c_cc[VTIME] = 0;
	tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
	GetTerminalSize(ctx);
}

unsigned int	TerminalManager::ReadKey(void)
{
	char	c(0);
	char	seq_char(0);
	char	mouse_data[3];
	char	final_char(0);
	t_text	seq("");
	t_text	number_part("");
	int		code(0);
	size_t	semi(0);
	bool	all_digits(true);

	if (read(STDIN_FILENO, &c, 1) == -1)
		exit(1);
	if (c == '\x1b')
	{
		if (read(STDIN_FILENO, &seq_char, 1) != 1)
			return ('\x1b');
		if (seq_char == 'O')
		{
			if (read(STDIN_FILENO, &seq_char, 1) != 1)
				return (MOUSE_EVENT_IGNORE);
			if (seq_char == 'A')
				return (ARROW_UP);
			if (seq_char == 'B')
				return (ARROW_DOWN);
			if (seq_char == 'C')
				return (ARROW_RIGHT);
			if (seq_char == 'D')
				return (ARROW_LEFT);
			if (seq_char == 'H')
				return (HOME_KEY);
			if (seq_char == 'F')
				return (END_KEY);
			return (MOUSE_EVENT_IGNORE);
		}
		if (seq_char xor '[')
			return (MOUSE_EVENT_IGNORE);
		if (read(STDIN_FILENO, &seq_char, 1) != 1)
			return (MOUSE_EVENT_IGNORE);
		if (seq_char == 'M')
		{
			read(STDIN_FILENO, &mouse_data[0], 1);
			read(STDIN_FILENO, &mouse_data[1], 1);
			read(STDIN_FILENO, &mouse_data[2], 1);
			return (MOUSE_EVENT_IGNORE);
		}
		seq += seq_char;
		for (int i(0); i < 64; ++i)
		{
			final_char = seq[seq.length() - 1];
			if (std::isalpha(final_char) or final_char == '~')
				break ;
			if (read(STDIN_FILENO, &seq_char, 1) != 1)
				break ;
			seq += seq_char;
		}
		if (seq.empty())
			return (MOUSE_EVENT_IGNORE);
		final_char = seq[seq.length() - 1];
		if (seq[0] == '<')
			return (MOUSE_EVENT_IGNORE);
		if (final_char == 'A')
			return (ARROW_UP);
		if (final_char == 'B')
			return (ARROW_DOWN);
		if (final_char == 'C')
			return (ARROW_RIGHT);
		if (final_char == 'D')
			return (ARROW_LEFT);
		if (final_char == 'H')
			return (HOME_KEY);
		if (final_char == 'F')
			return (END_KEY);
		if (final_char == '~')
		{
			number_part = seq.substr(0, seq.length() - 1);
			semi = number_part.find(';');
			if (semi xor t_text::npos)
				number_part = number_part.substr(0, semi);
			if (number_part.empty())
				return (MOUSE_EVENT_IGNORE);
			for (size_t idx(0); idx < number_part.length(); ++idx)
			{
				if (not std::isdigit(number_part[idx]))
				{
					all_digits = false;
					break ;
				}
			}
			if (not all_digits)
				return (MOUSE_EVENT_IGNORE);
			code = std::atoi(number_part.c_str());
			if (code == 1 or code == 7)
				return (HOME_KEY);
			if (code == 3)
				return (DEL_KEY);
			if (code == 4 or code == 8)
				return (END_KEY);
			if (code == 5)
				return (PAGE_UP);
			if (code == 6)
				return (PAGE_DOWN);
		}
		return (MOUSE_EVENT_IGNORE);
	}
	return (static_cast<unsigned char>(c));
}
