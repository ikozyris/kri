#pragma once
#include "../../headers/main.h"

char hrsize(size_t bytes, char *dest, ushort dest_cpt); // format bytes
ulong dchar2bytes(ulong disp_x, ulong from, const iter *i); // how many bytes are disp_x displayed chars
ulong bytes2dchar(ulong bytes, ulong from, const iter *i);
inline long calc_offset_dis(ulong disp_x, ulong from, const iter *i) { // offset until displayed character
	return (long)dchar2bytes(disp_x, from, i) - (long)flag;
}
inline long calc_offset_act(ulong pos, ulong from, const iter *i) { // offset from i until byte pos
	long n = bytes2dchar(pos, from, i);
	return (long)flag - n;
}
ulong mbcnt(const char *str, ulong len); // count multi-byte characters in string
uint prevdchar(); // left arrow on end of tab; update offset and move cursor
