#pragma once
#include "../../headers/headers.h"

extern long signed ofx; // offset in x-axis
extern char *lnbuf; // temporary buffer for output of data()
extern unsigned lnbf_cpt; // lnbuf capacity

#define array_size 8
#define gaplen(a) ((a).gpe() - (a).gps() + 1)
#define ingap(a, pos) (((pos) >= (a).gps() && (pos) <= (a).gpe()) ? true : false)
#define mveras(a, pos) (mv_curs(a, pos), eras(a))

inline uint64_t pow2(uint64_t n) { return 1 << n; }
inline uint64_t log2(uint64_t n) { return 32 - __builtin_clz(n) - 1; }
inline uint64_t min(uint64_t a, uint64_t b) { return a < b ? a : b; }
inline uint64_t max(uint64_t a, uint64_t b) { return a > b ? a : b; }

// pointer can also be 44 bits (3 for 8-aligned + 1 for userspace)
const __uint128_t PTR_MASK = 0xFFFFFFFFFFFF;	// first 48-bits, = 2^48 - 1
const __uint128_t GPx_MASK = 0x7FFFFFFFF;	// first 35-bits, = 2^35 - 1
const __uint128_t CPT_MASK = 0x3F;		// first 6-bits,  = 2^6  - 1

/* bit packing in mem:
 * 0-47: pointer	48-bit	when expanded to 56-bits: see line 18,28
 * 48-82: gap start	35-bit	max value = 2^35 - 1
 * 83-117: gap end	35-bit	
 * 118-124: capacity	6-bit	2^cpt => max value = 2^(2^6) = 2^64 - 1
 * 124-127: flags	4-bit	(reserved for future use)*/
struct gap_buf {
	__uint128_t mem;

	char *buffer() { return (char*)(mem & PTR_MASK); } // in userspace bit 47 := 0
	char &operator[](unsigned pos) const {
		return ((char*)(mem & PTR_MASK))[pos];
	}
	uint64_t gps() { return (mem >> 48) & GPx_MASK; } // 0-based
	uint64_t gpe() { return (mem >> 83) & GPx_MASK; } // 0-based
	uint64_t cpt() { return pow2((mem >> 118) & CPT_MASK); } // always power of 2, 1-based
	uint64_t len() { return cpt() - gaplen(*this); } // indirectly calculated, 1-based

	// zero all bits outside of mask and then apply those who are in the mask
	void set_buf(char *buf) { mem = (mem & ~PTR_MASK) | ((uint64_t)buf); }
	void set_gps(uint64_t st) { mem = ((mem & ~(GPx_MASK << 48)) | ((__uint128_t)st << 48)); }
	void set_gpe(uint64_t en) { mem = ((mem & ~(GPx_MASK << 83)) | ((__uint128_t)en << 83)); }
	// capacity is always power of 2, only the exponent is stored
	void set_cpt(uint64_t cpt) { mem = ((mem & ~(CPT_MASK << 118)) | ((__uint128_t)(log2(cpt)) << 118)); }

	gap_buf() {
		mem = 0;
		set_cpt(array_size);
		set_gpe(array_size - 1);
		char *buf = (char*)malloc(array_size);
		set_buf(buf);
	}
	~gap_buf() {
		free(buffer());
		mem = 0;
	}
};

void init(gap_buf &a); // initialize the gap buffer (should already be called by constructor)
void resize(gap_buf &a, unsigned size); // resize the buffer
void mv_curs(gap_buf &a, unsigned pos); // move the cursor to position
void insert_c(gap_buf &a, unsigned pos, char ch); // insert character at position
void insert_s(gap_buf &a, unsigned pos, const char *str, unsigned len); // insert string with given length at pos
void apnd_c(gap_buf &a, char ch); // append character
void apnd_s(gap_buf &a, const char *str, unsigned size); // append string with given size
void apnd_s(gap_buf &a, const char *str); // append null-terminated string
void eras(gap_buf &a); // erase the character at current cursor position
unsigned data(gap_buf &src, unsigned from, unsigned to); // copy buffer with range to lnbuf
char at(gap_buf &src, unsigned pos); // return character at position calculating the gap
unsigned data2(gap_buf &src, unsigned from, unsigned to); // same as data(), different implementation
unsigned shrink(gap_buf &a); // shrink buffer to just fit size (gap = 2 bytes)
