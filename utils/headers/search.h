#pragma once
#include "io.h"
#include "key_func.h"

vector<unsigned> bm_search(const gap_buf &buf, const char *str, unsigned short len);
vector<unsigned> search(const gap_buf &buf, const char *str, unsigned short len); // search for str in buf, return vector of <position, color(position's)>
void find(const char *str);
