#pragma once
#include "io.h"
#include "key_func.h"
#include "../../ds/headers/dynarray.h"

void search_la(uint from, uint to);
void search_lc(uint from, uint to);
void find(const char *str, uint from, uint to, char mode); // wrapper for search_l
