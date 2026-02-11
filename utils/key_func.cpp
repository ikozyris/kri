#include "headers/key_func.h"

// display stats on header
void stats()
{
	char *_tmp = (char*)malloc(256);
#ifndef RELEASE
	uint cutd = 0, cutb = 0;
	if (!cut.empty()) {
		cutb = cut.back().byte;
		cutd = cut.back().dchar;
	}
	snprintf(_tmp, min(maxx, 256), "maxx %u off %u len %u gs %u ge %u cpt %u cut%lu[d%u,b%u] x %u ofx %ld ry %u lines %u   ",
	maxx, it.offset, it.len(), it.gps(), it.gpe(), it.cpt(), cut.size(), cutd, cutb, x, ofx, ry, text.lines);
#else
	ulong sumlen = 0;
	chunk *i;
	for (i = text.head->next; i != text.tail; i = i->next)
		sumlen += i->merged_lines.len();
	snprintf(_tmp, min(maxx, 256), "len %u  cpt %u  y %u  x %u  sum len %lu  lines %u  cut %lu  ofx %ld  ",
		it.len(), it.cpt(), ry, x, sumlen, text.lines, cut.size(), ofx);
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
		snprintf(buffer, 64, "freed: %u B", lnbf_cpt);
		clear_header();
		print2header(buffer, 1);

		// shrink line buffer
		lnbf_cpt = 16;
		lnbuf = (char*)realloc(lnbuf, lnbf_cpt);
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
		if (a <= text.lines) {
			scroll2(a);
			iterate_fw(&it, ofy - ry);
		}
	} else if (strncmp(tmp, "find", 4) == 0) { // example: find string
		uint from = 0, to = text.lines;
		char mode = 'h';
		char *pr2 = input_header("range/mode: "); // 5-10 h
		sscanf(pr2, "%u-%u %c", &from, &to, &mode);
		free(pr2);
		find(tmp + 5, from, to, mode);
	} else if (strncmp(tmp, "replace", 7) == 0) {
		uint from = 0, to = text.lines;
		sscanf(tmp + 6, "%u-%u", &from, &to);
		free(tmp);
		if (to > text.lines || from > to) {
			print2header("Invalid parameters", 1);
			return;
		}

		char *old = input_header("old: ");
		if (old[0] == 0) return;
		char *newst = input_header("new: ");
		ushort old_len = strlen(old), newst_len = strlen(newst);
		uint count = search(old, old_len, from, to, 'h');

		int offset = (int)newst_len - (int)old_len;
		iter tmp_it;
		point2begin(&tmp_it);
		for (uint i = 0; i < occurrences.size(); ++i) { // occurrences in each chunk
			for (uint j = 0; j < occurrences[i].len(); ++j) {
				uint index = occurrences[i].array[j];
				mv_curs(*tmp_it.orig, index + offset * (int)j);
				tmp_it.orig->gpe = tmp_it.orig->gpe + old_len;
				insert_s(*tmp_it.orig, newst, newst_len);

				uint n = line_pos(tmp_it.parent(), index + offset * (int)j);
				tmp_it.parent()->len[n] += offset;
			}
			tmp_it.orig = &tmp_it.parent()->next->merged_lines;
			occurrences[i].set_len(0); // cleanup for next search
		}
		tmp = (char*)malloc(128);
		sprintf(tmp, "Replaced %u occurences of \"%s\" with \"%s\" from line %u to %u", count, old, newst, from, to);
		print2header(tmp, 1);
		print_text(y);

		free(newst);
		free(old);
	} else
		print2header("command not found", 3);
	free(tmp);
}

void scroll2(uint a)
{
	ofy = a - 1;
	ofx = 0;
	cut.clear();
	print_lines();
	wrefresh(ln_win);
	print_text(0);
}

// insert enter in rx of buffer, create new line node and reprint
void enter()
{
	chunk *ch = it.parent();
	insert_c(*it.orig, '\n');

	if (ch->len) { // this chunk has multiple lines; just insert new length
		if (ch->len_cpt < ch->num_lines + 1) {
			ch->len_cpt *= 2;
			ch->len = (uchar*)realloc(ch->len, ch->len_cpt);
		}
		uint pos = it.relative_pos;
		memmove(&ch->len[pos + 1], &ch->len[pos], ch->num_lines - pos);
		ch->num_lines++;
		ch->len[pos + 1] = it.len() - it.gps() + 1;
		ch->len[pos] = it.gps();

		it.offset = it.orig->gps;
		it.relative_pos++;
		if (it.orig->len() > MAX_CHUNK_SIZE)
			split_mline(&text, it.parent());
	} else { // worst case; lines > 256B; create new chunk for the new line
		chunk *t = create_chunk();
		data(*it.orig, rx + 1, it.len() + 1);
		apnd_s(t->merged_lines, lnbuf, it.len() - rx - 1);
		it.orig->gps = rx + 1;
		it.orig->gpe = it.cpt() - 1;

		insc_after(&text, it.parent(), t);
		point2chunk(&it, t);
		it.global_pos++;
	}

	text.lines++;
	ofx = 0;
	cut.clear();
	print_text(y);
	if (y < maxy - 1)
		wmove(text_win, y + 1, 0);
	else { // y = maxy; scroll
		wscrl(ln_win, 1);
		mvwprintw(ln_win, maxy - 1, 0, "%3u", ry + 2);
		wnoutrefresh(ln_win);
		wscrl(text_win, 1);
		++ofy;
		mvprint_line(maxy - 1, 0, &it, 0, 0);
		wmove(text_win, maxy - 1, x);
	}
}

// TODO: this is a repetitive mess
// go to target byte, if necessary cut line
void mvr_scurs(uint t_byte)
{
	ofx = calc_offset_act(t_byte, 0, &it);
	if (t_byte - ofx <= maxx) // line fits in screen
		wmove(text_win, y, t_byte - ofx - 1);
	else { // cut line
		cut.clear();
		uint bytes = 0;
		if (ofx == 0 && t_byte > (uint)5e8) {
			while (bytes + maxx < t_byte) {
				bytes += maxx - 1;
				cut.push_back({maxx - 1, bytes});
				ofx += maxx - 1;
			}
			flag = t_byte % (maxx - 1);
		} else {
			while (1) { // TODO: optimize
				const uint nbytes = dchar2bytes(maxx - 1, bytes, &it);
				if (nbytes >= t_byte)
					break;
				cut.push_back({flag, nbytes}); // flag was changed by dchar2bytes
				ofx += flag;
				bytes = nbytes;
			}
		}
		if (t_byte != it.len()) {
			mvprint_line(y, 0, &it, bytes, 0);
			if (!overflows[y])
				clean_mark(y);
			x = bytes2dchar(t_byte, bytes, &it) - 1;
		} else {
			mvprint_line(y, 0, &it, bytes, t_byte);
			clean_mark(y);
			x = (flag == maxx - 1 ? flag : flag - 1);
		}
		if (x + (uint)ofx > t_byte)
			ofx = t_byte - x + 1;
		wmove(text_win, y, x);
	}
}

void mvl_scurs(uint t_byte)
{
	while (cut.back().byte > t_byte) {
		ofx -= cut.back().dchar;
		cut.pop_back();
	}
	mvprint_line(y, 0, &it, cut.back().byte, 0);
}

// go to start-of-line, uncut line if needed
void sol()
{
	if (!cut.empty()) { // line has been cut
		mvprint_line(y, 0, &it, 0, 0);
		highlight(y, &it);
	}
	cut.clear();
	wmove(text_win, y, ofx = 0);
}

// scroll screen down, print last line
void scrolldown()
{
	iterate_fw(&it, 1);
	++ofy;
	cut.clear();
	ofx = 0;
	wscrl(text_win, 1);
	wscrl(ln_win, 1);
	mvwprintw(ln_win, maxy - 1, 0, "%3u", ry + 2);
	wnoutrefresh(ln_win);
	mvprint_line(y, 0, &it, 0, 0);
	highlight(y, &it);
	wmove(text_win, y, 0);
}

// scroll screen up, print first line
void scrollup()
{
	--ofy;
	iterate_bw(&it, 1);
	cut.clear();
	ofx = 0;
	wscrl(text_win, -1);
	wscrl(ln_win, -1);
	mvwprintw(ln_win, 0, 0, "%3u", ry);
	wnoutrefresh(ln_win);
	mvprint_line(0, 0, &it, 0, 0);
	highlight(0, &it);
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
		print_line(&it, cut.empty() ? 0 : cut.back().byte, 0, y);
		const uint tmp = flag; // changed later by highlight
		highlight(y, &it);
		wmove(text_win, y, tmp);
		return CUT;
	} else if (x > 0) { // go left
		wmove(text_win, y, x - 1);
		// handle special characters causing offsets
		if (it.buffer()[it.gps() - 1] == '\t')
			ofx += prevdchar();
		else if (it.buffer()[it.gps() - 1] < 0)
			--ofx;
		return NORMAL;
	} else if (y > 0) { // x = 0
		iterate_bw(&it, 1);
		--y;
		eol();
		return LN_CHANGE;
	}
	return NOTHING;
}

// right arrow
ushort right() {
	// let len overflow if 0
	if (rx >= it.len() - 1 && ry < text.lines) { // go to next line
		if (y == maxy - 1) {
			scrolldown();
			return SCROLL;
		} else if (!cut.empty()) // revert cut
			mvprint_line(y, 0, &it, 0, 0);
		wmove(text_win, y + 1, 0);
		iterate_fw(&it, 1);
		cut.clear();
		ofx = 0;
		return LN_CHANGE;
	} else if (x == maxx - 1) { // right to cut part of line
cut_line:
		clearline;
		ofx += x;
		cut.push_back({x, (cut.empty() ? 0 : cut.back().byte) + print_line(&it, ofx, 0, y)});
		wmove(text_win, y, 0);
		return CUT;
	} else if (rx < it.len()) { // go right
		wmove(text_win, y, x + 1);
		if (it.buffer()[it.gpe() + 1] == '\t') {
			if (x >= maxx - 7)
				goto cut_line;
			ofx -= 8 - x % 8 - 1;
			wmove(text_win, y, x + 8 - x % 8);
		} else if (it.buffer()[it.gpe() + 1] < 0)
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
		mv_curs(*it.orig, x + ofx);
	} while ((winch(text_win) & A_CHARTEXT) != ' ' && status == NORMAL);
}

void reset_view()
{
	ofy = ofx = 0;
	cut.clear();
	point2begin(&it);
	print_text(0);
	reset_header();
	print_lines();
	wmove(text_win, 0, 0);
	wnoutrefresh(ln_win);
	wnoutrefresh(header_win);
	doupdate();
}
