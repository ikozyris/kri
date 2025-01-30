#pragma once
#include "init.h"

extern bool eligible; // is syntax highlighting enabled
bool isc(const char *str);
void highlight(uint y); // highlight line y of screen if eligible
