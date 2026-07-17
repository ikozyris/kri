#include <ncurses.h>
typedef unsigned char uchar;
#define nelems(x) (sizeof(x) / sizeof((x)[0]))
// each array and its element length has to be sorted (for binary search)

struct delim {
	const char *delim;
	uchar len;
	char color;
	attr_t attr;
};

struct word_group {
	const char *words;
	const uchar *lens;
	uchar cnt;
	char color;
	attr_t attr;
};

struct line_trait { // like delimeters but always end at EOL
	const char *mark;
	uchar len;
	char color;
	bool start; // must start at x=0
	attr_t attr;
};

struct lang_t {
	const word_group *words;	// keywords group
	const delim *delims;	// string-like single line delimiters
	const line_trait *ln_traits;	// whole line highlighted
	uchar wordgr_cnt;	// lengths together for padding
	uchar delim_cnt;
	uchar lntrait_cnt;

	uchar comm_olen;	// opening length
	uchar comm_clen;	// closing length
	const char *comm_op;	// multi-line comment open
	const char *comm_cl;	// multi-line comment close
};

// C
static const word_group c_words[] = {
{
	"bool""char""const""double""enum""float""int""int16_t""int32_t""int64_t""long""short"
	"signed""size_t""uchar""uint""uint16_t""uint32_t""uint64_t""uint8_t""ulong""unsigned""ushort""void",
	(const uchar[]){0, 4, 8, 13, 19, 23, 28, 31, 38, 45, 52, 56, 61, 67, 73, 78, 82, 90, 98, 106, 113, 118, 126, 132, 136},
	24, COLOR_RED, 0
},
{
	"break""case""continue""default""do""else""extern""false""for""goto""if""inline"
	"return""sizeof""static""struct""switch""true""typedef""while",
	(const uchar[]){0, 5, 9, 17, 24, 26, 30, 36, 41, 44, 48, 50, 56, 62, 68, 74, 80, 86, 90, 97, 102},
	19, COLOR_BLUE, 0
},
{
	"!%&*+-/:<=>?[]^|~", // memchr is better but this modular
	(const uchar[]){0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17},
	17, COLOR_YELLOW, 0
}
};

static const line_trait c_lntraits[] = {
	{"#", 1, COLOR_CYAN, false, 0},
	{"//", 2, COLOR_GREEN, false, 0}
};

static const delim c_delims[] = {
	{"'", 1, COLOR_MAGENTA, 0},
	{"\"", 1, COLOR_MAGENTA, 0}
};

static const lang_t lang_c = {
	c_words, c_delims, c_lntraits,
	nelems(c_words), nelems(c_delims), nelems(c_lntraits),
	2, 2, "/*", "*/"
};

// Makefile
static const word_group mk_words[] = {{"=", (const uchar[]){0, 1}, 1, COLOR_YELLOW, 0}};
static const line_trait mk_lntraits[] = {{"#", 1, COLOR_GREEN, false, 0}};
static const lang_t lang_make = {
	mk_words, nullptr, mk_lntraits,
	nelems(mk_words), 0, nelems(mk_lntraits),
	0, 0, nullptr, nullptr
};

// Markdown
static const word_group md_words[] = {
	{"|", (const uchar[]){0, 1}, 1, COLOR_YELLOW, 0}
};

static const delim md_delims[] = {
	{"**", 2, COLOR_RED, A_BOLD},
	{"~~", 2, COLOR_WHITE, A_DIM}, // ncurses for some weird reason doesn't have strikethrough
	{"`", 1, COLOR_CYAN, 0},
	{"*", 1, COLOR_MAGENTA, A_ITALIC}, // why are there multiple syntaxes?
	{"_", 1, COLOR_MAGENTA, A_ITALIC}
};

static const line_trait md_lntraits[] = {
	{"#", 1, COLOR_BLUE, true, A_BOLD},
	{">", 1, COLOR_GREEN, true, 0}
};

static const lang_t lang_md = {
	md_words, md_delims, md_lntraits,
	nelems(md_words), nelems(md_delims), nelems(md_lntraits),
	3, 3, "```", "```"
};

// default, no highlighting
static const lang_t lang_none = {};
