#include "headers/io.h"

// pos: (1 left) (3 right) (else center)
void print2header(const char *msg, uchar pos)
{
	if (pos == 3)
		mvwprintw(header_win, 0, maxx - strlen(msg), "%s", msg);
	else if (pos == 1)
		mvwprintw(header_win, 0, 0, "%s", msg);
	else {
		uchar hmx = maxx / 2;
		// 20-width spaces + 0
		mvwprintw(header_win, 0, hmx - 10, "                    ");
		mvwprintw(header_win, 0, hmx - strlen(msg) / 2, "%s", msg);
	}
	wrefresh(header_win);
}

// Ask for input from header
char *input_header(const char *q)
{
	clear_header();
	wmove(header_win, 0, 0);
	wprintw(header_win, "%s", q);
	wmove(header_win, 0, strlen(q));
	echo();
	char *tmp = (char*)malloc(sizeof(char) * 128);
	if (wgetnstr(header_win, tmp, 128) == ERR) { [[unlikely]]
		reset_header();
		print2header("ERROR", 1);
		wmove(text_win, y, x);
	} if (strlen(tmp) <= 0)
		tmp[0] = 0;
	noecho();
	return tmp;
}

// prints substring of buffer from: (curr x + 'from' bytes), if (to == 0) print until maxx
uint print_line(const gap_buf &buffer, uint from, uint to, uint y)
{
	// only newline or emulated newline ('\0') is in buffer
	if (buffer.len() <= 1)
		return 0;
	if (to == 0) {
		uint prevx = getcurx(text_win); // in case x != 0 (mvprint_line)
		uint prop = dchar2bytes(maxx - 1 - prevx, from, buffer);
		if (prop < buffer.len() - 1) {
			to = prop;
			overflows[y] = true;
		} else {
			to = buffer.len();
			overflows[y] = false;
		}
	}
	uint rlen = data(buffer, from, to);
	if (lnbuf[rlen - 1] == '\n' || lnbuf[rlen - 1] == '\t')
		--rlen;
	waddnstr(text_win, lnbuf, rlen);
	wclrtoeol(text_win);
	print_del_mark(y);
	return rlen;
}

// print text starting from line
void print_text(uint line)
{
	list<gap_buf>::iterator iter = text.begin();
	advance(iter, ofy + line);
	wmove(text_win, line, 0);
	wclrtobot(text_win);
	wmove(text_win, line, 0);
	for (uint ty = line; ty < min(curnum + ofy + 1, maxy) && iter != text.end(); ++iter, ++ty) {
		mvprint_line(ty, 0, *iter, 0, 0);
		highlight(ty, *iter);
	}
}

// deleted a char; the mark moved left | invalidates flag
void print_new_mark()
{
	uint char_pos = dchar2bytes(maxx - 2, 0, *it);
	if (flag < maxx - 2) { // after deleting a char, new len could be < maxx
		if (overflows[y] == true) {
			overflows[y] = false;
			clean_mark(y);
		}
		return;
	}
	print_del_mark(y);
	char chp = at(*it, char_pos);
	if (chp == '\t')
		clean_mark(y);
	else if (chp > 0)
		mvwaddch(text_win, y, maxx - 2, chp);
	else if (chp < 0) { // 2 bytes to print
		char chp2 = at(*it, char_pos + 1);
		const char tmp[2] = {chp, chp2};
		mvwaddnstr(text_win, y, maxx -2, tmp, 2);
	}
}

// save buffer to global filename, if empty ask for it on header
void save()
{
	if (!filename)
		filename = (char*)input_header("Enter filename: ");
	FILE *fo = fopen(filename, "w");
	list<gap_buf>::iterator iter = text.begin();
	for (uint i = 0; iter != text.end() && i <= curnum; ++iter, ++i) {
		iter->buffer()[iter->gps()] = 0; // null-terminate substring
		fputs(iter->buffer(), fo); // print up to gps
		fputs(iter->buffer() + iter->gpe() + 1, fo); // print remaining bytes
	}
	fclose(fo);

	reset_header();
	print2header("Saved", 1);
	wmove(text_win, y, x);
}

// For size see: https://github.com/ikozyris/kri/wiki/Comments-on-optimizations#buffer-size-for-reading
#define SZ 524288 // 512 KiB

void read_fgets(FILE *fi)
{
	char *tmp = (char*)malloc(SZ);
	while ((fgets_unlocked(tmp, SZ, fi))) {
		apnd_s(*it, tmp);
		if (it->buffer()[it->len() - 1] == '\n') { [[unlikely]]
			if (++curnum >= text.size()) [[unlikely]]
				text.resize(text.size() * 2);
			++it;
		}
	}
	free(tmp);
}

void read_fread(FILE *fi)
{
	char *tmp = (char*)malloc(SZ + 1);
	uint a, j = 0, res;
	while ((a = fread(tmp, sizeof(tmp[0]), SZ, fi))) {
		tmp[a] = 0;
		while ((res = whereis(tmp + j, '\n')) > 0) {
			apnd_s(*it, tmp + j, res);
			j += res;
			if (++curnum >= text.size())
				text.resize(text.size() * 2);
			++it;
		}
		// if last character is not a newline
		apnd_s(*it, tmp + j, a - j);
		j = 0;
	}
	free(tmp);
}
