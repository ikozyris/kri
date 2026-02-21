#pragma once
#include "key_func.h"
#include "../../ds/headers/dynarray.h"

extern vector<dynarray> occurrences;
struct match {
	uint y;
	uint x;
	uint byte;
};

uint search(const char *str, uint len, uint from, uint to, char mode); // populate occurences
void find(const char *str, uint from, uint to, char mode); // wrapper for above
