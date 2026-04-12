#pragma once
#include "../ds/headers/merged_unrolled-list.h"
extern llist text;
extern iter it;

struct cut_s {
	uint dchar; // displayed characters of previous cut (or cut|slice of line)
	uint byte; // bytes printed in current cut
};
extern vector<cut_s> cut;
extern vector<bool> overflows;

extern WINDOW *header_win, *ln_win, *text_win;
extern wchar_t s[4];
extern char s2[4];
extern cchar_t mark;
extern uint flag; // displayed x processed by offset funcs in size.cpp
extern uint y, x;
extern uint maxy, maxx; // to store the maximum rows and columns
extern uint ofy; // offset in y axis of text and screen, x axis is in gapbuffer
extern uint ry, rx; // x, y positions in buffer/list
extern char *filename; // name of open file, if none, it isn't malloc'ed 
