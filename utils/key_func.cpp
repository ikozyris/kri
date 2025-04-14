#include "headers/key_func.h"

// display stats on header
void stats()
{
	char *_tmp = (char*)malloc(256);
	ulong sumlen = 0;
	for (auto &i : text)
		sumlen += i.len();
#ifndef RELEASE
	uint cutd = 0, cutb = 0;
	if (!cut.empty()) {
		cutb = cut.back().byte;
		cutd = cut.back().dchar;
	}
	snprintf(_tmp, min(maxx, 256), "maxx %u len %lu gs %lu ge %lu cpt %lu cut%lu[d%u,b%u] x: %u ofx: %ld ry: %lu     ",
		maxx, it->len(), it->gps(), it->gpe(), it->cpt(), cut.size(), cutd, cutb, x, ofx, ry);
#else	
	snprintf(_tmp, min(maxx, 256), "len %lu  cpt %lu  y %lu  x %u  sum len %lu  lines %lu  cut %lu  ofx %ld  ", 
		it->len(), it->cpt(), ry, x, sumlen, curnum, cut.size(), ofx);
#endif
	print2header(_tmp, 1);
	free(_tmp);
	wmove(text_win, y, x);
}

// choose command
void command()
{
	char *tmp = input_header("Enter command: ");
	if (strcmp(tmp, "resetheader") == 0)
		reset_header();
	else if (strcmp(tmp, "shrink") == 0) {
		char buffer[64] = "";
		snprintf(buffer, 64, "freed: %lu B", lnbf_cpt - 16 +
			sizeof(list<gap_buf>) * (text.size() - curnum));
		clear_header();
		print2header(buffer, 1);

		// shrink line buffer
		lnbf_cpt = 16;
		lnbuf = (char*)realloc(lnbuf, lnbf_cpt);
                // shrink linked list
                text.resize(curnum + 1);
	} else if (strcmp(tmp, "stats") == 0)
		stats();
	else if (strcmp(tmp, "suspend") == 0) {
		endwin();
		pause();
		reset_view();
	} else if (strcmp(tmp, "help")  == 0)
		print2header("resetheader, shrink, stats, suspend, scroll, find, replace", 1);
	else if (strncmp(tmp, "scroll", 6) == 0) {
		uint a;
		sscanf(tmp + 7, "%u", &a);
		if (a <= curnum) {
			ofy = a - 1;
			print_lines();
			wrefresh(ln_win);
			print_text(0);
			advance(it, ofy - ry);
		}
	} else if (strncmp(tmp, "find", 4) == 0) { // example: find string
		uint from = 0, to = curnum;
		char mode = 'h';
		char *pr2 = input_header("range/mode: "); // 5-10 h
		sscanf(pr2, "%u-%u %c", &from, &to, &mode);
		free(pr2);
		find(tmp + 5, from, to + 1, mode); // we want closed interval, function is open
	} else if (strncmp(tmp, "replace", 7) == 0) {
		uint from = 0, to = curnum;
		sscanf(tmp + 8, "%u-%u", &from, &to);
		free(tmp);

		tmp = input_header("replace: ");
		char *newst = input_header("with: ");
		ushort tmp_len = strlen(tmp), newst_len = strlen(newst);
		long offset = 0;
		uint count = 0;

		list<gap_buf>::iterator iter = text.begin();
		advance(iter, from);
		for (uint i = from; i <= to; ++i, ++iter) {
			vector<uint> matches = search_a(*iter, tmp, tmp_len);
			count += matches.size();
			for (uint j = 0; j < matches.size(); ++j) {
				mv_curs(*iter, (long)matches[j] + offset);
				iter->set_gpe(iter->gpe() + tmp_len);
				insert_s(*iter, matches[j] + offset, newst, newst_len);
				offset += (long)newst_len - (long)tmp_len;
			}
			offset = 0;
		}
		char *tmp_buff = (char*)malloc(128);
		sprintf(tmp_buff, "Replaced %u occurences of \"%s\" with \"%s\" from line %u to %u", count, tmp, newst, from, to);
		print2header(tmp_buff, 1);
		print_text(0);
		free(newst);
		free(tmp_buff);
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
	data(*it, rx + 1, it->len() + 1);
	apnd_s(*t, lnbuf, it->len() - rx - 1);
	it->set_gps(rx + 1);
	it->set_gpe(it->cpt() - 1);

	++it; // insert is on previous than current it
	++curnum;
	text.insert(it, *t); // insert new node with text after rx
	--it;
	free(t);
	cut.clear();
	print_text(y);
	if (y < maxy - 1)
		wmove(text_win, y + 1, 0);
	else { // y = maxy; scroll
		wscrl(ln_win, 1);
		mvwprintw(ln_win, maxy - 1, 0, "%3lu", ry + 2);
		wnoutrefresh(ln_win);
		wscrl(text_win, 1);
		++ofy;
		mvprint_line(maxy - 1, 0, *it, 0, 0);
		wmove(text_win, maxy - 1, x);
	}
	ofx = 0;
}

// go to target byte, if necessary cut line
void mvr_scurs(ulong t_byte)
{
	ofx = calc_offset_act(t_byte, 0, *it);
	if (t_byte - ofx <= maxx) // line fits in screen
		wmove(text_win, y, t_byte - ofx - 1);
	else { // cut line 
		cut.clear();
		ulong bytes = 0;
		if (ofx == 0 && t_byte > (uint)5e8) {
			while (bytes + maxx < t_byte) {
				bytes += maxx - 1;
				cut.push_back({maxx - 1, bytes});
				ofx += maxx - 1;
			}
			flag = t_byte % (maxx - 1);
		} else {
			while (1) {
				const ulong nbytes = dchar2bytes(maxx - 1, bytes, *it);
				if (nbytes >= t_byte - 1)
					break;
				cut.push_back({flag, nbytes}); // flag was changed by dchar2bytes
				ofx += flag;
				bytes = nbytes;
			}
		}
		if (t_byte != it->len()) {
			mvprint_line(y, 0, *it, bytes, 0);
			if (!overflows[y])
				clean_mark(y);
			x = bytes2dchar(t_byte, bytes, *it) - 1;
		} else {
			mvprint_line(y, 0, *it, bytes, t_byte);
			clean_mark(y);
			x = (flag == maxx - 1 ? flag : flag - 1);
		}
		if (x + (uint)ofx > t_byte) 
			ofx = t_byte - x + 1;
		wmove(text_win, y, x);
	}
}

// go to start-of-line, uncut line if needed
void sol()
{
	if (!cut.empty()) { // line has been cut
		mvprint_line(y, 0, *it, 0, 0);
		highlight(y, *it);
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
	mvwprintw(ln_win, maxy - 1, 0, "%3lu", ry + 2);
	wnoutrefresh(ln_win);
	mvprint_line(y, 0, *it, 0, 0);
	highlight(y, *it);
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
	mvwprintw(ln_win, 0, 0, "%3lu", ry);
	wnoutrefresh(ln_win);
	mvprint_line(0, 0, *it, 0, 0);
	highlight(0, *it);
	wmove(text_win, 0, 0);
}

// left arrow
ushort left()
{
	if (x == 0 && ofx == 0 && ofy > 0 && y == 0) {
		scrollup();
		eol();
		return SCROLL;
	} else if (x == 0 && !cut.empty()) { // line has been cut
		clearline;
		ofx -= cut.back().dchar;
		cut.pop_back();
		print_line(*it, cut.empty() ? 0 : cut.back().byte, 0, y);
		const uint tmp = flag; // changed later by highlight
		highlight(y, *it);
		wmove(text_win, y, tmp);
		return CUT;
	} else if (x > 0) { // go left
		wmove(text_win, y, x - 1);
		// handle special characters causing offsets
		if (it->buffer()[it->gps() - 1] == '\t')
			ofx += prevdchar();
		else if (it->buffer()[it->gps() - 1] < 0)
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
ushort right() {
	if (rx >= it->len() - 1 && ry < curnum) { // go to next line
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
		cut.push_back({x, (cut.empty() ? 0 : cut.back().byte) + print_line(*it, ofx, 0, y)});
		wmove(text_win, y, 0);
		return CUT;
	} else { // go right
		wmove(text_win, y, x + 1);
		if (it->buffer()[it->gpe() + 1] == '\t') {
			if (x >= maxx - 7)
				goto cut_line;
			ofx -= 8 - x % 8 - 1;
			wmove(text_win, y, x + 8 - x % 8);
		} else if (it->buffer()[it->gpe() + 1] < 0)
			++ofx;
		return NORMAL;
	}
	return NOTHING;
}

// go to end/start of previous word (call like (prnxt_word(left)))
void prnxt_word(ushort func(void))
{
	ushort status;
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
