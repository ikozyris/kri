#include "headers/search.h"

// heuristics
static uchar badchar[256];
static uint goodsuffix[256];
static void _badchar(const char *str, uchar len);
static void _goodsuffix(const char *str, uchar len);
struct match {
	uint y;
	uint x;
	uint byte;
};

// TODO: give hint of left/right bound based on cur_occ
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

static void highlight_occ(const vector<match> &matches, uint cur_occ, uint str_len)
{
	if (matches[cur_occ].y - ofy == 0 && ofx != 0) { // y line may have offset on x axis
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

// highlight or count occurrences of str in range [from, to)
void find(const char *str, uint from, uint to, char mode)
{
	uint str_len = strlen(str);
	if (str_len == 0 || str_len >= 256 || to - 1 > curnum || from > to) {
		print2header("Invalid parameters", 1);
		return;
	}

	if (str_len > 1) {
		_badchar(str, str_len);
		_goodsuffix(str, str_len);
	}

	list<gap_buf>::iterator tmp_it = text.begin();
	advance(tmp_it, from);
	it = tmp_it;
	vector<vector<uint>> occurrences;
	uint total = 0;
	if (mode == 'h') {
		occurrences = search_la(from, to, str, str_len);
		for (const auto &vec : occurrences)
			total += vec.size();
	} else
		total += search_lc(from, to, str, str_len);

	clear_header();
	snprintf(lnbuf, lnbf_cpt, "%u matches on lines [%u, %u]", total, from, to);
	print2header(lnbuf, 1);

	if (mode == 'c' || total == 0)
		return;
	str_len -= mbcnt(str, str_len); // get displayed characters

	vector<match> matches; // y, x, byte
	for (uint i = 0; i < occurrences.size(); ++i) { // occurrences in each node
		for (uint j = 0; j < occurrences[i].size(); ++j) {
			uint index = occurrences[i][j];
			matches.push_back({from + i, (uint)bytes2dchar(index, 0, *tmp_it), index});
		}
		occurrences[i].clear();
		tmp_it++;
	}
	ry = from;
	scroll2(matches[0].y + 1);
	uint cur_occ = 0; // current occurrence
	highlight_occ(matches, 0, str_len);

	y = 0;
	curs_set(0);
	int ch;
	while ((ch = wgetch(text_win))) {
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
			if (matches[cur_occ].y >= curnum || ofy >= curnum)
				break;
			advance(it, matches[cur_occ].y - ry);
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
			advance(it, (long)matches[cur_occ].y - (long)ry); // negative = back
			scroll2(matches[cur_occ].y + 1);
			if (matches[cur_occ].x >= maxx)	
				mvr_scurs(matches[cur_occ].byte);
			break;

		default:
			goto exit;
		}
		highlight_occ(matches, cur_occ, str_len);
		ry = ofy;
	}
exit:
	curs_set(1);
	reset_view();
}

// each thread searches on chunk of lines with this
static void _search_lc(uint from, uint to, list<gap_buf>::iterator it, const char *str, ushort str_len, uint &count)
{
	for (uint i = from; i < to; ++i, ++it)
		count += search_c(*it, str, str_len);
}

// each thread searches on chunk of lines with this
static void _search_la(uint from, uint to, list<gap_buf>::iterator it, const char *str, ushort str_len, vector<vector<uint>> &matches)
{
	for (uint i = from; i < to; ++i, ++it) {
		vector<uint> tmp = search_a(*it, str, str_len);
		matches.push_back(tmp);
	}
}

static void partition_chunks(uint &nthreads, uint &chunk, uint from, uint to)
{
	nthreads = thread::hardware_concurrency();
	if (nthreads == 0 || to - from < (uint)3e3)
		nthreads = 1;
	chunk = (to - from + 1) / nthreads;
}

static void join_threads(vector<thread> &threads)
{
	for (auto &thread : threads)
		if (thread.joinable())
			thread.join();
}

// search for str in range [from, to) return occurrences
vector<vector<uint>> search_la(uint from, uint to, const char *str, ushort str_len)
{
	uint nthreads, chunk;
	partition_chunks(nthreads, chunk, from, to);
	vector<thread> threads(nthreads);
	vector<vector<vector<uint>>> indices(nthreads);

	list<gap_buf>::iterator tmp_it = text.begin();
	advance(tmp_it, from);
	for (uint i = 0; i < nthreads; ++i) {
		uint st = i * chunk;
		uint end = min((i + 1) * chunk, to);

		threads.emplace_back(_search_la, st, end, tmp_it, str, str_len, ref(indices[i]));
		advance(tmp_it, chunk);
	}
	join_threads(threads);
	// merge results (each threads' chunks to one vector)
	vector<vector<uint>> result;
	for (vector<vector<uint>> &vec : indices)
		result.insert(result.end(), vec.begin(), vec.end());
	return result;
}

// search for str in range [from, to)
ulong search_lc(uint from, uint to, const char *str, ushort str_len)
{
	uint nthreads, chunk;
	partition_chunks(nthreads, chunk, from, to);
	vector<thread> threads(nthreads);
	vector<uint> indices(nthreads);

	list<gap_buf>::iterator tmp_it = text.begin();
	advance(tmp_it, from);
	for (uint i = 0; i < nthreads; ++i) {
		uint st = i * chunk;
		uint end = min((i + 1) * chunk, to);

		threads.emplace_back(_search_lc, st, end, tmp_it, str, str_len, ref(indices[i]));
		advance(tmp_it, chunk);
	}
	join_threads(threads);

	ulong total = 0;
	for (uint tmp : indices)
		total += tmp;
	return total;
}

static void _badchar(const char *str, uchar len)
{
	for (uint i = 0; i < 256; ++i) // BMH table
		badchar[i] = len;
	for (uint i = 0; i < len; i++)
		badchar[(uchar)str[i]] = len - i - 1;
}

static void _goodsuffix(const char *str, uchar len)
{
	int *pos = (int*)malloc(len * sizeof(int));
	fill(pos, pos + len, -1);

	for (uint i = 1; i < len; i++) {
		int j = pos[i - 1];
		while (j >= 0 && str[i] != str[j])
			j = pos[j];
		pos[i] = j + 1;
	}

	goodsuffix[0] = len;
	for (uint i = 1; i < len; i++)
		goodsuffix[i] = len - pos[i];

	for (uint i = len - 1; i > 0; i--) {
		if (str[i] != str[pos[i]])
			goodsuffix[i] = len - i;
		else
			goodsuffix[i] = goodsuffix[pos[i]];
	}
	free(pos);
}

static vector<uint> bm_search(const gap_buf &buf, const char *str, ushort len, bool append)
{
	vector<uint> matches;
	uint count = 0;

	for (uint i = 0; i < buf.len() - len;) {
		uint j;

		// check from end of str
		for (j = len - 1; j < len && str[j] == at(buf, i + j); --j);

		if (j > len) { // unsigned overflow => matched
			if (append) // this wasn't a bottleneck in benchmarks (maybe retest?)
				matches.push_back(i);
			else
				++count;
			i += len; // no overlaps
		} else
			i += max(badchar[(uchar)at(buf, i + j)], goodsuffix[j]);
	}
	if (!append)
		matches.push_back(count);
	return matches;
}

// each thread searches with this
static void searchch_a(const gap_buf &buf, char ch, ulong st, ulong end, vector<uint> &matches)
{
	ulong st1 = st, st2, end1 = end, end2 = 0;
	const char *buffer = buf.buffer();
	prepare_iteration(buf, st, end, st1, end1, st2, end2);
	for (ulong i = st1; i < end1; ++i)
		if (buffer[i] == ch)
			matches.push_back(i - st1 + st);
	for (ulong i = st2; i < end2; ++i)
		if (buffer[i] == ch)
			matches.push_back(i - st2 + st);
}

// each thread searches with this
static void searchch_c(const gap_buf &buf, char ch, ulong st, ulong end, uint &count)
{
	ulong st1, end1, st2, end2;
	const char *buffer = buf.buffer();
	prepare_iteration(buf, st, end, st1, end1, st2, end2);
	for (ulong i = st1; i < end1; ++i)
		if (buffer[i] == ch)
			++count;
	for (ulong i = st2; i < end2; ++i)
		if (buffer[i] == ch)
			++count;
}

// wrapper for searchch() to launch with multi-threaded
static vector<uint> mt_search(const gap_buf &buf, char ch, bool append)
{
	uint nthreads, chunk;
	partition_chunks(nthreads, chunk, 0, buf.len() - 1); // -1 as last char is \n
	vector<thread> threads(nthreads);
	vector<vector<uint>> indices(nthreads); // each thread's result

	for (uint i = 0; i < nthreads; ++i) {
		ulong st = i * chunk;
		ulong end = min((i + 1) * chunk, buf.len() - 1);

		if (append)
			threads.emplace_back(searchch_a, ref(buf), ch, st, end, ref(indices[i]));
		else {
			indices[i].push_back(0);
			threads.emplace_back(searchch_c, ref(buf), ch, st, end, ref(indices[i][0]));
		}
	}
	join_threads(threads);

	vector<uint> matches;
	matches.reserve(indices[0].size());
	if (append)
		for (const auto &vec : indices)
			matches.insert(matches.end(), vec.begin(), vec.end());
	else {
		matches.push_back(0);
		for (const auto &vec : indices)
			matches[0] += vec[0];
	}
	return matches;
}

// search for str in buf, return vector of occurences
vector<uint> search_a(const gap_buf &buf, const char *str, ushort len)
{
	vector<uint> matches;
	if (len >= buf.len())
		return matches;

	if (len == 1)
		matches = mt_search(buf, str[0], 1);
	else
		matches = bm_search(buf, str, len, 1);
	return matches;
}

uint search_c(const gap_buf &buf, const char *str, ushort len)
{
	if (len >= buf.len())
		return 0;

	vector<uint> matches;
	if (len == 1)
		matches = mt_search(buf, str[0], 0);
	else
		matches = bm_search(buf, str, len, 0);
	return matches[0];
}
