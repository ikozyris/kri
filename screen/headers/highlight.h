#pragma once
#include "init.h"
#include "../../utils/headers/search.h"

extern bool eligible; // is syntax highlighting enabled
bool isc(const char *str);
void highlight(uint line, const gap_buf &buffer); // highlight line y of screen if eligible
#define clear_attrs (wmove(text_win, y, 0), wchgat(text_win, maxx - 1, 0, 0, nullptr))
