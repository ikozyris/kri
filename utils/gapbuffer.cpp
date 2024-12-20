#include "headers/gapbuffer.h"

long signed ofx;
char *lnbuf;
unsigned lnbf_cpt;

void init(gap_buf &a)
{
	a.buffer = (char*)malloc(array_size);
	a.len = a.gps = 0;
	a.gpe = array_size - 1;
	a.cpt = array_size;
}

void resize(gap_buf &a, unsigned size)
{
	a.buffer = (char*)realloc(a.buffer, size);
	if (a.gpe < a.cpt - 1) { // copy after gap (newline + more)
	memmove(a.buffer + size - a.len + a.gps, a.buffer + a.gpe + 1, a.len - a.gps);
		a.gpe = size - a.len + a.gps - 1;
	} else
		a.gpe = size - 1;
	a.cpt = size;
}

void mv_curs(gap_buf &a, unsigned pos)
{
	if (a.gps == a.gpe) [[unlikely]]
		resize(a, __bit_ceil(a.cpt + 1));
	if (pos > a.gps) // move gap to right
		memmove(a.buffer + a.gps, a.buffer + a.gpe + 1, pos - a.gps);
	else if (pos < a.gps) // move gap to left
		memmove(a.buffer + pos + gaplen(a), a.buffer + pos, a.gps - pos);
	if (pos >= a.len)
		a.gpe = a.cpt - 1;
	else
		a.gpe += pos - a.gps;
	a.gps = pos;
}

void insert_c(gap_buf &a, unsigned pos, char ch)
{
	if (a.len == a.cpt) [[unlikely]]
		resize(a, __bit_ceil(a.cpt + 1));
	if (ingap(a, pos)) { [[likely]]
		a[pos] = ch;
		++a.gps;
	} else {
		mv_curs(a, pos);
		a[pos] = ch;
	}
	++a.len;
}

void insert_s(gap_buf &a, unsigned pos, const char *str, unsigned len)
{
	// TODO: are both checks needed? (checked indirectly?)
	if (a.len + len >= a.cpt) [[unlikely]]
		resize(a, __bit_ceil(a.len + len + 2));
	if (gaplen(a) <= len)
		mv_curs(a, pos);
	memcpy(a.buffer + pos, str, len);
	a.len += len;
	a.gps += len;
}

void apnd_c(gap_buf &a, char ch)
{
	if (a.len >= a.cpt) [[unlikely]]
		resize(a, __bit_ceil(a.cpt + 1));
	a[a.len] = ch;
	++a.len;
	++a.gps;
}

void apnd_s(gap_buf &a, const char *str, unsigned size)
{
	if (a.len + size >= a.cpt) [[unlikely]]
		resize(a, __bit_ceil(a.len + size + 2));
	memcpy(a.buffer + a.len, str, size);
	a.len += size;
	a.gps = a.len;
}

void apnd_s(gap_buf &a, const char *str)
{
	unsigned i = a.len;
	while (str[i - a.len] != 0) {
		a[i] = str[i - a.len];
		if (++i == a.cpt)
			resize(a, a.cpt * 2); 
	}
	a.len += i - a.len;
	a.gps = a.len;
}

void eras(gap_buf &a)
{
	if (a[a.gps - 1] < 0) { // unicode
		--a.gps;
		--a.len;
		--ofx; // assumes this UTF-8 point is 2 bytes
	}
	--a.gps;
	--a.len;
}

#define error_check {\
	if (src.len == 0 || from == to)\
		return lnbuf[0] = 0;\
	/* error checking and recovery */\
	if (from > src.len)\
		from = 0;\
	if (to < from || to > src.len)\
		to = src.len;\
	if (lnbf_cpt < to - from + 1) {\
		free(lnbuf);\
		lnbf_cpt = __bit_ceil(to - from + 1);\
		lnbuf = (char*)malloc(lnbf_cpt);\
	}\
}

// TODO: this is a mess
// NOTE: destination buffer is lnbuf
// extract data from src buffer, returns length extracted (to - from)
unsigned data(const gap_buf &src, unsigned from, unsigned to)
{
	error_check;
	// try some special cases where 1 copy is required
	if (src.gps == src.len && src.gpe == src.cpt - 1) // gap ends at end
		memcpy(lnbuf, src.buffer + from, min(to - from, src.gps));
	else if (src.gps == 0) // x = 0; gap at start
		memcpy(lnbuf, src.buffer + from + src.gpe + 1, min(to, src.len) - from);
	else {
		if (from < src.gps) {
			memcpy(lnbuf, src.buffer + from, min(to, src.gps) - from);
			if (to > src.gps)
				memcpy(lnbuf + src.gps - from, src.buffer + src.gpe + 1, to - src.gps);
		} else
			memcpy(lnbuf, src.buffer + from + gaplen(src), to - from);
	}
	lnbuf[to - from] = 0;
	return to - from;
}

// returns character at pos keeping in mind the gap
char at(const gap_buf &src, unsigned pos)
{
	if (pos >= src.len)
		return 0;
	if (pos >= src.gps)
		return src[pos + gaplen(src)];
	return src[pos];
}

unsigned data2(const gap_buf &src, unsigned from, unsigned to)
{
	error_check;
	for (unsigned i = from; i < to; ++i)
		lnbuf[i - from] = at(src, i);
	return to - from;
}

// shrink buffers to just fit line (reduce RAM usage)
unsigned shrink(gap_buf &a)
{
	unsigned bytes = a.cpt;
	mv_curs(a, a.len);
	resize(a, a.len + 2);
	return bytes - a.cpt;
}	
