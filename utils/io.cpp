#include "headers/io.h"

// pos: (1 left) (3 right) (else center)
void print2header(const char *msg, unsigned char pos)
{
	if (pos == 3)
		mvwprintw(header_win, 0, maxx - strlen(msg), "%s", msg);
	else if (pos == 1)
		mvwprintw(header_win, 0, 0, "%s", msg);
	else {
		unsigned char hmx = maxx / 2;
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

// prints substring of buffer, if (to == 0) print until maxx
unsigned print_line(const gap_buf &buffer, unsigned from, unsigned to)
{
	// only newline or emulated newline ('\0') is in buffer
	if (buffer.len <= 1)
		return 0;
	if (to == 0) {
		unsigned prevx = getcurx(text_win); // in case x != 0 (mvprint_line)
		unsigned prop = dchar2bytes(maxx - 1 - prevx, from, buffer);
		if (prop < buffer.len - 1) {
			to = prop;
			cchar_t tmp;
			setcchar(&tmp, L">", A_STANDOUT, COLOR_BLACK, nullptr);
			mvwins_wch(text_win, getcury(text_win), maxx - 1, &tmp);
			wmove(text_win, getcury(text_win), prevx);
		} else
			to = buffer.len;
	}
	unsigned rlen = data(buffer, from, to);
	if (lnbuf[rlen - 1] == '\n' || lnbuf[rlen - 1] == '\t')
		--rlen;
	waddnstr(text_win, lnbuf, rlen);
	wclrtoeol(text_win);
	return rlen;
}

// print text starting from line
void print_text(unsigned line)
{
	list<gap_buf>::iterator iter = text.begin();
	advance(iter, ofy + line);
	wmove(text_win, line, 0);
	wclrtobot(text_win);
	wmove(text_win, line, 0);
	for (unsigned ty = line; ty < min(curnum + ofy + 1, maxy) && iter != text.end(); ++iter, ++ty) {
		mvprint_line(ty, 0, *iter, 0, 0);
		highlight(ty);
	}
}

// save buffer to global filename, if empty ask for it on header
void save()
{
	if (!filename)
		filename = (char*)input_header("Enter filename: ");
	FILE *fo = fopen(filename, "w");
	list<gap_buf>::iterator iter = text.begin();
	for (unsigned i = 0; iter != text.end() && i <= curnum; ++iter, ++i) {
		data(*iter, 0, iter->len);
		fputs(lnbuf, fo);
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
		if (it->buffer[it->len - 1] == '\n') { [[unlikely]]
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
	unsigned a, j = 0, res;
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
