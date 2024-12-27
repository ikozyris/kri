#include "headers/key_func.h"

// display stats on header
void stats()
{
	char *_tmp = (char*)malloc(256);
	unsigned sumlen = 0;
	for (auto &i : text)
		sumlen += i.len;
#ifndef RELEASE
	unsigned cutd = 0, cutb = 0;
	if (!cut.empty()) {
		cutb = cut.back().byte;
		cutd = cut.back().dchar;
	}
	snprintf(_tmp, min(maxx, 256), "maxx %u len %u gs %u ge %u cpt %u cut[d%u,b%u] x: %u ofx: %ld ry: %u     ",
		maxx, it->len, it->gps, it->gpe, it->cpt, cutd, cutb, x, ofx, ry);
#else	
	snprintf(_tmp, min(maxx, 256), "len %u  cpt %u  y %u  x %u  sum len %u  lines %u  cut %lu  ofx %ld  ", 
		it->len, it->cpt, ry, x, sumlen, curnum, cut.size(), ofx);
#endif
	print2header(_tmp, 1);
	free(_tmp);
	wmove(text_win, y, x);
}

// prompt for command
void command()
{
	char *tmp = input_header("Enter command: ");
	if (strcmp(tmp, "resetheader") == 0)
		reset_header();
	else if (strcmp(tmp, "shrink") == 0) {
		size_t prev = memusg(); // RAM usage before

		// shrink line buffer
		lnbf_cpt = 16;
		lnbuf = (char*)realloc(lnbuf, lnbf_cpt);
                // shrink linked list
                text.resize(curnum + 1);
		//shrink each line
		size_t bytes_freed = 0;
		for (auto &i : text)
			bytes_freed += shrink(i);
		char *bytes_saved_str = (char*)malloc(24);
		hrsize(bytes_freed, bytes_saved_str, 24);

		size_t curr = memusg(); // RAM usage after
		char *prev_curr_str = (char*)malloc(24);
		hrsize((prev - curr) * 1000, prev_curr_str, 24);

		char buffer[64] = "";
		snprintf(buffer, 64, "freed: %s | kernel reclaimed %s", bytes_saved_str, prev_curr_str);
		free(bytes_saved_str);
		free(prev_curr_str);
		clear_header();
		print2header(buffer, 1);
	} else if (strcmp(tmp, "usage") == 0) {
		size_t pid = 0;

		// Linux-only
		FILE *file = fopen("/proc/self/status", "r");
		while (fscanf(file, " %127s", tmp) == 1)
			if (strcmp(tmp, "Pid:") == 0) {
				fscanf(file, " %lu", &pid);
				break;
			}
		fclose(file);
		char ram_usg[24];
		hrsize(memusg() * 1000, ram_usg, 24);
		sprintf(tmp, "RAM: %s PID: %lu", ram_usg, pid);
		clear_header();
		print2header(tmp, 1);
	} else if (strcmp(tmp, "stats") == 0)
		stats();
	else if (strncmp(tmp, "run", 3) == 0) {
		clear();
		refresh();
		int res = system(tmp + 4);
		printw("\nreturned: %d", res);

		getch();
		reset_view();
	} else if (strcmp(tmp, "help")  == 0)
		print2header("resetheader, shrink, usage, stats, run [cmd], scroll [line], find [token]", 1);
	else if (strncmp(tmp, "scroll", 6) == 0) {
		unsigned a;
		sscanf(tmp + 7, "%u", &a);
		if (a <= curnum) {
			ofy = a - 1;
			print_lines();
			wrefresh(ln_win);
			print_text(0);
			advance(it, ofy - ry);
		}
	} else if (strncmp(tmp, "find", 4) == 0) { // example: find_all_shw abc
		unsigned char mode = 0;
		if (strncmp(tmp + 5, "all", 3) == 0)
			mode = 2;
		if (strncmp(tmp + 9, "shw", 3) == 0)
			mode += 1;

		if (tmp[4] == ' ')
			find((const char*)tmp + 5, 3);
		else
			find((const char*)tmp + 13, mode);
	} else
		print2header("command not found", 3);
	free(tmp);
}

// insert enter in rx of buffer, create new line node and reprint
void enter()
{
	insert_c(*it, rx, '\n');
	gap_buf *t = (gap_buf*)malloc(sizeof(gap_buf));
	init(*t);
	/* If buffer's last char is a newline; rx = it->len - 1; gpe = cpt - 2
	 * so the last character (newline or emulated) will be copied over
	 * Otherwise, (x != EOL) copy the remaining bytes
	 */
	data(*it, rx + 1, it->len + 1);
	apnd_s(*t, lnbuf, it->len - rx - 1);
	it->len = it->gps = rx + 1;
	it->gpe = it->cpt - 1;

	++it; // insert is on previous than current it
	++curnum;
	text.insert(it, *t); // insert new node with text after rx
	--it;
	free(t);
	print_text(y);
	if (y < maxy - 1)
		wmove(text_win, y + 1, 0);
	else { // y = maxy; scroll
		wscrl(text_win, 1);
		++ofy;
		mvprint_line(maxy - 1, 0, *it, 0, 0);
		wmove(text_win, maxy - 1, x);
	}
	ofx = 0;
}

// go to end-of-line, if necessary cut line
void eol()
{
	ofx = calc_offset_act(it->len, 0, *it);
	if (it->len - ofx <= maxx) // line fits in screen
		wmove(text_win, y, it->len - ofx - 1);
	else { // cut line
		cut.clear();
		unsigned bytes = 0, nbytes = 0;
		while (bytes < it->len) {
			nbytes = dchar2bytes(maxx - 1, bytes, *it);
			if (nbytes >= it->len - 1)
				break;
			cut.push_back({flag, nbytes}); // flag was changed by dchar2bytes
			ofx += flag;
			bytes = nbytes;
		}
		clearline;
		print_line(*it, bytes, it->len);
		x = getcurx(text_win);
		if (x + ofx > it->len - 1) 
			ofx -= x + ofx - it->len + 1;
	}
}

// go to start-of-line, uncut line if needed
void sol()
{
	if (!cut.empty()) { // line has been cut
		clearline;
		print_line(*it, 0, 0);
		highlight(y);
	}
	cut.clear();
	wmove(text_win, y, ofx = 0);
}

// scroll screen down, print last line
void scrolldown()
{
	++it;
	++ofy;
	cut.clear();
	ofx = 0;
	wscrl(text_win, 1);
	wscrl(ln_win, 1);
	mvwprintw(ln_win, maxy - 1, 0, "%3u", ry + 2);
	wrefresh(ln_win);
	mvprint_line(y, 0, *it, 0, 0);
	highlight(y);
	wmove(text_win, y, 0);
}

// scroll screen up, print first line
void scrollup()
{
	--ofy;
	--it;
	cut.clear();
	ofx = 0;
	wscrl(text_win, -1);
	wscrl(ln_win, -1);
	mvwprintw(ln_win, 0, 0, "%3u", ry);
	wrefresh(ln_win);
	mvprint_line(0, 0, *it, 0, 0);
	highlight(0);
	wmove(text_win, 0, 0);
}

// left arrow
unsigned short left()
{
	if (x == 0 && ofx == 0 && ofy > 0 && y == 0) {
		scrollup();
		eol();
		return SCROLL;
	} else if (x == 0 && !cut.empty()) { // line has been cut
		clearline;
		ofx -= cut.back().dchar;
		cut.pop_back();
		print_line(*it, cut.empty() ? 0 : cut.back().byte, 0);
		highlight(y);
		wmove(text_win, y, flag + 1);
		return CUT;
	} else if (x > 0) { // go left
		wmove(text_win, y, x - 1);
		// handle special characters causing offsets
		if (it->buffer[it->gps - 1] == '\t')
			ofx += prevdchar();
		else if (it->buffer[it->gps - 1] < 0)
			--ofx;
		return NORMAL;
	} else if (y > 0) { // x = 0
		--it;
		--y;
		eol();
		return LN_CHANGE;
	}
	return NOTHING;
}

// right arrow
unsigned short right() {
	if (rx >= it->len - 1 && ry < curnum) { // go to next line
		if (y == maxy - 1) {
			scrolldown();
			return SCROLL;
		} else if (!cut.empty()) // revert cut
			mvprint_line(y, 0, *it, 0, 0);
		wmove(text_win, y + 1, 0);
		++it;
		cut.clear();
		ofx = 0;
		return LN_CHANGE;
	} else if (x == maxx - 1) { // right to cut part of line
cut_line:
		clearline;
		ofx += x;
		cut.push_back({x, (cut.empty() ? 0 : cut.back().byte) + print_line(*it, ofx, 0)});
		wmove(text_win, y, 0);
		return CUT;
	} else { // go right
		wmove(text_win, y, x + 1);
		if (it->buffer[it->gpe + 1] == '\t') {
			if (x >= maxx - 7)
				goto cut_line;
			ofx -= 8 - x % 8 - 1;
			wmove(text_win, y, x + 8 - x % 8);
		} else if (it->buffer[it->gpe + 1] < 0)
			++ofx;
		return NORMAL;
	}
	return NOTHING;
}

// go to end/start of previous word (call like (prnxt_word(left)))
void prnxt_word(unsigned short func(void))
{
	unsigned short status;
	do {
		status = func();
		x = getcurx(text_win);
		mv_curs(*it, x + ofx);
	} while ((winch(text_win) & A_CHARTEXT) != ' ' && status == NORMAL);
}

void reset_view()
{
	ofy = ofx = 0;
	cut.clear();
	it = text.begin();
	print_text(0);
	reset_header();
	print_lines();
	wmove(text_win, 0, 0);
	wnoutrefresh(ln_win);
	wnoutrefresh(header_win);
	doupdate();
}
