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
	}
	noecho();
	return tmp;
}

// prints substring of buffer from: (curr x + 'from' bytes), if (to == 0) print until maxx
uint print_line(const iter *i, uint from, uint to, uint y)
{
	// only newline or emulated newline ('\0') is in buffer
	if (i->len() <= 1)
		return 0;
	if (to == 0) {
		uint prevx = getcurx(text_win); // in case x != 0 (mvprint_line)
		uint prop_bytes = dchar2bytes(maxx - 1 - prevx, from, i);
		if (prop_bytes < i->len() - 1) {
			to = prop_bytes;
			overflows[y] = true;
		} else {
			to = i->len();
			overflows[y] = false;
		}
	}
	uint rlen = data(*i->orig, from + i->offset, to + i->offset);
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
	iter i;
	point2chunk(&i, text.head->next);
	i.global_pos = 0;
	if (ofy + line)
		iterate_fw(&i, ofy + line);
	wmove(text_win, line, 0);
	wclrtobot(text_win);
	mvprint_line(line, 0, &i, 0, 0);
	highlight(line, &i);
	for (uint ty = line + 1; ty <= min(text.lines - ofy, maxy - 1); ++ty) {
		iterate_fw(&i, 1);
		mvprint_line(ty, 0, &i, 0, 0);
		highlight(ty, &i);
	}
}

// deleted a char; the mark moved left | invalidates flag
void print_new_mark()
{
	uint char_pos = dchar2bytes(maxx - 2, 0, &it);
	if (flag < maxx - 2) { // after deleting a char, new len could be < maxx
		if (overflows[y] == true) {
			overflows[y] = false;
			clean_mark(y);
		}
		return;
	}
	print_del_mark(y);
	char chp = at(*it.orig, char_pos + it.offset);
	if (chp == '\t')
		clean_mark(y);
	else if (chp > 0)
		mvwaddch(text_win, y, maxx - 2, chp);
	else if (chp < 0) { // 2 bytes to print
		char chp2 = at(*it.orig, char_pos + 1);
		const char tmp[2] = {chp, chp2};
		mvwaddnstr(text_win, y, maxx - 2, tmp, 2);
	}
}

// save buffer to global filename, if empty ask for it on header
void save()
{
	if (!filename)
		filename = (char*)input_header("Enter filename: ");
	FILE *fo = fopen(filename, "w");
	if (!fo) {
		print2header("Unable to open file for writing, check permissions", 1);
		wmove(text_win, y, x);
		return;
	}
#define gpb i->merged_lines
	chunk *i = text.head->next;
	for (uint j = 0; i != text.tail->prev && j < text.nodes; ++j, i = i->next) {
		fwrite(gpb.buffer(), 1, gpb.gps, fo);
		fwrite(gpb.buffer() + gpb.gpe + 1, 1, gpb.cpt() - gpb.gpe - 1, fo); // print remaining bytes
	}
	// last line may have a \0 byte at i->length, don't print it | TODO: simplify
	uint end = gpb.gps;
	if (end > 0 && gpb.buffer()[gpb.gps - 1] == 0)
		end--;	
	fwrite(gpb.buffer(), 1, end, fo);
	end = gpb.cpt() - gpb.gpe - 1;
	if (end > 0 && gpb.buffer()[gpb.cpt() - 1] == 0)
		end--;
	fwrite(gpb.buffer() + gpb.gpe + 1, 1, end, fo);
#undef gpb
	fclose(fo);
	reset_header();
	print2header("Saved", 1);
	wmove(text_win, y, x);
}

#define SZ 2*1024*1024

uint fgets_ret(char *buf, FILE *in)
{
	uint prev = ftell(in);
	fgets_unlocked(buf, SZ, in);
	uint next = ftell(in);
	return next - prev;
}

chunk *new_chunk(chunk *chnk) {
	if (!chnk->len) // chunks have 1 line if not merged
		chnk->num_lines = 1; // from 0
	chunk *t = create_chunk();
	text.nodes++;
	connect(chnk, t);
	return t;
}

void read_file2(FILE *in)
{
	chunk *chnk = text.head->next;
	void *buffer = mmap(NULL, SZ, PROT_READ | PROT_WRITE,
			    MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB | MAP_HUGE_2MB, -1, 0);
	char *buf = (char*)buffer;
	uint ln_sz = 0, ch_sz = 0; // current line size, current chunk size

	while (const uint bytes_read = fgets_ret(buf, in)) {
		ln_sz += bytes_read;

		// line won't fit in the current (non empty) chunk and hasn't been written yet
		if (ch_sz + ln_sz >= MAX_CHUNK_SIZE && ln_sz == bytes_read && ch_sz > 0) {
			chnk = new_chunk(chnk);
			ch_sz = 0;
		}
		ch_sz += bytes_read;

		if (bytes_read < SZ - 1 || buf[bytes_read - 1] == '\n') { // found the end of this line
			if (ln_sz < MAX_CHUNK_SIZE) // line can be merged in a chunk
				append_len(chnk, ln_sz);
			text.lines++;
			ln_sz = 0;
		} 
		apnd_s(chnk->merged_lines, buf, bytes_read); // write the line
	}
	munmap(buffer, SZ);
	if (!chnk->len)
		chnk->num_lines = 1;
	connect(chnk, text.tail);
}
