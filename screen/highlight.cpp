#include "headers/highlight.h"

bool eligible; // is syntax highlighting enabled
// each array and its element length has to be sorted (for binary search)
static const char types[] = {"bool""char""const""double""enum""float""int""int16_t""int32_t""int64_t""long""short"
	"signed""size_t""uchar""uint""uint16_t""uint32_t""uint64_t""uint8_t""ulong""unsigned""ushort""void"};
// prefix sum of array
static uchar types_len[] = {4, 8, 13, 19, 23, 28, 31, 38, 45, 52, 56, 61, 67, 73, 78, 82, 90, 98, 106, 113, 118, 126, 132, 136};
static const char keywords[] = {"break""case""continue""default""do""else""extern""false""for""goto""if""inline"
	"return""sizeof""static""struct""switch""true""while"};
static uchar keywords_len[] = {5, 9, 17, 24, 26, 30, 36, 41, 44, 48, 50, 56, 62, 68, 74, 80, 86, 90, 95};
static const char oper[] = {'!', '%', '&', '*', '+', '-', '/', ':', '<', '=', '>', '?', '[', ']', '^', '|', '~', 0};

#define DEFINC	COLOR_CYAN
#define COMMENT	COLOR_GREEN
#define TYPE	COLOR_RED
#define OPER	COLOR_YELLOW
#define KEYWORD	COLOR_BLUE
#define STR	COLOR_MAGENTA
// TODO: color for numbers?

// checks if file is C source code
bool isc(const char *str)
{
	uint res = whereis(str, '.');
	if (res == 0)
		return false;
	str += res;
	if (!strcmp(str, "c") || !strcmp(str, "cpp") || !strcmp(str, "cc")
		|| !strcmp(str, "h") || !strcmp(str, "hpp"))
		return true;
	return false;
}

typedef struct res_s {
	uchar len;
	char type;
} res_t;

// helper binary search
bool binary_search(const char *arr, const uchar *len_arr, uint size, const char *line, res_t &res, char type)
{
	int lo = 0, hi = size - 1, mid;
	while (lo <= hi) {
		mid = lo + (hi - lo) / 2;
		int cmp = strncmp(arr + len_arr[mid-1], line, len_arr[mid] - len_arr[mid - 1]);
		if (cmp == 0) {
			res.len = len_arr ? (len_arr[mid] - len_arr[mid - 1]) : 1;
			res.type = type;
			return true;
		} else if (cmp < 0)
			lo = mid + 1;
		else
			hi = mid - 1;
	}
	return false;
}

#define nelems(x)  (sizeof(x) / sizeof((x)[0]))
bool is_separator(char ch) { return (ch > 31 && ch < 48) || (ch > 57 && ch < 65) || (ch > 90 && ch < 95) || ch > 122; }
#define lookup(x) while (i < len && lnbuf[i] != x) ++i
inline ulong lookup2(const gap_buf &buf, ulong i) { // len = 2, boyer-moore is not useful
	while (i < buf.len() && !(at(buf, i) == '*' && at(buf, i + 1) == '/'))
		++i;
	return i;
}

// identify color to use
res_t get_category(const char *line)
{
	res_t res;
	res.len = 0;
	res.type = COLOR_WHITE;

	if (binary_search(types, types_len, nelems(types_len), line, res, TYPE));
	else if (binary_search(keywords, keywords_len, nelems(keywords_len), line, res, KEYWORD));
	else if (whereis(oper, line[0])) { // binary search would require an array of the form {1,2,3..}
		res.len = 1;
		res.type = OPER;
	}
	return res;
}

bool comment;
// highight line
void apply(uint line, const gap_buf &buf)
{
	if (line == 0) // there is no previous line visible
		comment = false;
	wmove(text_win, line, 0);

	calc_offset_act(buf.len(), 0, buf);
	const uint len = min(maxx - 1, flag);
	if (len > lnbf_cpt) { // resize to fit line
		free(lnbuf);
		lnbuf = (char*)malloc(lnbf_cpt = len);
	}
	winnstr(text_win, lnbuf, len);
	uint previ = 0, i = 0;

	// previous line was a multi-line comment, this might be too
	if (comment) {
		ulong pos = lookup2(buf, 0);
		if (pos == buf.len()) { // still a comment
			wchgat(text_win, len, 0, COMMENT, 0);
			return;
		}
		
		comment = false;
		if (pos > len) { // ends after len
			wchgat(text_win, len, 0, COMMENT, 0);
			return;
		}
		
		calc_offset_act(pos + 2, 0, buf);
		wchgat(text_win, flag, 0, COMMENT, 0);
	
		i = flag; // continue from end of comment
	}

	for (; i < len; ++i) {
		wmove(text_win, line, i);
		if (lnbuf[i] == '#') { // define / include
			wchgat(text_win, maxx - i - 1, 0, DEFINC, 0);
			return;
		} else if (lnbuf[i] == '/' && lnbuf[i + 1] == '/') { // comments
			wchgat(text_win, maxx - i - 1, 0, COMMENT, 0);
			return;
		} else if (lnbuf[i] == '/' && lnbuf[i + 1] == '*') {
			previ = i;
			i += 2;
			ulong pos = lookup2(buf, i); // comment might end in this line

			i = min(pos + 1, len);
			if (i >= len - 1) // comment continues in next line
				comment = true;
			
			wchgat(text_win, i - previ + 1, 0, COMMENT, 0);
		} else if (lnbuf[i] == '\'') { // string / char
			previ = i++;
			lookup('\'');
			wchgat(text_win, i - previ + 1, 0, STR, 0);
		} else if (lnbuf[i] == '\"') {
			previ = i++;
			lookup('\"');
			wchgat(text_win, i - previ + 1, 0, STR, 0);
		} else { // type (int, char) / keyword (if, return) / operator (=, +)
			res_t res = get_category(lnbuf + i);
			if (res.len == 0)
				continue;

			// no highlight for non-separated matches
			bool next = is_separator(lnbuf[i + res.len]);
			bool prev = i == 0 ? true : is_separator(lnbuf[i - 1]);
			// except for operators which are separators
			if ((next && prev) || res.type == OPER)
				wchgat(text_win, res.len, 0, res.type, 0);
			i += res.len - 1;
		}
	}
}

// wrapper for apply()
void highlight(uint line, const gap_buf &buf)
{
#ifdef HIGHLIGHT
	if (eligible)
		apply(line, buf);
#endif
}
