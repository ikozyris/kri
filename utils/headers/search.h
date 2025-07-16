#pragma once
#include "io.h"
#include "key_func.h"

// search for str in buf, return vector of occurences
vector<uint> search_a(const gap_buf &buf, const char *str, ushort len);
vector<vector<uint>> search_la(uint from, uint to, const char *str, ushort len); // use above to search on multiple lines
uint search_c(const gap_buf &buf, const char *str, ushort len); // same as above, but only counts
uint search_lc(uint from, uint to, const char *str, ushort len);
vector<uint> mt_search(const gap_buf &buf, char ch, bool append);
void find(const char *str, uint from, uint to, char mode); // wrapper for search_l
