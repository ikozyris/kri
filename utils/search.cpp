#include "headers/search.h"

// highlight or count occurrences of str in range [from, to)
void find(const char *str, uint from, uint to, char mode)
{
	uint str_len = strlen(str);
	if (str_len == 0 || to > curnum || from > to)
		return;

	list<gap_buf>::iterator tmp_it = text.begin();
	advance(tmp_it, from);
	it = tmp_it;
	const ulong first_batch = min(maxy, to); // how many lines to display
	vector<vector<uint>> matches;
	ulong total = 0;
	if (mode == 'h') {
		// fetches only the occurrences that will be shown +1
		matches = search_la(from, from + first_batch + 1, str, str_len);
		for (const auto &vec : matches)
			total += vec.size();
		if (to > first_batch) // just count the remaining (if any)
			total += search_lc(first_batch + 1, to, str, str_len);
		// TODO: fix the showing highlights to work without overcounting
		else if (to < first_batch)
			total -= matches.back().size(); // overcounted
	} else
		total += search_lc(from, to, str, str_len);

	clear_header();
	snprintf(lnbuf, lnbf_cpt, "%lu matches", total);
	print2header(lnbuf, 1);

	if (mode == 'c')
		return;
	str_len -= mbcnt(str, str_len); // get displayed characters

	// displayed x, previous byte, previous print byte, previous iterated occurrence
	vector<uint> dix(maxy), previ(maxy), prevpr(maxy), prevx(maxy);
	int ch = 0;
	curs_set(0);
	do {
		switch (ch) {
		case KEY_RIGHT:
			tmp_it = it;
			for (uint i = from; i < first_batch; ++i, ++tmp_it)
				if (prevpr[i] != 0 && previ[i] != matches[ofy + i].back()) // more occurrences remaining
					mvprint_line(i, 0, *tmp_it, prevpr[i], 0);
			break;

		case 0:
		case KEY_LEFT:
			tmp_it = it;
			for (uint i = from; i < first_batch; ++i, ++tmp_it) {
				mvprint_line(i, 0, *tmp_it, 0, 0);
				dix[i] = previ[i] = prevpr[i] = prevx[i] = 0;
			}
			break;

		case KEY_DOWN:
			for (uint i = from; i < first_batch; ++i)
				dix[i] = previ[i] = prevpr[i] = prevx[i] = 0;
			if (ofy + maxy > min(curnum, to))
				break;
			++ofy;
			++it;
			print_text(0);
			++tmp_it;
			matches.emplace_back(search_a(*tmp_it, str, str_len));
			break;

		case KEY_UP:
			for (uint i = from; i < first_batch; ++i)
				dix[i] = previ[i] = prevpr[i] = prevx[i] = 0;
			if (ofy <= 0)
				break;
			--ofy;
			--it;
			print_text(0);
			break;

		case 27:
		case 'q':
		default:
			goto exit;
		}

		tmp_it = it;
		for (uint i = 0; i < first_batch; ++i, ++tmp_it) { // line
			for (uint j = prevx[i]; j < matches[ofy + i].size(); ++j) { // occurrence
				const uint pos = matches[ofy + i][j];
				const uint hg_pos = bytes2dchar(pos, previ[i], *tmp_it);
				prevx[i] = j;
				if (dix[i] + hg_pos >= maxx - 1 + prevpr[i]) { // cut
					prevpr[i] = pos - pos % (maxx - 1);
					break;
				}
				previ[i] = pos;
				dix[i] += hg_pos;
				wmove(text_win, i + from, dix[i] % (maxx - 1));
				wchgat(text_win, str_len, A_STANDOUT, 0, 0);
			}
		}
	} while ((ch = wgetch(text_win)));
exit:
	curs_set(1);
	reset_view();
}

// each thread searches on chunk of lines with this
void _search_lc(uint from, uint to, list<gap_buf>::iterator it, const char *str, ushort str_len, uint &count)
{
	for (uint i = from; i < to; ++i, ++it)
		count += search_c(*it, str, str_len);
}

// each thread searches on chunk of lines with this
void _search_la(uint from, uint to, list<gap_buf>::iterator it, const char *str, ushort str_len, vector<vector<uint>> &matches)
{
	for (uint i = from; i < to; ++i, ++it) {
		vector<uint> tmp = search_a(*it, str, str_len);
		matches.push_back(tmp);
	}
}

void partition_chunks(uint &nthreads, uint &chunk, uint from, uint to)
{
	nthreads = thread::hardware_concurrency();
	if (nthreads == 0 || to - from < (uint)1e6)
		nthreads = 1;
	chunk = (to - from + 1) / nthreads;
}

void join_threads(vector<thread> &threads)
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

uchar *_badchar(const char *str, uchar len)
{
	uchar *badchar = (uchar*)malloc(256);
	for (uint i = 0; i < 256; ++i) // BMH table
		badchar[i] = len;
	for (uint i = 0; i < len; i++)
		badchar[(uchar)str[i]] = len - i - 1;

	return badchar;
}

uint *_goodsuffix(const char *str, ushort len)
{
	uint *gs = (uint*)malloc(len * sizeof(uint));
	int *pos = (int*)malloc(len * sizeof(int));
	fill(pos, pos + len, -1);

	for (uint i = 1; i < len; i++) {
		int j = pos[i - 1];
		while (j >= 0 && str[i] != str[j])
			j = pos[j];
		pos[i] = j + 1;
	}

	gs[0] = len;
	for (uint i = 1; i < len; i++)
		gs[i] = len - pos[i];

	for (uint i = len - 1; i > 0; i--) {
		if (str[i] != str[pos[i]])
			gs[i] = len - i;
		else
			gs[i] = gs[pos[i]];
	}
	free(pos);
	return gs;
}

vector<uint> bm_search(const gap_buf &buf, const char *str, ushort len, bool append)
{
	vector<uint> matches;
	uint count = 0;
	// heuristics
	uchar *badchar = _badchar(str, len);
	uint *goodsuffix = _goodsuffix(str, len);

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
	free(goodsuffix);
	free(badchar);
	if (!append)
		matches.push_back(count);
	return matches;
}

// each thread searches with this
void searchch_a(const gap_buf &buf, char ch, ulong st, ulong end, vector<uint> &matches)
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
void searchch_c(const gap_buf &buf, char ch, ulong st, ulong end, uint &count)
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
vector<uint> mt_search(const gap_buf &buf, char ch, bool append)
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
