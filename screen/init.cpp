#include "headers/init.h"

// initialize curses mode
void init_curses()
{
	// UTF-8
	setlocale(LC_ALL, "");
	setlocale(LC_NUMERIC,"C");

	initscr();
	raw();
	noecho(); // only temporarly enabled when asking for input in header
#ifdef HIGHLIGHT
	start_color();
	init_pair(1, COLOR_RED, COLOR_BLACK);
	init_pair(2, COLOR_GREEN, COLOR_BLACK);
	init_pair(3, COLOR_YELLOW, COLOR_BLACK);
	init_pair(4, COLOR_BLUE, COLOR_BLACK);
	init_pair(5, COLOR_MAGENTA, COLOR_BLACK);
	init_pair(6, COLOR_CYAN, COLOR_BLACK);
	init_pair(7, COLOR_WHITE, COLOR_BLACK);
#endif
}

// initialize text window
void init_text()
{
	text_win = newwin(maxy - 1, maxx - 4, 1, 4);
	scrollok(text_win, TRUE);
	keypad(text_win, TRUE);
	wmove(text_win, 0, 0);
}

// print line numbering (ln_win)
void print_lines()
{
	short i = maxy;
	do
		mvwprintw(ln_win, i - 1, 0, "%3ld", i + ofy);
	while (--i != 0);
}

// initialize line numbering window (ln_win)
void init_lines()
{
	ln_win = newwin(maxy, 4, 1, 0);
	wmove(ln_win, 0, 0);
	wattrset(ln_win, A_DIM);
	scrollok(ln_win, TRUE);
	print_lines();
}

// clear any text on header (header_win)
void clear_header()
{
	wmove(header_win, 0, 0);
	wclrtoeol(header_win);
}

// clear header and print title
 void reset_header()
{
	clear_header();
	mvwprintw(header_win, 0, maxx / 2 - sizeof(name) / 2, "%s", name);
}

void init_header()
{
	// newwin arguments: height, width, start y, start x
	header_win = newwin(1, maxx, 0, 0);
	wbkgdset(header_win, A_STANDOUT);
	reset_header();
}
