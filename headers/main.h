#pragma once
#include "../utils/headers/gapbuffer.h"
extern list<gap_buf> text;
extern list<gap_buf>::iterator it;

// displayed characters of previous cut (or cut|slice of line)
#define dchar first
// bytes printed in current cut
#define byte second
extern vector<pair<uint, uint>> cut;
extern vector<bool> overflows;

extern WINDOW *header_win, *ln_win, *text_win;
extern wchar_t s[4];
extern char s2[4];
extern cchar_t mark;
extern uint flag; // actual x processed by offset funcs in size.cpp
extern uint y, x, len;
extern uint maxy, maxx; // to store the maximum rows and columns
extern long ofy; // offset in y axis of text and screen, x axis is in gapbuffer
extern uint ry, rx; // x, y positions in buffer/list
extern char *filename; // name of open file, if none, it isn't malloc'ed 
extern uint curnum; // total lines counter
