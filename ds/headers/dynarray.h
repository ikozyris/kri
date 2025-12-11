#include "../../headers/headers.h"

// a vector<uint> is 24 bytes, this is just 8
struct dynarray {
	uint *array;

	void set_len(uint n) { (*(array - 1)) = n; }
	void incr_len() { (*(array - 1))++; }
	uint len() { return *(array - 1); }
	uint cpt() { return __bit_ceil(len()); }

	dynarray() {
		// cannot be more as __bit_ceil(0) == 1
		array = (uint*)malloc(2 * sizeof(uint));
		array[0] = 0;
		array++;
	}

	void append(uint elem) {
		uint length = len();
		uint capacity = cpt();
		if (length >= capacity)
			array = (uint*)realloc(array - 1, (capacity * 2 + 1) * sizeof(uint)) + 1;
		array[length] = elem;
		incr_len();
	}
};
