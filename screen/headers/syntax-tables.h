#include <ncurses.h>
typedef unsigned char uchar;
#define nelems(x) (sizeof(x) / sizeof((x)[0]))
// each array and its element length has to be sorted (for binary search)

struct delim {
	const char *delim;
	uchar len;
	char color;
};

struct word_group {
	const char *words;
	const uchar *lens;
	uchar cnt;
	char color;
};

struct line_trait { // like delimeters but always end at EOL
	const char *mark;
	uchar len;
	char color;
};

struct lang_t {
	const word_group *words;	// keywords group
	const delim *delims;	// string-like single line delimiters
	const line_trait *ln_traits;	// whole line highlighted
	uchar wordgr_cnt;	// lengths together for padding
	uchar delim_cnt;
	uchar lntrait_cnt;

	uchar comm_clen;	// closing length
	const char *comm_op;	// multi-line comment open
	const char *comm_cl;	// multi-line comment close
};

// C
static const word_group c_words[] = {
{
	"bool""char""const""double""enum""float""int""int16_t""int32_t""int64_t""long""short"
	"signed""size_t""uchar""uint""uint16_t""uint32_t""uint64_t""uint8_t""ulong""unsigned""ushort""void",
	(const uchar[]){4, 8, 13, 19, 23, 28, 31, 38, 45, 52, 56, 61, 67, 73, 78, 82, 90, 98, 106, 113, 118, 126, 132, 136},
	24,
	COLOR_RED
},
{
	"break""case""continue""default""do""else""extern""false""for""goto""if""inline"
	"return""sizeof""static""struct""switch""true""while",
	(const uchar[]){5, 9, 17, 24, 26, 30, 36, 41, 44, 48, 50, 56, 62, 68, 74, 80, 86, 90, 95},
	19,
	COLOR_BLUE
},
{
	"!%&*+-/:<=>?[]^|~", // memchr is better but this is versatile
	(const uchar[]){1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17},
	17,
	COLOR_YELLOW
}
};

static const line_trait c_lntraits[] = {
	{"#", 1, COLOR_CYAN},
	{"//", 2, COLOR_GREEN}
};

static const delim c_delims[] = {
	{"'", 1, COLOR_MAGENTA},
	{"\"", 1, COLOR_MAGENTA}
};

static const lang_t lang_c = {
	c_words, c_delims, c_lntraits, 3, 2, 2,
	2, "/*", "*/"
};

static const delim m_delims[] = {"=", 1, COLOR_YELLOW};
static const line_trait m_lntraits[] = {"#", 1, COLOR_GREEN};
static const lang_t lang_make = {
	nullptr, m_delims, m_lntraits, 0, 1, 1,
	0, nullptr, nullptr
};

// default, no highlighting
static const lang_t lang_none = {};
