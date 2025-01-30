#pragma once
#include "../../screen/headers/highlight.h"

void print2header(const char *msg, uchar pos); // print string to header; pos = 1left 3right else center
char *input_header(const char *q); // return allocated 128 byte string, answer to question q, posted in header
#define clearline (wmove(text_win, y, 0), wclrtoeol(text_win))
#define mvprint_line(y, x, buffer, from, to) (wmove(text_win, y, x), print_line(buffer, from, to, y))
uint print_line(const gap_buf &buffer, uint from, uint to, uint y); // print substring of buffer, to = 0 -> fill line
void print_text(uint line); // print text string from line
#define clean_mark(line) (mvwaddch(text_win, (line), maxx - 1, ' '))
#define print_del_mark(line) {if (overflows[line]) {mvwins_wch(text_win, (line), maxx - 1, &mark);}}
void print_new_mark();
void save(); // save buffer to filename
void read_fgets(FILE *fi); // read file with fgets
void read_fread(FILE *fi); // read file with fread (faster)
