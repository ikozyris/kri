#pragma once
#include "../../headers/headers.h"

extern long ofx; // offset in x-axis
extern char *lnbuf; // temporary buffer for output of data()
extern uint lnbf_cpt; // lnbuf capacity

#define array_size 8
#define gaplen(a) ((a).gpe() - (a).gps() + 1ul)
#define ingap(a, pos) (((pos) >= (a).gps() && (pos) <= (a).gpe()) ? true : false)
#define mveras(a, pos) (mv_curs(a, pos), eras(a))

inline ulong pow2(ulong n) { return 1ul << n; }
inline uint log2(ulong n) { return 63u - __builtin_clzl(n); }
inline ulong min(ulong a, ulong b) { return a < b ? a : b; }
inline ulong max(ulong a, ulong b) { return a > b ? a : b; }

// pointer can also be 44 bits (3 for 8-aligned + 1 for userspace)
const __uint128_t PTR_MASK = 0xFFFFFFFFFFFF;	// first 48-bits, = 2^48 - 1
const __uint128_t GPx_MASK = 0x7FFFFFFFF;	// first 35-bits, = 2^35 - 1
const __uint128_t CPT_MASK = 0x3F;		// first 6-bits,  = 2^6  - 1

/* bit packing in mem:
 * 0-47: pointer	48-bit	when expanded to 56-bits: see line 18,28
 * 48-82: gap start	35-bit	max value = 32 GiB
 * 83-117: gap end	35-bit	
 * 118-124: capacity	6-bit	2^cpt => max value = 2^(2^6) = 2^64 - 1
 * 124-127: flags	4-bit	(reserved for future use)*/
struct gap_buf {
	__uint128_t mem;

	char *buffer() const { return (char*)(mem & PTR_MASK); } // in userspace bit 47 = 0
	char &operator[](ulong pos) const {
		return ((char*)(mem & PTR_MASK))[pos];
	}
	ulong gps() const { return (mem >> 48) & GPx_MASK; } // 0-based
	ulong gpe() const { return (mem >> 83) & GPx_MASK; } // 0-based
	ulong cpt() const { return pow2((mem >> 118) & CPT_MASK); } // always power of 2, 1-based
	ulong len() const { return cpt() - gaplen(*this); } // indirectly calculated, 1-based
	//uchar flag(uchar bit) { return (mem >> 124) & 4; }

	// zero all bits outside of mask and then apply those who are in the mask
	void set_buf(char *buf) { mem = (mem & ~PTR_MASK) | ((ulong)buf); }
	void set_gps(ulong st) { mem = ((mem & ~(GPx_MASK << 48)) | ((__uint128_t)st << 48)); }
	void set_gpe(ulong en) { mem = ((mem & ~(GPx_MASK << 83)) | ((__uint128_t)en << 83)); }
	// capacity is always power of 2, only the exponent is stored
	void set_cpt(ulong cpt) { mem = ((mem & ~(CPT_MASK << 118)) | ((__uint128_t)(log2(cpt)) << 118)); }
	//void set_flag(bool flag, uchar bit) { mem = ((mem & ~(1 << (124+bit)))) | ((__uint128_t)flag << (124+bit)); }

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
void resize(gap_buf &a, ulong size); // resize the buffer
void mv_curs(gap_buf &a, ulong pos); // move the cursor to position
void insert_c(gap_buf &a, ulong pos, char ch); // insert character at position
void insert_s(gap_buf &a, ulong pos, const char *str, ulong len); // insert string with given length at pos
void apnd_c(gap_buf &a, char ch); // append character
void apnd_s(gap_buf &a, const char *str, ulong size); // append string with given size
void apnd_s(gap_buf &a, const char *str); // append null-terminated string
void eras(gap_buf &a); // erase the character at current cursor position
ulong data(const gap_buf &src, ulong from, ulong to); // copy buffer with range to lnbuf
char at(const gap_buf &src, ulong pos); // return character at position calculating the gap
void prepare_iteration(const gap_buf &src, ulong from, ulong to, ulong &st1, ulong &end1, ulong &st2, ulong &end2);
