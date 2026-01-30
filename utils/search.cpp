#include "headers/search.h"

// global is occurrences, each local is matches
static vector<dynarray> occurrences(4);
// shared parameters
static const char *string;
static bool append;
static iter first_line;
static iter last_line;

// TODO: give hint of left/right bound based on cur_occ
static uint ln_start(const vector<pair<uint, uint>> &yx, uint y) {
	uint lo = 0, hi = yx.size() - 1, mid;
	while (lo < hi) {
		mid = lo + (hi - lo) / 2;
		if (yx[mid].first < y)
			lo = mid + 1;
		else
			hi = mid; // find leftmost occurrence
	}
	return lo;
}

// convert index of chunk buffer to 2d position relative to the start of chunk
static pair<uint, uint> index2yx(uint index, iter *it)
{
	uint cbyte = 0;
	const chunk *ch = it->parent();
	for (uint i = 0; i < ch->num_lines; ++i) {
		if (index < cbyte + ch->len[i]) {
			uint dx = bytes2dchar(index, cbyte, it);
			if (dx >= maxx - 1) // if it's outside of visible range we don't need this
				dx = index;
			return {i, dx};
		}
		cbyte += ch->len[i];
	}
	return {index, 0}; // only one line is in chunk
}

static void highlight_occ(const vector<pair<uint,uint>> &matches, uint cur_occ)
{
	if (matches[cur_occ].first - ofy == 0) // first line may have offset on x axis
		mvwchgat(text_win, 0, matches[cur_occ].second - ofx,
			string[0], A_STANDOUT, 0, 0);
	while (cur_occ < matches.size()) {
		if (matches[cur_occ].first >= maxy + ofy)
			break;
		if (matches[cur_occ].second < maxx) // ignored on handled above
			mvwchgat(text_win, (uint)matches[cur_occ].first - ofy, matches[cur_occ].second,
				string[0], A_STANDOUT, 0, 0);
		cur_occ++;
	}
}

static void *_search_lc(void *args);
static void *_search_la(void *args);
static void search_mt_common(uint from, uint to, void *search_fn(void*));

// highlight or count occurrences of str in range [from, to]
void find(const char *str, uint from, uint to, char mode)
{
	uint str_len = str[0];
	if (str_len == 0 || to > text.lines || from > to) {
		print2header("Invalid parameters", 1);
		return;
	}

	iter tmp_it;
	point2begin(&tmp_it);
	iterate_fw(&tmp_it, from);
	it = tmp_it;
	append = mode == 'h';

	string = str;
	if (append)
		search_mt_common(from, to, _search_la);
	else
		search_mt_common(from, to, _search_lc);
	ulong total = 0;
	for (auto i : occurrences)
		total += i.len();
	clear_header();
	snprintf(lnbuf, lnbf_cpt, "%lu matches on lines [%u, %u]", total, from, to);
	print2header(lnbuf, 1);

	if (mode == 'c' || total == 0)
		return;
	str++;
	str_len -= mbcnt(str, str_len); // get displayed characters

	bool after_first_chunk = first_line.parent() == text.head->next;
	uint cline = after_first_chunk ? 0 : from - 1; // cumulative line up to previous chunk
	vector<pair<uint,uint>> matches; // y,x
	for (uint i = 0; i < occurrences.size(); ++i) { // occurrences in each chunk
		for (uint j = 0; j < occurrences[i].len(); ++j) { // i.len() may be 0
			uint index = occurrences[i].array[j];
			if (i == 0)
				index += first_line.offset;
			matches.emplace_back(index2yx(index, &first_line));
			if (i != 0 || after_first_chunk == 0)
				matches.back().first += cline;
		}
		cline += first_line.parent()->num_lines;
		first_line.orig = &first_line.parent()->next->merged_lines;
		occurrences[i].set_len(0); // cleanup for next search
	}

	scroll2(matches[0].first + 1);
	uint cur_occ = 0; // current occurrence
	highlight_occ(matches, 0);

	y = 0;
	curs_set(0);
	int ch;
	while ((ch = wgetch(text_win))) {
		switch (ch) {
		case KEY_RIGHT: // next occurrence in the same line
			if (cur_occ == matches.size() - 1 || matches[cur_occ + 1].first != matches[cur_occ].first)
				break;
			cur_occ++;
			mvr_scurs(matches[cur_occ].second);
			break;

		case KEY_LEFT: // previous occurrence in the same line
			if (!cur_occ || matches[cur_occ - 1].first != matches[cur_occ].first)
				break;
			cur_occ--;
			mvl_scurs(matches[cur_occ].second);
			break;

		case KEY_DOWN: // previous occurrence in previous lines
			if (cur_occ == matches.size() - 1 || matches[cur_occ].first == matches.back().first)
				break;
			cur_occ = ln_start(matches, matches[cur_occ].first + 1);
			if (matches[cur_occ].first >= text.lines || it.global_pos + ofy - ry >= text.lines)
				break;
			scroll2(matches[cur_occ].first + 1);
			if (matches[cur_occ].second >= maxx)	
				mvr_scurs(matches[cur_occ].second); // this is dx not byte
			iterate_fw(&it, ofy - ry);
			break;

		case KEY_UP: // next occurrence in next lines
			if (matches[cur_occ].first == 0 || cur_occ == 0)
				break;
			cur_occ = ln_start(matches, matches[cur_occ].first - 1);
			if (matches[cur_occ].first == ry)
				cur_occ--;
			scroll2(matches[cur_occ].first + 1);
			iterate_bw(&it, ry - ofy);
			break;

		default:
			goto exit;
		}
		highlight_occ(matches, cur_occ);
		ry = ofy;
	}
exit:
	curs_set(1);
	reset_view();
}

static void bitap_search(const uchar *buf, uint blen, uint offset, dynarray *matches)
{
	uint plen = string[0]; // pascal string
	if (blen < plen)
		return;
	const uchar *str = (const uchar*)string + 1;

	// for each byte value, 0 where the str has match
	ulong mask[256];
	memset(mask, -1, sizeof(mask));

	// for each position i in buf, bit i = 0 in mask[buf[i]]
	for (uint i = 0; i < plen; i++)
		mask[str[i]] &= ~(1ul << i);

	ulong state = ~1; // no matches
	ulong accept_bit = 1ul << plen;

	for (uint i = 0; i < blen; i++) {
		state = (state | mask[buf[i]]) << 1ul;

		// matched all plen bits in sequence
		if ((state & accept_bit) == 0) {
			if (append)
				matches->append(i + 1 + offset - plen);
			else
				matches->incr_len();
		}
	}
}

// find hello in he_llo given gap length when an occurrence may be split between gap start and gap end
static void mid_search(const gap_buf *gbuf, dynarray *matches)
{
        uint gaplen = gaplen(*gbuf);
	const char *buf = gbuf->buffer();
	const char *str = string + 1;

	for (uint i = gbuf->gps - string[0] + 1; i < gbuf->gps; ++i) {
		int a, b = 1;
		// start by comparing the first bytes up to the gap start ("he")
		a = memcmp(buf + i, str, gbuf->gps - i);
		if (!a) // if those match then compare the rest ("llo")
			b = memcmp(buf + gbuf->gps + gaplen, str + gbuf->gps - i, string[0] - (gbuf->gps - i));

		if (a == 0 && b == 0) {
			matches->append(i);
			return; // only one match can fit between the gap
		}
	}
}

// search in range [from, to]
static void ranged_searchstr(dynarray *matches, const gap_buf *buf, uint from, uint to)
{
	if (string[0] >= buf->len())
		return;
	uint from1, from2, to1, to2;
	prepare_iteration(buf, from, to, from1, to1, from2, to2);
	bitap_search((uchar*)buf->buffer() + from1, to1 - from1, 0, matches);
	if (from2 > to1 && to2 - from2 >= string[0]) {
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
	chunk *a = last_line.parent();
	uint dist = to - from;
	// inlined iterate_fw with counting chunks
	if (dist + last_line.relative_pos >= a->num_lines) { // go to next chunk
		dist += last_line.relative_pos;
		do {
			dist -= a->num_lines;
			a = a->next;
			num_chunks++;
		} while (dist > a->num_lines);
		point2chunk(&last_line, a);
	}
	line_offset(&last_line, dist);

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

	uint nthreads = num_chunks < 256 ? 1 : sysconf(_SC_NPROCESSORS_ONLN);
	occurrences.resize(append ? num_chunks : nthreads);
	num_chunks--; // first and last chunk are processed independently
	uint chunk_sz = num_chunks / nthreads;
	uint remainder = num_chunks % nthreads;

	pthread_t *threads = (pthread_t*)malloc(nthreads * sizeof(pthread_t));
	struct args_str *args = (struct args_str*)malloc(nthreads * sizeof(struct args_str));
	for (uint i = 0; i < nthreads; ++i) {
		uint st = i * chunk_sz;
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
	ranged_searchstr(&occurrences[num_chunks], last_line.orig, 0, last_line.offset + last_line.len() - 1);

	for (uint i = 0; i < nthreads; ++i)
		pthread_join(threads[i], nullptr);
	free(threads);
	free(args);
}

// each thread searches on chunk of nodes with this
static void *_search_lc(void *args)
{
	struct args_str *a = (struct args_str*)args;
	uint count = 0;

	for (uint i = 0; i < a->count; ++i) {
		occurrences[a->out].set_len(0); // reset previous search
		searchstr(&occurrences[a->out], &a->lines->merged_lines);
		count += occurrences[a->out].array[0];
		a->lines = a->lines->next;
	}
	occurrences[a->out].set_len(count);
	return nullptr;
}

// each thread searches on chunk of nodes with this
static void *_search_la(void *arg)
{
	struct args_str *a = (struct args_str*)arg;

	for (uint i = 0; i < a->count; ++i) {
		occurrences[a->out].set_len(0); // reset previous search
		searchstr(&occurrences[a->out], &a->lines->merged_lines);
		a->out++; // next chunk in occurrences[]
		a->lines = a->lines->next;
	}
	return nullptr;
}
