#pragma once
#include "../../headers/main.h"

char hrsize(size_t bytes, char *dest, ushort dest_cpt); // format bytes
ulong memusg(); // memory usage (PSS) of kri in kilobytes
ulong whereis(const char *str, char ch); // position of ch in str (wrapper for strch)
ulong dchar2bytes(ulong disp_x, ulong from, const gap_buf &buf); // how many bytes are disp_x displayed chars
ulong bytes2dchar(ulong bytes, ulong from, const gap_buf &buf);
inline long calc_offset_dis(ulong disp_x, const gap_buf &buf) { // offset until displayed character
	return (long)dchar2bytes(disp_x, 0, buf) - (long)flag;
}
inline long calc_offset_act(ulong pos, ulong from, const gap_buf &buf) { // offset from i until byte pos
	long i = bytes2dchar(pos, from, buf);
	return (long)flag - i;
}
ulong mbcnt(const char *str, ulong len); // count multi-byte characters in string
uint prevdchar(); // left arrow on end of tab; update offset and move cursor
