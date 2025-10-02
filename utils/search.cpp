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
static void search_mb_common(uint from, uint to, void *search_fn(void*));

// highlight or count occurrences of str in range [from, to)
void find(const char *str, uint from, uint to, char mode)
{
	uint str_len = str[0];
	if (str_len == 0 || to - 1 > text.lines || from > to) {
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
		search_mb_common(from, to, _search_la);
	else
		search_mb_common(from, to, _search_lc);
	ulong total = 0;
	for (auto i : occurrences)
		total += i.len();

	clear_header();
	snprintf(lnbuf, lnbf_cpt, "%lu matches on lines [%u, %u]", total, from, to - 1);
	print2header(lnbuf, 1);

	if (mode == 'c' || total == 0)
		return;
	str++;
	str_len -= mbcnt(str, str_len); // get displayed characters

	uint cline = 0; // cumulative line
	vector<pair<uint,uint>> matches; // y,x
	for (uint i = 0; i < occurrences.size(); ++i) { // occurrences in each chunk
		for (uint j = 0; j < occurrences[i].len(); ++j) { // i.len() may be 0
			matches.emplace_back(index2yx(occurrences[i].array[j], &first_line));
			matches.back().first += cline;
		}
		append = mode == 'h';
		cline += first_line.parent()->num_lines;
		first_line.orig = &first_line.parent()->next->merged_lines;
		occurrences[i].set_len(0);
	}

	reset_view();
	scroll2(matches[0].first + 1);
	uint cur_occ = 0; // current occurrence
	highlight_occ(matches, 0);

	y = ofx = 0;
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

static void bitap_search(const uchar *buf, uint blen, dynarray *matches)
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

	ulong state = -1; // no matches
	ulong accept_bit = 1ul << plen;

	for (uint i = 0; i < blen; i++) {
		state = (state | mask[buf[i]]) << 1ul;

		// matched all plen bits in sequence
		if ((state & accept_bit) == 0) {
			if (append)
				matches->append(i + 1 - plen);
			else
				matches->incr_len();
		}
	}
}

static void mid_search(const char *buf, dynarray *matches, uint midlen)
{
	const uint plen = string[0];
	const char *str = string + 1;
	// an occurrence might be split between gap start and gap end
	const uint midpoint = plen - 1;
	// if st underflows, it becomes: 2^32-x (which is always) > gps, so loop never gets executed
	for (uint i = 0; i < midpoint; ++i) {
		bool a, b;
		a = strncmp(buf + i, str, midpoint - i);
		if (a)
			b = strncmp(buf + midpoint + midlen, str + i, midpoint + midlen - i);

		if ((a & b) == 0) {
			if (append)
				matches->append(i);
			else
				matches->incr_len();
			break; // only one match can fit between the gap
		}
	}
}

// search for str in buf, return vector of occurrences
static void searchstr(dynarray *matches, const gap_buf *buf)
{
	if (string[0] >= buf->len())
		return;

	uint end1 = buf->gps; // for clarity
	uint st2 = buf->gpe + 1, end2 = buf->cpt();

	bitap_search((uchar*)buf->buffer(), end1, matches);
	if (end2 - st2 > 1) // if only 1 char is left it is the newline
		mid_search(buf->buffer() + end1 + 1 - string[0], matches, st2 - end1);
	bitap_search((uchar*)buf->buffer() + st2, end2 - st2, matches);
}

static void ranged_searchstr(dynarray *matches, const gap_buf *buf, uint from, uint to)
{
	uint from1, from2, to1, to2;
	prepare_iteration(buf, from, to, from1, to1, from2, to2);
	bitap_search((uchar*)buf->buffer() + from1, to1 - from1, matches);
	if (to2 > from2) {
		mid_search(buf->buffer(), matches, to2 - to1 + 1);
		bitap_search((uchar*)buf->buffer() + from2, to2 - from2, matches);
	}
}

// String search

// arguments for each thread (shared for count/append)
struct args_str {
	chunk *lines;
	uint count; // count of chunks to process
	uint out; // array of matches or pointer to count
};

static void search_mb_common(uint from, uint to, void *search_fn(void*))
{
	// first chunk may need an offset for start
	point2begin(&first_line);
	iterate_fw(&first_line, from);
	chunk *chi = first_line.parent();

	uint cur_ln = first_line.global_pos, num_chunks = 0;
	cur_ln += chi->num_lines;
	while (cur_ln < to) {
		chi = chi->next;
		cur_ln += chi->num_lines;
		num_chunks++;
	}
	// last chunk may need reduced length
	point2chunk(&last_line, chi);
	line_offset(&last_line, cur_ln - to);
	if (num_chunks == 0) {
		ranged_searchstr(&occurrences[0], first_line.orig, first_line.offset, last_line.offset + last_line.len());
		return;
	}
	last_line.global_pos = to;
	chi = first_line.parent()->next;
	// it's probably better to directly call these than doing an ugly combination of goto and if
	if (num_chunks == 1) {
		ranged_searchstr(&occurrences[0], first_line.orig, first_line.offset, first_line.orig->len());
		ranged_searchstr(&occurrences[1], last_line.orig, 0, last_line.len());
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
	ranged_searchstr(&occurrences[0], first_line.orig, first_line.offset, first_line.orig->len());
	ranged_searchstr(&occurrences[num_chunks], last_line.orig, 0, last_line.len());

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
