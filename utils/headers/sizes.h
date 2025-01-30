#pragma once
#include "../../headers/main.h"

char hrsize(size_t bytes, char *dest, ushort dest_cpt); // format bytes
uint memusg(); // memory usage (PSS) of kri in kilobytes
uint whereis(const char *str, char ch); // position of ch in str (wrapper for strch)
long calc_offset_dis(uint dx, const gap_buf &buf); // offset until displayed character
long calc_offset_act(uint pos, uint i, const gap_buf &buf); // offset from i until byte pos
uint dchar2bytes(uint dx, uint from, const gap_buf &buf); // how many bytes are dx displayed characters
uint mbcnt(const char *str, uint len); // count multi-byte characters in string
uint prevdchar(); // left arrow on end of tab; update offset and move cursor
