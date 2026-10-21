#pragma once
#include "key_func.h"
#include "../../ds/headers/dynarray.h"

extern vector<dynarray> occurrences;
// arrays of occurrences in use by the last search, max index is occ_slots - 1
// only shrink command frees array
extern uint occ_slots;
struct match {
	uint y;
	uint x;
	uint byte;
};

uint search(const char *str, uint len, uint from, uint to, char mode); // populate occurences
void find(const char *str, uint from, uint to, char mode); // wrapper for above
