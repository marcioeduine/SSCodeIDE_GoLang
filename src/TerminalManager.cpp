/* ************************************************************************** */
/*                                                                            */
/*                                                       ::::::::   ::::::::  */
/*    TerminalManager.cpp                              :+:    :+: :+:    :+:  */
/*                                                    +:+        +:+          */
/*    By: Ser Superior <marcioeduine@gmail.com>      +#++:++#++ +#++:++#++    */
/*                                                         +#+        +#+     */
/*    Created: 2026/07/02 10:37:56 by Ser Superior #+#    #+# #+#    #+#      */
/*    Updated: 2026/09/09 15:10:00 by Ser Superior ########   ########        */
/*                                                                            */
/* ************************************************************************** */

#include "sscode.hpp"
#include <cstdlib>
#include <chrono>

static bool	g_ncurses_initialized = false;

static void	StaticCleanup(void)
{
	if (g_ncurses_initialized)
	{
		printf("\033[?1000l\033[?1002l\033[?1006l");
		fflush(stdout);
		endwin();
		g_ncurses_initialized = false;
	}
}

void	TerminalManager::GetTerminalSize(EditorContext &ctx)
{
	getmaxyx(stdscr, ctx.screen_rows, ctx.screen_cols);
	if (ctx.screen_rows <= 0) ctx.screen_rows = 24;
	if (ctx.screen_cols <= 0) ctx.screen_cols = 80;
}

void	TerminalManager::Init(EditorContext &ctx)
{
	setlocale(LC_ALL, "");
	initscr();
	raw();
	noecho();
	keypad(stdscr, TRUE);
	set_escdelay(25);
	curs_set(1);
	mousemask(0, NULL);
	printf("\033[?1000l\033[?1002l\033[?1003l\033[?1006l");
	fflush(stdout);

	g_ncurses_initialized = true;
	atexit(StaticCleanup);

	GetTerminalSize(ctx);
}

void	TerminalManager::Shutdown(void)
{
	StaticCleanup();
}

int		TerminalManager::ReadKey(EditorContext &ctx)
{
	wint_t		wch = 0;
	int			ret = get_wch(&wch);

	if (ret == KEY_CODE_YES)
	{
		if (wch == KEY_MOUSE)
		{
			MEVENT	event;
			if (getmouse(&event) == OK)
			{
				if (event.bstate & BUTTON4_PRESSED)
					return (KEY_MOUSE_SCROLL_UP);
				if (event.bstate & BUTTON5_PRESSED)
					return (KEY_MOUSE_SCROLL_DOWN);
				if (event.bstate & (BUTTON1_CLICKED | BUTTON1_PRESSED | BUTTON1_RELEASED | BUTTON1_DOUBLE_CLICKED))
				{
					static int last_cx = -1;
					static int last_cy = -1;
					static std::chrono::steady_clock::time_point last_click_tp;

					auto now = std::chrono::steady_clock::now();
					auto diff_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_click_tp).count();

					if (event.x == last_cx and event.y == last_cy and diff_ms < 150)
						return (KEY_MOUSE_IGNORE);

					last_cx = event.x;
					last_cy = event.y;
					last_click_tp = now;

					ctx.mouse_x = event.x;
					ctx.mouse_y = event.y;
					ctx.mouse_bstate = event.bstate;
					return (KEY_MOUSE_CLICK);
				}
			}
			return (KEY_MOUSE_IGNORE);
		}
		if (wch == KEY_BACKSPACE)
			return (127);
		return (static_cast<int>(wch));
	}
	else if (ret == OK)
	{
		if (wch == 127 or wch == 8)
			return (127);
		if (wch == '\n' or wch == '\r')
			return ('\r');
		return (static_cast<int>(wch));
	}
	return (0);
}
