#pragma once
#include "init.h"

extern bool eligible; // is syntax highlighting enabled
bool detect_lang(const char *str);
void scan_comments(uint target_line);
void highlight(uint line, const iter *i); // highlight line y of screen if eligible
#define clear_attrs (wmove(text_win, y, 0), wchgat(text_win, maxx - 1, 0, 0, nullptr))
