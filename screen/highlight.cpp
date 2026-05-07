#include "headers/highlight.h"
#include "headers/syntax-tables.h"

bool eligible; // is syntax highlighting enabled
static const lang_t *lang; // tables are dynamically swapped for each language
// TODO: color for numbers?

static const struct {
	const char *ext;
	const lang_t *l;
} ext_map[] = {
	// TODO: separate C and C++
	{"c",   &lang_c}, {"cpp", &lang_c}, {"cc", &lang_c}, {"h", &lang_c}, {"hpp", &lang_c},
	{"mk", &lang_make}, {"md", &lang_md}
};

// detect language from filename, TODO: check first line content (shebang etc.)
bool detect_lang(const char *str)
{
	// special-case: basename match (e.g. "Makefile")
	const char *base = strrchr(str, '/');
	base = base ? base + 1 : str;
	if (strcmp(base, "Makefile") == 0) {
		lang = &lang_make;
		return true;
	}

	const char *dot = strrchr(str, '.');
	if (dot == 0)
		goto none;
	for (uint i = 0; i < nelems(ext_map); ++i) {
		if (strcmp(dot + 1, ext_map[i].ext) == 0) {
			lang = ext_map[i].l;
			return true;
		}
	}
none:
	lang = &lang_none;
	return false;
}

typedef struct res_s {
	uchar len;
	char type;
	attr_t attr;
} res_t;

// helper binary search
static bool binary_search(const char *arr, const uchar *len_arr, uint size, const char *line, res_t &res, char type, attr_t attr)
{
	int lo = 0, hi = size - 1, mid;
	while (lo <= hi) {
		mid = (hi + lo) / 2;
		int cmp = strncmp(arr + len_arr[mid], line, len_arr[mid + 1] - len_arr[mid]);
		if (cmp == 0) {
			res.len = len_arr[mid + 1] - len_arr[mid];
			res.type = type;
			res.attr = attr;
			return true;
		} else if (cmp < 0)
			lo = mid + 1;
		else
			hi = mid - 1;
	}
	return false;
}

static bool is_separator(char ch) { return (ch > 31 && ch < 48) || (ch > 57 && ch < 65) || (ch > 90 && ch < 95) || ch > 122; }
static inline uint lookup2(const iter *cur_ln, uint start, uint len)
{
	uint line_end = len + cur_ln->offset, j;
	for (uint i = start + cur_ln->offset; i + lang->comm_clen <= line_end; ++i) {
		for (j = 0; j < lang->comm_clen; ++j)
			if (at(*cur_ln->orig, i + j) != lang->comm_cl[j])
				break;
		if (j == lang->comm_clen)
			return i - cur_ln->offset;
	}
	return len;
}

// identify color to use
static res_t get_category(const char *line)
{
	res_t res = {0, COLOR_WHITE, 0};

	for (uchar i = 0; i < lang->wordgr_cnt; ++i)
		if (binary_search(lang->words[i].words, lang->words[i].lens, lang->words[i].cnt, line, res, lang->words[i].color, lang->words[i].attr))
			return res;

	return res;
}

static vector<pair<uint, uint>> comment_blocks;
const char COMMENT = COLOR_GREEN; // color doesn't really matter
const char OPER = COLOR_YELLOW; // here it does
static char continued; // string/comment/directive etc. continued after cut/ in next line
// highight line
static void apply(uint line, const iter *cur_ln)
{
	if (line == 0) { // there is no previous line visible
		continued = 0;

		if (comment_blocks.size() && comment_blocks.back().first > cur_ln->global_pos)
			comment_blocks.pop_back();
		if (comment_blocks.size()) {
			auto &last_block = comment_blocks.back();
			if (cur_ln->global_pos >= last_block.first && cur_ln->global_pos <= last_block.second)
				continued = COMMENT;
		}
	}
	wmove(text_win, line, 0);

	const uint len = min(maxx - 1, bytes2dchar(cur_ln->len(), 0, cur_ln));
	if (len >= lnbf_cpt) { // resize to fit line
		free(lnbuf);
		lnbf_cpt = __bit_ceil(len + 1);
		lnbuf = (char*)malloc(lnbf_cpt);
	}
	winnstr(text_win, lnbuf, len);
	uint previ = 0, i = 0;

	// previous line was a multi-line comment, this might be too
	if (continued == COMMENT) {
		uint pos = lookup2(cur_ln, 0, cur_ln->len());
		if (pos == cur_ln->len()) { // still a comment
			wchgat(text_win, len, 0, COMMENT, 0);
			return;
		}

		continued = 0; // found the end of this block
		if (comment_blocks.size() && comment_blocks.back().second == UINT_MAX)
			comment_blocks.back().second = cur_ln->global_pos;

		if (pos > len) { // ends after len
			wchgat(text_win, len, 0, COMMENT, 0);
			return;
		}

		i = bytes2dchar(pos + lang->comm_clen, 0, cur_ln); // continue from end of comment
		wchgat(text_win, i, 0, COMMENT, 0);
	}

	for (; i < len; ++i) {
		wmove(text_win, line, i);

		for (uint j = 0; j < lang->lntrait_cnt; ++j)
			if (strncmp(lnbuf + i, lang->ln_traits[j].mark, lang->ln_traits[j].len) == 0) {
				if (lang->ln_traits[j].start && i != 0)
					continue;
				wchgat(text_win, maxx - i - 1, lang->ln_traits[j].attr, lang->ln_traits[j].color, 0);
				return;
			}

		for (uint j = 0; j < lang->delim_cnt; ++j) {
			if (strncmp(lnbuf + i, lang->delims[j].delim, lang->delims[j].len) != 0)
				continue;
			previ = i;
			for (i += lang->delims[j].len; i + lang->delims[j].len <= len; ++i)
				if (strncmp(lnbuf + i, lang->delims[j].delim, lang->delims[j].len) == 0)
					break;
			i += lang->delims[j].len - 1; // last char of closing delim
			wchgat(text_win, i - previ + 1, lang->delims[j].attr, lang->delims[j].color, 0);
			goto next;
		}

		if (lang->comm_op && starts_with(lnbuf + i, lang->comm_op)) {
			previ = i;
			i = dchar2bytes(i + strlen(lang->comm_op), 0, cur_ln);
			uint pos = lookup2(cur_ln, i, cur_ln->len());

			if (pos == cur_ln->len()) { // comment continues in next line
				continued = COMMENT; // start of new block
				if (comment_blocks.empty() || comment_blocks.back().second != UINT_MAX)
					comment_blocks.push_back({cur_ln->global_pos, UINT_MAX});
				wchgat(text_win, len - previ, 0, COMMENT, 0);
				return;
			}

			i = bytes2dchar(pos + lang->comm_clen, 0, cur_ln);
			wchgat(text_win, i - previ + 1, 0, COMMENT, 0);
		} else { // type (int, char) / keyword (if, return) / operator (=, +)
			res_t res = get_category(lnbuf + i);
			if (res.len == 0)
				continue;

			// no highlight for non-separated matches
			bool next = is_separator(lnbuf[i + res.len]);
			bool prev = i == 0 ? true : is_separator(lnbuf[i - 1]);
			// except for operators which are separators
			if ((next && prev) || res.type == OPER)
				wchgat(text_win, res.len, res.attr, res.type, 0);
			i += res.len - 1;
		}
next:; // continue; but for when inside other loop
	}
}

// wrapper for apply()
void highlight(uint line, const iter *i)
{
#ifdef HIGHLIGHT
	if (eligible)
		apply(line, i);
#endif
}
