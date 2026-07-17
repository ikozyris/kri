#pragma once
#include "../../headers/headers.h"

extern long ofx; // offset in x-axis = bytes - dchars
extern char *lnbuf; // temporary buffer for output of data()
extern uint lnbf_cpt; // lnbuf capacity

#define array_size 8
#define gaplen(a) ((a).gpe - (a).gps + 1u)
#define ingap(a, pos) (((pos) >= (a).gps && (pos) <= (a).gpe) ? true : false)

inline uint pow2(uint n) { return 1u << n; }
inline uint log2(uint n) { return 31u - __builtin_clz(n); }
inline uint min(uint a, uint b) { return a < b ? a : b; }
inline uint max(uint a, uint b) { return a > b ? a : b; }

// pointer can also be 44 bits (3 for 8-aligned + 1 for userspace)
const ulong PTR_MASK = (1ul << 48) - 1;

struct gap_buf {
	uintptr_t ptr; // TODO: take advantage of ARM TBI / Intel LAM
	uint gps; // first gap char (gap start)
	uint gpe; // last gap char (gap end)

	char *buffer() const { return (char*)(ptr & PTR_MASK); } // in userspace bit 47 = 0
	char &operator[](uint pos) const { return ((char*)(ptr & PTR_MASK))[pos]; }
	uint cpt() const { uchar x = ptr >> 56; return (x > 29) ? (x * 0x1000000) : (1u << x); }
	uint len() const { return cpt() - gaplen(*this); } // indirectly calculated, 1-based

	// zero all bits outside of mask and then apply those in the mask
	void set_buf(char *buf) { ptr = (ptr & ~PTR_MASK) | (ulong)buf; }
	void incr_cpt() { ptr += 0x100000000000000; }
	void set_cpt(ulong n) { ptr = (ptr & PTR_MASK) | (n << 56u); } // from 0-255
};

void init(gap_buf &a); // initialize the gap buffer (should already be called by constructor)
void resize2fit(gap_buf &a, uint sz); // resize the buffer to be >= than specified size
void resize_up(gap_buf &a); // increase
void mv_curs(gap_buf &a, uint pos); // move the cursor to position
void insert_c(gap_buf &a, char ch); // insert character at cursor position
void insert_s(gap_buf &a, const char *str, uint len); // insert string with given length at cursor pos
void copy_buffer(const gap_buf &src, gap_buf &dest, uint from, uint to); // range copy gap buffer
void eras(gap_buf &a); // erase the character at current cursor position
uint data(const gap_buf &src, uint from, uint to); // copy buffer with range to lnbuf
void prepare_iteration(const gap_buf *src, uint from, uint to, uint &st1, uint &end1, uint &st2, uint &end2);
// return character at position calculating the gap
inline char at(const gap_buf &src, uint pos) {
	if (pos >= src.gps)
		pos += gaplen(src);
	return src[pos];
}
