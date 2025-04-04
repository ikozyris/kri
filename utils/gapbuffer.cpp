#include "headers/gapbuffer.h"

long ofx;
char *lnbuf;
uint lnbf_cpt;

void init(gap_buf &a)
{
	a.lo = a.hi = 0;
	char *buffer = (char*)malloc(array_size);
	a.set_buf(buffer);
	a.set_gpe(array_size - 1);
	a.set_cpt(array_size);
}

void resize(gap_buf &a, ulong size)
{
	char *buffer = a.buffer();
	buffer = (char*)realloc(buffer, size);
	a.set_buf(buffer);
	if (a.gpe() < a.cpt() - 1) { // copy after gap (newline + more)
		memmove(a.buffer() + size - a.cpt() + a.gpe() + 1, a.buffer() + a.gpe() + 1, a.cpt() - a.gpe() - 1);
		a.set_gpe(size - a.cpt() + a.gpe());
	} else
		a.set_gpe(size - 1);
	a.set_cpt(size);
}

void mv_curs(gap_buf &a, ulong pos)
{
	if (a.gps() >= a.gpe() + 1) [[unlikely]]
		resize(a, __bit_ceil(a.cpt() + 1));
	if (pos > a.gps()) // move gap to right
		memmove(a.buffer() + a.gps(), a.buffer() + a.gpe() + 1, pos - a.gps());
	else if (pos < a.gps()) // move gap to left
		memmove(a.buffer() + pos + gaplen(a), a.buffer() + pos, a.gps() - pos);
	if (pos >= a.len())
		a.set_gpe(a.cpt() - 1);
	else
		a.set_gpe(a.gpe() + pos - a.gps());
	a.set_gps(pos);
}

void insert_c(gap_buf &a, ulong pos, char ch)
{
	if (a.gps() >= a.gpe()) [[unlikely]]
		resize(a, __bit_ceil(a.cpt() + 1));
	if (!ingap(a, pos))
		mv_curs(a, pos);
	a[pos] = ch;
	a.set_gps(a.gps() + 1);
}

void insert_s(gap_buf &a, ulong pos, const char *str, ulong len)
{
	// TODO: are both checks needed? (checked indirectly?)
	if (a.gps() + len >= a.gpe() + 1) [[unlikely]]
		resize(a, __bit_ceil(a.len() + len + 2));
	if (gaplen(a) <= len)
		mv_curs(a, pos);
	memcpy(a.buffer() + pos, str, len);
	a.set_gps(a.gps() + len);
}

void apnd_c(gap_buf &a, char ch)
{
	if (a.gps() >= a.gpe() + 1) [[unlikely]]
		resize(a, __bit_ceil(a.cpt() + 1));
	a[a.len()] = ch;
	a.set_gps(a.gps() + 1);
}

void apnd_s(gap_buf &a, const char *str, ulong size)
{
	if (a.gps() + size >= a.cpt()) [[unlikely]]
		resize(a, __bit_ceil(a.len() + size + 2));
	memcpy(a.buffer() + a.gps(), str, size);
	a.set_gps(a.gps() + size);
}

void apnd_s(gap_buf &a, const char *str)
{
	ulong i = a.len();
	while (str[i - a.len()] != 0) {
		a[i] = str[i - a.len()];
		if (++i == a.cpt())
			resize(a, a.cpt() * 2); 
	}
	a.set_gps(i);
}

void eras(gap_buf &a)
{
	if (a[a.gps() - 1] < 0) { // unicode
		a.set_gps(a.gps() - 1);
		--ofx; // assumes this UTF-8 point is 2 bytes
	}
	a.set_gps(a.gps() - 1);
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
ulong data(const gap_buf &src, ulong from, ulong to)
{
	error_check;
	// try some special cases where 1 copy is required
	if (src.gpe() >= src.cpt() - 1) // gap ends at end
		memcpy(lnbuf, src.buffer() + from, min(to - from, src.gps()));
	else if (src.gps() == 0) // x = 0; gap at start
		memcpy(lnbuf, src.buffer() + from + src.gpe() + 1, min(to, src.len()) - from);
	else {
		if (from < src.gps()) {
			memcpy(lnbuf, src.buffer() + from, min(to, src.gps()) - from);
			if (to > src.gps())
				memcpy(lnbuf + src.gps() - from, src.buffer() + src.gpe() + 1, to - src.gps());
		} else
			memcpy(lnbuf, src.buffer() + from + gaplen(src), to - from);
	}
	lnbuf[to - from] = 0;
	return to - from;
}

// returns character at pos keeping in mind the gap
char at(const gap_buf &src, ulong pos)
{
	if (pos >= src.gps())
		pos += gaplen(src);
	return src[pos];
}

// to iterate in range [from, to) over the gap buffer on positions containg actual data, 2 loops may be needed to skip the gap
void prepare_iteration(const gap_buf &src, ulong from, ulong to, ulong &st1, ulong &end1, ulong &st2, ulong &end2)
{
	st1 = from; end1 = to; st2 = end2 = 0;
	if (from >= src.gps()) {
		st1 = gaplen(src) + from;
		end1 = gaplen(src) + to;
	} else if (from < src.gps() && to > src.gps()) {
		end1 = src.gps();
		st2 = src.gpe() + 1;
		end2 = gaplen(src) + to - from + 1 - src.gps(); // remainder
	}
}
