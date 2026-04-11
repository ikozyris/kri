#include "headers/search.h"

// global is occurrences, each local is matches
vector<dynarray> occurrences(2);
ulong mask[256];
// shared parameters
static const char *string;
static uint str_len;
static bool append;
static iter first_line;
static iter last_line;

// TODO: give hint of left/right bound based on cur_occ
// find the first element that is >= y
static uint ln_start(const vector<match> &yx, uint y) {
	uint lo = 0, hi = yx.size() - 1, mid;
	while (lo < hi) {
		mid = lo + (hi - lo) / 2;
		if (yx[mid].y < y)
			lo = mid + 1;
		else
			hi = mid; // find leftmost occurrence
	}
	return lo;
}

// convert index of chunk buffer to 2d position relative to the start of chunk
static match index2yx(uint index, iter *it, uint &prev_byte, uint &prev_x)
{
	uint cbyte = 0;
	const chunk *ch = it->parent();
	uint ln_len = ch->len ? ch->len[0] : ch->merged_lines.len(); // for standalone lines
	uint y = 1;
	for (; y < ch->num_lines; ++y) {
		if (index < cbyte + ln_len)
			break;
		cbyte += ln_len;
		ln_len = ch->len[y];
	}
	if (prev_byte < cbyte) {
		prev_byte = cbyte;
		prev_x = 0;
	}
	prev_x += bytes2dchar(index, prev_byte, it);
	prev_byte = index;
	return {y - 1, prev_x, index - cbyte};
}

static void highlight_occ(const vector<match> &matches, uint cur_occ)
{
	if (matches[cur_occ].y - ofy == 0 && ofx != 0) { // first line may have offset on x axis
		mvwchgat(text_win, 0, matches[cur_occ].byte - ofx, str_len, A_STANDOUT, 0, 0);
		cur_occ++;
	}
	while (cur_occ < matches.size()) {
		if (matches[cur_occ].y >= maxy + ofy)
			break;
		if (matches[cur_occ].x < maxx) // ignored on handled above
			mvwchgat(text_win, (uint)matches[cur_occ].y - ofy, matches[cur_occ].x,
				str_len, A_STANDOUT, 0, 0);
		cur_occ++;
	}
}

static void *_search_lc(void *args);
static void *_search_la(void *args);
static void search_mt_common(uint from, uint to, void *search_fn(void*));

uint search(const char *str, uint len, uint from, uint to, char mode)
{
	string = str;
	str_len = len;
	append = mode == 'h';

	if (append)
		search_mt_common(from, to, _search_la);
	else
		search_mt_common(from, to, _search_lc);
	ulong total = 0;
	for (auto i : occurrences)
		total += i.len();
	return total;
}

// highlight or count occurrences of str in range [from, to]
void find(const char *str, uint from, uint to, char mode)
{
	str_len = strlen(str);
	if (!str_len || to > text.lines || from > to) {
		print2header("Invalid parameters", 1);
		return;
	}
	uint total = search(str, str_len, from, to, mode);

	clear_header();
	snprintf(lnbuf, lnbf_cpt, "%u matches on lines [%u, %u]", total, from, to);
	print2header(lnbuf, 1);

	iter tmp_it;
	point2begin(&tmp_it);
	iterate_fw(&tmp_it, from);
	it = tmp_it;

	if (mode == 'c' && total > 0)
		for (uint i = 0; i < occurrences.size(); ++i)
			occurrences[i].array[0] = 0;

	if (mode == 'c' || total == 0)
		return;
	str_len -= mbcnt(str, str_len); // get displayed characters

	bool after_first_chunk = first_line.parent() == text.head->next;
	uint cline = after_first_chunk ? 0 : from - 1; // cumulative line up to previous chunk
	vector<match> matches; // y, x, byte (in line not chunk)
	matches.reserve(total);
	for (uint i = 0; i < occurrences.size(); ++i) { // occurrences in each chunk
		uint prev_byte = 0, prev_x = 0;
		for (uint j = 1; j <= occurrences[i].len(); ++j) {
			uint index = occurrences[i].array[j];
			if (i == 0)
				index += first_line.offset;
			matches.emplace_back(index2yx(index, &first_line, prev_byte, prev_x));
			if (i != 0 || after_first_chunk == 0)
				matches.back().y += cline;
		}
		cline += first_line.parent()->num_lines;
		first_line.orig = &first_line.parent()->next->merged_lines;
		occurrences[i].array[0] = 0; // cleanup for next search
	}

	iterate_fw(&it, matches[0].y - from);
	scroll2(matches[0].y + 1);
	uint cur_occ = 0; // current occurrence
	if (matches[0].x >= maxx)
		mvr_scurs(matches[0].byte);

	y = 0;
	curs_set(0);
	int ch = 0;
	do {
		switch (ch) {
		case KEY_RIGHT: // next occurrence in the same line
			if (cur_occ == matches.size() - 1 || matches[cur_occ + 1].y != matches[cur_occ].y)
				break;
			cur_occ++;
			mvr_scurs(matches[cur_occ].byte);
			break;

		case KEY_LEFT: // previous occurrence in the same line
			if (!cur_occ || matches[cur_occ - 1].y != matches[cur_occ].y)
				break;
			cur_occ--;
			mvl_scurs(matches[cur_occ].byte);
			break;

		case KEY_DOWN: // next occurrence in following lines
			if (cur_occ == matches.size() - 1 || matches[cur_occ].y == matches.back().y)
				break;
			cur_occ = ln_start(matches, matches[cur_occ].y + 1);
			if (matches[cur_occ].y >= text.lines || it.global_pos + ofy - ry >= text.lines)
				break;
			iterate_fw(&it, matches[cur_occ].y - ry);
			scroll2(matches[cur_occ].y + 1);
			if (matches[cur_occ].x >= maxx)
				mvr_scurs(matches[cur_occ].byte);
			break;

		case KEY_UP: // previous occurrence in previous lines
			if (matches[cur_occ].y == 0 || cur_occ == 0)
				break;
			cur_occ = ln_start(matches, matches[cur_occ].y - 1);
			if (matches[cur_occ].y == ry)
				cur_occ--;
			iterate_bw(&it, ry - matches[cur_occ].y);
			scroll2(matches[cur_occ].y + 1);
			if (matches[cur_occ].x >= maxx)
				mvr_scurs(matches[cur_occ].byte);
			break;

		default:
			if (ch != 0)
				goto exit;
		}
		highlight_occ(matches, cur_occ);
		ry = ofy;
	} while ((ch = wgetch(text_win)));
exit:
	curs_set(1);
	reset_view();
}

static void init_bitap()
{
	memset(mask, -1, sizeof(mask));
	for (uint i = 0; i < str_len; i++)
		mask[(uchar)string[i]] &= ~(1ul << i);
}

static void bitap_search(const uchar *buf, uint blen, uint offset, dynarray *matches)
{
	uint plen = str_len; // pascal string
	if (blen < plen)
		return;

	ulong state = ~1; // no matches
	ulong accept_bit = 1ul << plen; // 0 for match

	if (append) { // split loops
		for (uint i = 0; i < blen; i++) {
			state = (state | mask[buf[i]]) << 1ul;
			// last bit matches
			if ((state & accept_bit) == 0)
				matches->append(i + 1 + offset - plen);
		}
	} else {
		uint count = 0;
		for (uint i = 0; i < blen; i++) {
			state = (state | mask[buf[i]]) << 1ul;
			if ((state & accept_bit) == 0)
				count++;
		}
		matches->array[0] += count;
	}
}

// find hello in he_llo given gap length when an occurrence may be split between gap start and gap end
static void mid_search(const gap_buf *gbuf, dynarray *matches)
{
        uint gaplen = gaplen(*gbuf);
	const char *buf = gbuf->buffer();

	for (uint i = gbuf->gps - str_len + 1; i < gbuf->gps; ++i) {
		int a, b = 1;
		// start by comparing the first bytes up to the gap start ("he")
		a = memcmp(buf + i, string, gbuf->gps - i);
		if (!a) // if those match then compare the rest ("llo")
			b = memcmp(buf + gbuf->gps + gaplen, string + gbuf->gps - i, str_len - (gbuf->gps - i));

		if (a == 0 && b == 0) {
			matches->append(i);
			return; // only one match can fit between the gap
		}
	}
}

// search in range [from, to]
static void ranged_searchstr(dynarray *matches, const gap_buf *buf, uint from, uint to)
{
	if (str_len >= buf->len())
		return;
	uint from1, from2, to1, to2;
	prepare_iteration(buf, from, to, from1, to1, from2, to2);
	bitap_search((uchar*)buf->buffer() + from1, to1 - from1, 0, matches);
	if (from2 > to1 && to2 - from2 >= str_len) {
		mid_search(buf, matches);
		bitap_search((uchar*)buf->buffer() + from2, to2 - from2, from2 - 2, matches);
	}
}

// search for str in buf, write in matches
static void searchstr(dynarray *matches, const gap_buf *buf)
{
	ranged_searchstr(matches, buf, 0, buf->len() - 1);
}

// arguments for each thread (shared for count/append)
struct args_str {
	chunk *lines;
	uint count; // count of chunks to process
	uint out; // index in occurences[]
};

static void search_mt_common(uint from, uint to, void *search_fn(void*))
{
	// first chunk may need an offset for start
	point2begin(&first_line);
	iterate_fw(&first_line, from);
	chunk *chi = first_line.parent();

	last_line.orig = first_line.orig;
	last_line.relative_pos = first_line.relative_pos;
	last_line.offset = first_line.offset;

	uint num_chunks = 0;
{ // inlined iterate_fw with counting chunks
	chunk *a = last_line.parent();
	uint dist = to - from;
	if (dist + last_line.relative_pos >= a->num_lines) { // go to next chunk
		dist += last_line.relative_pos;
		do {
			dist -= a->num_lines;
			a = a->next;
			num_chunks++;
		} while (dist >= a->num_lines);
		point2chunk(&last_line, a);
	}
	line_offset(&last_line, dist);
}

	init_bitap();
	if (num_chunks == 0) {
		ranged_searchstr(&occurrences[0], first_line.orig, first_line.offset, last_line.offset + last_line.len() - 1);
		return;
	}
	last_line.global_pos = to;
	chi = first_line.parent()->next;
	// it's probably better to directly call these than doing an ugly combination of goto and if
	if (num_chunks == 1) {
		ranged_searchstr(&occurrences[0], first_line.orig, first_line.offset, first_line.orig->len() - 1);
		ranged_searchstr(&occurrences[1], last_line.orig, 0, last_line.offset + last_line.len() - 1);
		return;
	}

	uint nthreads = num_chunks < 256 ? 1 : (sysconf(_SC_NPROCESSORS_ONLN) - 1);
	occurrences.resize(append ? num_chunks : nthreads + 1);
	num_chunks--; // first and last chunk are processed independently
	uint chunk_sz = num_chunks / nthreads;
	uint remainder = num_chunks % nthreads;

	pthread_t *threads = (pthread_t*)malloc(nthreads * sizeof(pthread_t));
	struct args_str *args = (struct args_str*)malloc(nthreads * sizeof(struct args_str));
	for (uint i = 0; i < nthreads; ++i) {
		uint st = i * (append ? chunk_sz : 1);
		uint size = chunk_sz + (i == nthreads - 1 ? remainder : 0);

		args[i].count = size;
		args[i].lines = chi;
		args[i].out = st + 1;

		pthread_create(&threads[i], nullptr, search_fn, &args[i]);
		while (size--)
			chi = chi->next;
	}

	// first and last lines are special cases and thus processed by main thread
	ranged_searchstr(&occurrences[0], first_line.orig, first_line.offset, first_line.orig->len() - 1);
	ranged_searchstr(&occurrences.back(), last_line.orig, 0, last_line.offset + last_line.len() - 1);

	for (uint i = 0; i < nthreads; ++i)
		pthread_join(threads[i], nullptr);
	free(threads);
	free(args);
}

// each thread searches on chunk of nodes with this
static void *_search_lc(void *args)
{
	struct args_str *a = (struct args_str*)args;

	for (uint i = 0; i < a->count; ++i) {
		searchstr(&occurrences[a->out], &a->lines->merged_lines);
		a->lines = a->lines->next;
	}
	return nullptr;
}

// each thread searches on chunk of nodes with this
static void *_search_la(void *arg)
{
	struct args_str *a = (struct args_str*)arg;

	for (uint i = 0; i < a->count; ++i) {
		occurrences[a->out].array[0] = 0; // reset previous search
		searchstr(&occurrences[a->out], &a->lines->merged_lines);
		a->out++; // next chunk in occurrences[]
		a->lines = a->lines->next;
	}
	return nullptr;
}
