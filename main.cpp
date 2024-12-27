#include "utils/headers/key_func.h"
#include "headers/keybindings.h"

list<gap_buf> text(2);
list<gap_buf>::iterator it;
vector<pair<unsigned, unsigned>> cut;
WINDOW *header_win, *ln_win, *text_win;
wchar_t s[4];
char s2[4], *filename;
unsigned flag, ry, rx, maxy, maxx, curnum;
unsigned short y, x, len;
long ofy;

int main(int argc, char *argv[])
{
	if (argc > 1 && (strcmp(argv[1], "-h") == 0 ||
	strcmp(argv[1], "--help") == 0)) {
		puts(name);
		puts("kri: a simple, compact and fast text editor.\n"
		"Source code: https://github.com/ikozyris/kri\n"
		"Wiki: https://github.com/ikozyris/kri/wiki\n"
		"License: GNU GPL v3\n");
		puts("Usage:\n"
		"       --help, -h          Show this help\n"
		"<file>                     Open file and allow edits\n"
		"-                          Ask for file name on save\n\n"
		"Default keybindings:\n"
		"Save:                      Ctrl-S\n"
		"Exit:                      Ctrl-X\n"
		"Go to start of line:       Ctrl-A\n"
		"Go to end of line:         Ctrl-E\n"
		"Enter built-in terminal:   Alt-C\n"
		"Open other file:           Alt-R\n"
		"Previous/Next word         Shift + Left/Right arrow\n"
		"Show debbuging info:       Alt-I (also command stats in built-in terminal)\n\n"
		"Built-in terminal commands:\n"
		"scroll                     Scroll to line\n"
		"usage                      Show RAM usage and PID of kri\n"
		"run                        Execute a command\n"
		"help                       List commands\n"
		"fixgap                     Benchmark move gap to end and back");
		return 0;
	}
	it = text.begin();

	init_curses();
	getmaxyx(stdscr, maxy, maxx);

	// initialize windows
	init_header();
	init_lines();
	init_text();

	getmaxyx(text_win, maxy, maxx);
	wnoutrefresh(ln_win);
	wnoutrefresh(header_win);
	doupdate();

read:
	if (argc > 1) {
		filename = (char*)malloc(sizeof(char) * 128);
		strcpy(filename, argv[1]);
		FILE *fi = fopen(filename, "r");
#ifdef HIGHLIGHT
		eligible = isc(argv[1]); // syntax highlighting
#endif
		if (fi == NULL) {
			print2header("New file", 1);
			goto loop;
		}
		read_fread(fi);
		fclose(fi);
	}
loop:
	// all functions think there is a newline at EOL, emulate it
	if (at(*it, it->len) != '\n')
		apnd_c(*it, 0);
	print_text(0);
	wmove(text_win, 0, 0);
	it = text.begin();
	while (1) {
		getyx(text_win, y, x);
		ry = y + ofy;
		// if out of bounds: move (to avoid bugs) & TODO: simplify
		if (x > min(it->len - 1 - ofx, maxx))
			wmove(text_win, y, x = min(it->len - ofx - 1, maxx));
		rx = x + ofx;
		mv_curs(*it, rx);

#ifndef RELEASE
		stats();
#endif
#ifdef DEBUG
		print_text(y);
		wmove(text_win, y, x);
#endif
	//goto stop;
		wget_wch(text_win, (wint_t*)s);
		switch (s[0]) {
		case DOWN:
			if (ry >= curnum) // do not scroll indefinetly
				break;
			if (!cut.empty()) // revert cut
				mvprint_line(y, 0, *it, 0, 0);
			cut.clear();
			ofx = 0; // invalidated
			if (y == maxy - 1 && ry < curnum)
				scrolldown();
			else {
				++it;
				ofx = calc_offset_dis(x, *it);
				if (flag < maxx)
					wmove(text_win, y + 1, flag);
				else { // tab cut
					y++;
					x = flag;
					ofx += prevdchar();
				}
			}
			break;

		case UP:
			if (!cut.empty()) // revert cut
				mvprint_line(y, 0, *it, 0, 0);
			if (y == 0 && ofy != 0)
				scrollup();
			else if (y != 0) {
				--it;
				ofx = calc_offset_dis(x, *it);
				if (flag < maxx)
					wmove(text_win, y - 1, flag);
				else { // tab cut
					y--;
					x = flag;
					ofx += prevdchar();
				}
			}
			cut.clear();
			break;

		case LEFT:
			left();
			break;

		case KEY_SLEFT:
			prnxt_word(left);
			break;

		case RIGHT:
			right();
			break;

		case KEY_SRIGHT:
			prnxt_word(right);
			break;

		case BACKSPACE:
			if (x > 0) {
				eras(*it);
				if (it->buffer[it->gps] == '\t') {
					wmove(text_win, y, x - 1);
					ofx += prevdchar();
					x = getcurx(text_win);
					wmove(text_win, y, 0);
					print_line(*it, 0, 0);
					wclrtoeol(text_win);
					wmove(text_win, y, x);
				} else
					mvwdelch(text_win, y, x - 1);
			} else if (!cut.empty()) {
				eras(*it);
				left();
			} else if (y != 0) { // x = 0 && cut.empty(); merge lines
				list<gap_buf>::iterator curln = it;
				--it;
				mv_curs(*it, it->len); // delete \n
				it->gpe = it->cpt - 1;
				unsigned tmp = --(it->len);
				data(*curln, 0, curln->len);
				apnd_s(*it, lnbuf, curln->len); // merge
				text.erase(curln); // delete actual line
				--curnum;
				print_text(y - 1);
				wmove(text_win, y - 1, tmp);
			}
			break;

		case DELETE:
			if (it->buffer[it->gpe + 1u] == '\n') { // similar to backspace
				list<gap_buf>::iterator curln = it; // current line
				curln->gpe = curln->cpt - 1; // delete newline
				curln->len--;
				++it; // next line
				data(*it, 0, it->len);
				apnd_s(*curln, lnbuf, it->len);
				text.erase(it);
				it = curln;
				--curnum;
				print_text(y);
				wmove(text_win, y, x);
			} else if (rx + 1 < it->len) {
				// or mblen(it->buffer + it->gpe + 1, 3);
				unsigned len = it->buffer[it->gpe + 1] < 0 ? 2 : 1;
				mveras(*it, rx + len);
				ofx += len - 1;
				wclrtoeol(text_win);
				mvprint_line(y, x, *it, rx, 0);
				wmove(text_win, y, x);
			}
			break;

		case DELLINE:
			if (curnum > 0 && text.size() > 2) {
				curnum--;
				list<gap_buf>::iterator curln = it;
				++it;
				text.erase(curln);
				print_text(y);
				wmove(text_win, y, 0);
			} else { // clear line buffer
				it->gps = 0; it->gpe = it->cpt - 2;
				it->len = 1; it->buffer[it->cpt - 1] = 0;
				clearline;
			}
			break;

		case ENTER:
			highlight(y);
			enter();
			break;

		case HOME:
			sol();
			break;

		case END:
			eol();
			break;

		case SAVE:
			save();
			s2[0] = 0; // no new char has been inserted since last save
			argc = 3;
			if (text.size() % 2 == 1)
				text.resize(text.size() + 1);
			break;

		case 27: { // ALT or ESC
			wtimeout(text_win, 1000);
			int ch = wgetch(text_win);
			wtimeout(text_win, -1);
			if (ch == INFO)
				stats();
			else if (ch == CMD)
				command();
			else if (ch == SWITCH) { // switch file
				argc = 2;
				argv[1] = input_header("File to open: ");
				list<gap_buf>::iterator iter;
				unsigned i;
				for (iter = text.begin(), i = 0; iter != text.end() && i <= curnum; ++iter, ++i) {
					iter->len = iter->gps = 0;
					iter->gpe = iter->cpt;
				}
				curnum = 0;
				it = text.begin();
				wclear(text_win);
				goto read;
			}
			wmove(text_win, y, x);
			break;
		}

		case REFRESH:
			reset_view();
			break;

		case KEY_RESIZE:
			endwin();
			refresh();
			getmaxyx(stdscr, maxy, maxx);
			delwin(text_win);
			init_text();
			getmaxyx(text_win, maxy, maxx);
			reset_view();
			break;

		case EXIT:
			// has char been inserted, new file, allocations are multiples of 2
			if (s2[0] != 0 || argc < 2 || text.size() % 2 == 1) {
				char *in = input_header("Unsaved changes, exit? (y/n) ");
				flag = in[0]; // tmp var to free branchlessly
				free(in);
				if (flag != 'y') {
					reset_header();
					wmove(text_win, y, x);
					break;
				}
			}
			goto stop;

		case KEY_TAB:
			winsnstr(text_win, "        ", 8 - x % 8);
			wmove(text_win, y, x + 8 - x % 8);
			insert_c(*it, rx, '\t');
			ofx -= 7 - x % 8;
			break;

		default:
			if (s[0] > 0 && s[0] < 32) // not a character
				break;
			if (x == maxx - 1) { // cut line
				cut.push_back({maxx - 1, ofx});
				clearline;
				ofx += maxx - 1;
				print_line(*it, ofx, 0);
				wmove(text_win, y, x = 0);
				wrefresh(text_win);
				rx = x + ofx;
			} if (it->buffer[it->gpe + 1] == '\t') { // next character is newline
				waddnwstr(text_win, s, 1);
				if (x % 8 >= 7)
					winsch(text_win, '\t');
			} else {
				wins_nwstr(text_win, s, 1);
				wmove(text_win, y, x + 1);
			}
			len = wcstombs(s2, s, 4);
			insert_s(*it, rx, s2, len);
			if (len > 1)
				ofx += len - 1; // UTF-8 character
			break;
		}
	}
stop:
	free(lnbuf);
	delwin(text_win);
	delwin(ln_win);
	delwin(header_win);
	endwin();
	return 0;
}
