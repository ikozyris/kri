#pragma once
#include "../../headers/main.h"

char hrsize(size_t bytes, char *dest, unsigned short dest_cpt); // format bytes
unsigned memusg(); // memory usage (PSS) of kri in kilobytes
unsigned whereis(const char *str, char ch); // position of ch in str (wrapper for strch)
long calc_offset_dis(unsigned dx, gap_buf &buf); // offset until displayed character
long calc_offset_act(unsigned pos, unsigned i, gap_buf &buf); // offset from i until byte pos
unsigned dchar2bytes(unsigned dx, unsigned from, gap_buf &buf); // how many bytes are dx displayed characters
unsigned mbcnt(const char *str, unsigned len); // count multi-byte characters in string
unsigned prevdchar(); // left arrow on end of tab; update offset and move cursor
