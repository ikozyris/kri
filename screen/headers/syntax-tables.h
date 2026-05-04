typedef unsigned char uchar;
#define nelems(x) (sizeof(x) / sizeof((x)[0]))
// each array and its element length has to be sorted (for binary search)

struct lang_t {
	const char *types;	// concatenated sorted type strings
	const uchar *types_len;	// prefix sum lengths
	uchar types_cnt;

	const char *keywords;
	const uchar *keywords_len;
	uchar keywords_cnt;

	const char *oper;
	uchar oper_sz;

	const char *coms;	// single-line comment prefix
	const char *comm;	// multi-line comment open
	const char *comm_cl;	// multi-line comment close
	uchar comm_clen;	// closing length

	char preproc;	// preprocessor char (e.g. '#' for C)

	bool has_strings;	// supports string literals
};

// C
static const char ctypes[] = {"bool""char""const""double""enum""float""int""int16_t""int32_t""int64_t""long""short"
	"signed""size_t""uchar""uint""uint16_t""uint32_t""uint64_t""uint8_t""ulong""unsigned""ushort""void"};
// prefix sum of array
static uchar ctypes_len[] = {4, 8, 13, 19, 23, 28, 31, 38, 45, 52, 56, 61, 67, 73, 78, 82, 90, 98, 106, 113, 118, 126, 132, 136};
static const char ckeywords[] = {"break""case""continue""default""do""else""extern""false""for""goto""if""inline"
	"return""sizeof""static""struct""switch""true""while"};
static uchar ckeywords_len[] = {5, 9, 17, 24, 26, 30, 36, 41, 44, 48, 50, 56, 62, 68, 74, 80, 86, 90, 95};
static const char coper[] = {'!', '%', '&', '*', '+', '-', '/', ':', '<', '=', '>', '?', '[', ']', '^', '|', '~', 0};

static const lang_t lang_c = {
	ctypes, ctypes_len, nelems(ctypes_len),
	ckeywords, ckeywords_len, nelems(ckeywords_len),
	coper, nelems(coper),
	"//", "/*", "*/", 2,
	'#', true
};

// Makefile
static const char moper[] = {'=', 0};

static const lang_t lang_make = {
	nullptr, nullptr, 0,
	nullptr, nullptr, 0,
	moper, nelems(moper),
	"#", nullptr, nullptr, 0,
	0, false
};

// default, no highlighting
static const lang_t lang_none = {};
