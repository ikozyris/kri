#pragma once
#include "io.h"
#include "key_func.h"

vector<uint> bm_search(const gap_buf &buf, const char *str, ushort len);
// search for str in buf, return vector of <position, color(position's)>
vector<uint> search_a(const gap_buf &buf, const char *str, ushort len);
uint search_c(const gap_buf &buf, const char *str, ushort len);
void find(const char *str, uchar mode); // wrapper searching
