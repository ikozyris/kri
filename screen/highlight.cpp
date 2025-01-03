#include "headers/highlight.h"

bool eligible; // is syntax highlighting enabled
const char *types[] = {"int", "char", "float", "double", "unsigned", "void", "const", 
	"size_t", "bool", "signed", "long", "enum", "static", "short", "extern", "uint64_t", 
	"uint32_t", "uint16t", "uint8t", "int64_t", "int32_t", "int16_t", "int8_t"};
const char *defs[]  = {"if", "else", "while", "for", "do", "return", "sizeof", "switch",
	"goto", "case", "break", "struct", "default", "continue", "true", "false"};
const char oper[]  = {'=', '+', '-', '*', '/', '&', '|', '^', '~', '<', '>', '[', ']'};
unsigned char types_len[] = {3, 4, 5, 6, 8, 4, 5, 6, 4, 6, 4, 4, 6, 5, 6, 8, 8, 8, 7, 7, 7, 7, 6};
unsigned char defs_len[] = {2, 4, 5, 3, 2, 6, 6, 6, 4, 4, 5, 6, 7, 8, 4, 5};

#define DEFINC	COLOR_CYAN
#define COMMENT	COLOR_GREEN
#define TYPES	COLOR_RED
#define OPER	COLOR_YELLOW
#define DEFS    COLOR_BLUE
#define STR	COLOR_MAGENTA
// TODO: color for numbers?

#define nelems(x)  (sizeof(x) / sizeof((x)[0]))
#define is_separator(ch) ((ch > 31 && ch < 48) || (ch > 57 && ch < 65) || (ch > 90 && ch < 95) || ch > 122)

wchar_t tmp[256];
char str[256];

// checks if file is C source code
bool isc(const char *str)
{
	unsigned res = whereis(str, '.');
	if (res == 0)
		return false;
	str += res;
	if (!strcmp(str, "c") || !strcmp(str, "cpp") || !strcmp(str, "cc")
		|| !strcmp(str, "h") || !strcmp(str, "hpp"))
		return true;
	return false;
}

typedef struct res_s {
	unsigned char len;
	char type;
} res_t;

// identify color to use
res_t get_category(const char *line)
{
	res_t res;
	res.len = 0;
	res.type = COLOR_WHITE;

	for (unsigned i = 0; i < nelems(types); ++i)
		if (strncmp(types[i], line, types_len[i]) == 0) {
			res.len = types_len[i];
			res.type = TYPES;
			return res;
		}
	for (unsigned i = 0; i < nelems(defs); ++i)
		if (strncmp(defs[i], line, defs_len[i]) == 0) {
			res.len = defs_len[i];
			res.type = DEFS;
			return res;
		}
	for (unsigned i = 0; i < nelems(oper); ++i)
		if (oper[i] == line[0]) {
			res.len = 1;
			res.type = OPER;
			return res;
		}
	return res;
}

// highight line if eligible = true
void apply(unsigned line)
{
	if (!eligible)
		return;
	wmove(text_win, line, 0);
	winwstr(text_win, tmp);
	unsigned len = wcstombs(str, tmp, min(256, maxx - 2));
	unsigned previ = 0;

	for (unsigned i = 0; i < len; ++i) {
		wmove(text_win, line, i);
		if (str[i] == '#') { // define / include
			wchgat(text_win, -1, 0, DEFINC, 0);
			return;
		} else if (str[i] == '/' && str[i + 1] == '/') { // comments
			wchgat(text_win, -1, 0, COMMENT, 0);
			return;
		} else if (str[i] == '/' && str[i + 1] == '*') {
			previ = i;
			i += 2;
			// TODO: end of multi-line comment might be in next line
			while (str[i] != '*' && str[i + 1] != '/' && i < len)
				++i;
			wchgat(text_win, i++ - previ + 2, 0, COMMENT, 0);
		} else if (str[i] == '\'') { // string / char
			previ = i++;
			while (str[i] != '\'' && i < len)
				++i;
			wchgat(text_win, i - previ + 1, 0, STR, 0);
		} else if (str[i] == '\"') {
			previ = i++;
			while (str[i] != '\"' && i < len)
				++i;
			wchgat(text_win, i - previ + 1, 0, STR, 0);
		} else { // type (int, char) / keyword (if, return) / operator (=, +)
			res_t res = get_category(str + i);

			// no highlight for non-separated matches
			bool next = is_separator(str[i + res.len]);
			bool prev = i == 0 ? 1 : is_separator(str[i - 1]);
			// except for operators which are separators
			if ((next && prev) || res.type == OPER)
				wchgat(text_win, res.len, 0, res.type, 0);
		}
	}
}

// wrapper for apply()
void highlight(unsigned y)
{
#ifdef HIGHLIGHT
	apply(y);
#endif
}
