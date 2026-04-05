#include "../../headers/headers.h"

// a vector<uint> is 24 bytes, this is just 8
struct dynarray {
	uint *array;

	void incr_len() { array[0]++; }
	uint len() { return array[0]; }
	uint cpt() { return __bit_ceil(len()); }

	dynarray() {
		// cannot be more as __bit_ceil(0) == 1
		array = (uint*)malloc(2 * sizeof(uint));
		array[0] = 0;
	}

	void append(uint elem) {
		uint capacity = cpt();
		if (len() >= capacity)
			array = (uint*)realloc(array, (capacity * 2 + 1) * sizeof(uint));
		incr_len();
		array[len()] = elem;
	}
};
