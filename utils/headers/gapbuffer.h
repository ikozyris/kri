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
const ulong PTR_MASK = 0xFFFFFFFFFFFF;	// 48-bits, = 2^48 - 1
const ulong GPx_MASK = 0x7FFFFFFFF;	// 35-bits, = 2^35 - 1
const ulong LOW_MASK = 0xFFFF;		// 16-bits \ 26 + 19 = 35
const ulong HIGH_MASK = 0x7FFFF;	// 19-bits /
const ulong CPT_MASK = 0x3F;		// 6-bits,  = 2^6  - 1

/* bit packing in mem:
 * 0-47: pointer	48-bit	when expanded to 56-bits: see line 18,30
 * 48-63,0-19: gap start 35-bit	max value = 32 GiB
 * 20-54: gap end	35-bit	
 * 55-61: capacity	6-bit	2^cpt => max value = 2^(2^6) = 2^64 - 1*/
struct gap_buf {
	ulong lo;
	ulong hi;

	char *buffer() const { return (char*)(lo & PTR_MASK); } // in userspace bit 47 = 0
	char &operator[](ulong pos) const { return ((char*)(lo & PTR_MASK))[pos]; }
	ulong gps() const { return ((hi & HIGH_MASK) << 16) | (lo >> 48); } // 0-based
	ulong gpe() const { return (hi >> 20) & GPx_MASK; } // 0-based
	ulong cpt() const { return pow2((hi >> 55) & CPT_MASK); } // always power of 2, 1-based
	ulong len() const { return cpt() - gaplen(*this); } // indirectly calculated, 1-based

	// zero all bits outside of mask and then apply those in the mask
	void set_buf(char *buf) { lo = (lo & ~PTR_MASK) | ((ulong)buf); }
	void set_gps(ulong st) { lo = (lo & ~(GPx_MASK << 48)) | (st << 48); 
				hi = (hi & ~HIGH_MASK) | (st >> 16); }
	void set_gpe(ulong en) { hi = (hi & ~(GPx_MASK << 20)) | (en << 20); }
	// capacity is always power of 2, only the exponent is stored
	void set_cpt(ulong cpt) { hi = (hi & ~(CPT_MASK << 55)) | ((ulong)log2(cpt) << 55); }

	gap_buf() {
		lo = hi = 0;
		set_cpt(array_size);
		set_gpe(array_size - 1);
		char *buf = (char*)malloc(array_size);
		set_buf(buf);
	}
	~gap_buf() {
		free(buffer());
		lo = hi = 0;
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
