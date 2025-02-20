#pragma once
#include "io.h"
#include "key_func.h"

// search for str in buf, return vector of occurences
vector<uint> search_a(const gap_buf &buf, const char *str, ushort len);
uint search_c(const gap_buf &buf, const char *str, ushort len); // same as above, but only counts
void find(const char *str, uchar mode); // wrapper searching
