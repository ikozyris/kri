#include "headers/sizes.h"

// convert bytes to base-10 (SI) human-readable string e.g 1000B = 1kB
char hrsize(size_t bytes, char *dest, ushort dest_cpt)
{
	const char suffix[] = {0, 'k', 'M', 'G', 'T'};
	uchar length = sizeof(suffix) / sizeof(suffix[0]), i;
	double dblBytes = bytes;

	for (i = 0; (bytes / 1000) > 0 && i < length - 1; ++i, bytes /= 1000)
		dblBytes = bytes / 1000.0;

	snprintf(dest, dest_cpt, "%.02lf %cB", dblBytes, suffix[i]);
	return suffix[i];
}

// TODO: make both functions __attrubute__ ((const)) by also returning flag
// displayed characters to bytes, flag -> disp_x where counting stopped at
uint dchar2bytes(uint disp_x, uint from, const iter *i)
{
	from += i->offset;
	uint max_len = i->len() - 1 + i->offset; // without this line's newline
	uint x = 0;
	while (x < disp_x && from < max_len)
		get_off(x, from, *i->orig);
	flag = x;
	return from - i->offset;
}

// bytes to displayed characters, flag -> bytes of x returned
uint bytes2dchar(uint bytes, uint from, const iter *i)
{
	from += i->offset;
	bytes += i->offset;
	uint x = 0;
	while (from < bytes)
		get_off(x, from, *i->orig);
	flag = from - i->offset;
	return x;
}

// count multi-byte characters in string
uint mbcnt(const char *str, uint len)
{
	uint count = 0; // multi-byte char
	for (uint i = 0; i < len && str[i] != 0; ++i)
		if (str[i] < 0)
			count++;
	return count / 2; // only 2-byte multibyte chars are supported
}

// currently on a tab; go to previous char
uint prevdchar()
{
	long prev_ofx = calc_offset_dis(x - 8, 0, &it);
	long diff = prev_ofx - ofx;
	wmove(text_win, y, x - diff - 1);
	return diff;
}
