#include "headers/gapbuffer.h"

long ofx;
char *lnbuf;
uint lnbf_cpt;

void init(gap_buf &a)
{
	a.gps = 0;
	a.ptr = 0x300000000000000;
	a.gpe = array_size - 1;
	char *buf = (char*)malloc(array_size);
	a.set_buf(buf);
}

void _resize_common(gap_buf &a, uint nsz, uint psz)
{
	char *buffer = a.buffer();
	buffer = (char*)realloc(buffer, nsz);
	a.set_buf(buffer);
	if (a.gpe < psz - 1) { // copy after gap (newline + more)
		memmove(a.buffer() + nsz - psz + a.gpe + 1, a.buffer() + a.gpe + 1, psz - a.gpe - 1);
		a.gpe = nsz - psz + a.gpe;
	} else
		a.gpe = nsz - 1;
}

void resize2fit(gap_buf &a, uint sz)
{
	uint target;
	if (sz > 29 * 0x1000000)
		target = sz / 0x1000000;
	else
		target = log2(sz);
	_resize_common(a, sz, a.cpt());
	a.set_cpt(target);
}

void resize_up(gap_buf &a)
{
	uint psz = a.cpt();
	a.incr_cpt();
	uint nsz = a.cpt();
	_resize_common(a, nsz, psz);
}

void mv_curs(gap_buf &a, uint pos)
{
	if (a.gps >= a.gpe + 1) [[unlikely]]
		resize_up(a);
	if (pos > a.gps) // move gap to right
		memmove(a.buffer() + a.gps, a.buffer() + a.gpe + 1, pos - a.gps);
	else if (pos < a.gps) // move gap to left
		memmove(a.buffer() + pos + gaplen(a), a.buffer() + pos, a.gps - pos);
	if (pos >= a.len())
		a.gpe = a.cpt() - 1;
	else
		a.gpe = a.gpe + pos - a.gps;
	a.gps = pos;
}

void insert_c(gap_buf &a, char ch)
{
	if (a.gps >= a.gpe) [[unlikely]]
		resize_up(a);
	a[a.gps] = ch;
	a.gps++;
}

void insert_s(gap_buf &a, const char *str, uint len)
{
	if (a.gps + len >= a.gpe + 1) [[unlikely]]
		resize2fit(a, __bit_ceil(a.len() + len + 2));
	memcpy(a.buffer() + a.gps, str, len);
	a.gps += len;
}

void apnd_c(gap_buf &a, char ch)
{
	if (a.gps >= a.gpe + 1) [[unlikely]]
		resize_up(a);
	a[a.len()] = ch;
	a.gps++;
}
void apnd_s(gap_buf &a, const char *str, uint size)
{
	if (a.gps + size >= a.cpt()) [[unlikely]]
		resize2fit(a, __bit_ceil(a.len() + size + 2));
	memcpy(a.buffer() + a.gps, str, size);
	a.gps += size;
}

void apnd_s(gap_buf &a, const char *str)
{
	uint i = a.len();
	while (str[i - a.len()] != 0) {
		a[i] = str[i - a.len()];
		if (++i == a.cpt())
			resize_up(a); 
	}
	a.gps += i;
}

void eras(gap_buf &a)
{
	if (a[a.gps - 1] < 0) { // unicode
		a.gps--;
		--ofx; // assumes this UTF-8 point is 2 bytes
	}
	a.gps--;
}

#define error_check {\
	if (src.len() == 0 || from == to)\
		return lnbuf[0] = 0;\
	/* error checking and recovery */\
	if (from > src.len())\
		from = 0;\
	if (to < from || to > src.len())\
		to = src.len();\
	if (lnbf_cpt < to - from + 1) {\
		free(lnbuf);\
		lnbf_cpt = __bit_ceil(to - from + 1);\
		lnbuf = (char*)malloc(lnbf_cpt);\
	}\
}

// TODO: this is a mess
// NOTE: destination buffer is lnbuf
// extract data from src buffer, returns length extracted (to - from)
uint data(const gap_buf &src, uint from, uint to)
{
	error_check;
	// try some special cases where 1 copy is required
	if (src.gpe >= src.cpt() - 1) // gap ends at end
		memcpy(lnbuf, src.buffer() + from, min(to - from, src.gps));
	else if (src.gps == 0) // x = 0; gap at start
		memcpy(lnbuf, src.buffer() + from + src.gpe + 1, min(to, src.len()) - from);
	else {
		if (from < src.gps) {
			memcpy(lnbuf, src.buffer() + from, min(to, src.gps) - from);
			if (to > src.gps)
				memcpy(lnbuf + src.gps - from, src.buffer() + src.gpe + 1, to - src.gps);
		} else
			memcpy(lnbuf, src.buffer() + from + gaplen(src), to - from);
	}
	lnbuf[to - from] = 0;
	return to - from;
}

// returns character at pos keeping in mind the gap
char at(const gap_buf &src, uint pos)
{
	if (pos >= src.gps)
		pos += gaplen(src);
	return src[pos];
}

// to iterate in range [from, to) over the gap buffer on positions containg actual data, 2 loops may be needed to skip the gap
void prepare_iteration(const gap_buf *src, uint from, uint to, uint &st1, uint &end1, uint &st2, uint &end2)
{
	st1 = from; end1 = to; st2 = end2 = 0; // range before gap; 1 it (default)
	const uint t_gpe = src->gpe + 1;
	if (from >= src->gps) { // range is after gap; 1 iteration needed
		st1 = t_gpe + from;
		end1 = t_gpe + to;
	} else if (to >= src->gps) { // range starts before gap and ends after; 2 it
		end1 = src->gps;
		st2 = t_gpe;
		end2 = t_gpe + to - from - 1;
	}
}
