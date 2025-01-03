#pragma once
#include "io.h"
#include "key_func.h"

vector<unsigned> bm_search(gap_buf &buf, const char *str, unsigned short len);
// search for str in buf, return vector of <position, color(position's)>
vector<unsigned> search_a(gap_buf &buf, const char *str, unsigned short len);
unsigned search_c(gap_buf &buf, const char *str, unsigned short len);
void find(const char *str, unsigned char mode); // wrapper searching
