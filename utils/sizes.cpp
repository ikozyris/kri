#include "headers/sizes.h"

// convert bytes to base-10 (SI) human-readable string e.g 1000B = 1kB
char hrsize(size_t bytes, char *dest, ushort dest_cpt)
{
	const char suffix[] = {0, 'k', 'M', 'G', 'T'};
	uchar length = sizeof(suffix) / sizeof(suffix[0]);

	double dblBytes = bytes;

	uchar i;
	for (i = 0; (bytes / 1000) > 0 && i < length - 1; ++i, bytes /= 1000)
		dblBytes = bytes / 1000.0;

	snprintf(dest, dest_cpt, "%.02lf %cB", dblBytes, suffix[i]);
	return suffix[i];
}

// helper function for calc_offset_[dis|act](), dchar2bytes()
static void get_off(ulong &x, ulong &i, const gap_buf &buf)
{
	char ch = at(buf, i);
	if (ch == '\t')
		x += 8 - x % 8 - 1; // -1 due to x++ at end
	else if (ch < 0)
		i++; // assumes this utf8 code point is 2 bytes
	x++;
	i++;
}

// displayed characters to bytes, flag -> disp_x where counting stopped at
ulong dchar2bytes(ulong disp_x, ulong from, const gap_buf &buf)
{
	ulong x = 0;
	while (x < disp_x && from < buf.len())
		get_off(x, from, buf);
	flag = x;
	return from;
}

// bytes to displayed characters, flag -> bytes of x returned
ulong bytes2dchar(ulong bytes, ulong from, const gap_buf &buf)
{
	ulong x = 0;
	while (from < bytes)
		get_off(x, from, buf);
	flag = from;
	return x;
}

// count multi-byte characters in string
ulong mbcnt(const char *str, ulong len)
{
	ulong count = 0; // multi-byte char
	for (ulong i = 0; i < len && str[i] != 0; ++i)
		if (str[i] < 0)
			count++;
	return count / 2; // only 2-byte multibyte chars are supported
}

// currently on a tab; go to previous char
uint prevdchar()
{
	long prev_ofx = calc_offset_dis(x - 8, *it);
	long diff = prev_ofx - ofx;
	wmove(text_win, y, x - diff - 1);
	return diff;
}
