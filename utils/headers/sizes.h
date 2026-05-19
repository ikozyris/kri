#pragma once
#include "../../headers/main.h"

char hrsize(size_t bytes, char *dest, ushort dest_cpt) __attribute__ ((const)); // format bytes
uint dchar2bytes(uint disp_x, uint from, const iter *i); // how many bytes are disp_x displayed chars
uint bytes2dchar(uint bytes, uint from, const iter *i);
inline long calc_offset_dis(uint disp_x, uint from, const iter *i) { // offset until displayed character
	return (long)dchar2bytes(disp_x, from, i) - (long)flag;
}
inline long calc_offset_act(uint pos, uint from, const iter *i) { // offset from i until byte pos
	long n = bytes2dchar(pos, from, i);
	return (long)flag - n;
}
uint mbcnt(const char *str, uint len) __attribute__ ((const)); // count multi-byte characters in string
uint prevdchar(); // left arrow on end of tab; update offset and move cursor
inline void get_off(uint &x, uint &i, const gap_buf &buf) { // helper function for calculating offsets
	char ch = at(buf, i);
	if (ch == '\t')
		x += 8 - x % 8 - 1; // -1 due to x++ at end
	else if (ch < 0)
		i++; // assumes this utf8 code point is 2 bytes
	x++;
	i++;
}
