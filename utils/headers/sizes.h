#pragma once
#include "../../headers/main.h"

char hrsize(size_t bytes, char *dest, ushort dest_cpt); // format bytes
ulong dchar2bytes(ulong disp_x, ulong from, const gap_buf &buf); // how many bytes are disp_x displayed chars
ulong bytes2dchar(ulong bytes, ulong from, const gap_buf &buf);
inline long calc_offset_dis(ulong disp_x, const gap_buf &buf) { // offset until displayed character
	return (long)dchar2bytes(disp_x, 0, buf) - (long)flag;
}
inline long calc_offset_act(ulong pos, ulong from, const gap_buf &buf) { // offset from i until byte pos
	long i = bytes2dchar(pos, from, buf);
	return (long)flag - i;
}
// helper function for calc_offset_[dis|act](), dchar2bytes()
inline void get_off(ulong &x, ulong &i, const gap_buf &buf)
{
	char ch = at(buf, i);
	if (ch == '\t')
		x += 8 - x % 8 - 1; // -1 due to x++ at end
	else if (ch < 0)
		i++; // assumes this utf8 code point is 2 bytes
	x++;
	i++;
}

ulong mbcnt(const char *str, ulong len); // count multi-byte characters in string
uint prevdchar(); // left arrow on end of tab; update offset and move cursor
