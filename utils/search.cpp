#include "headers/search.h"

// highlight or count occurrences of str in range [from, to]
void find(const char *str, uint from, uint to, char mode)
{
	uint str_len = strlen(str);
	if (str_len == 0)
		return;

	list<gap_buf>::iterator tmp_it = text.begin();
	advance(tmp_it, from);
	it = tmp_it;
	const ulong first_batch = min(maxy, to); // how many lines to display
	vector<vector<uint>> matches(first_batch + 1);
	ulong total = 0;

	if (mode == 'h') // save positions for highlighting
		for (uint i = from; i <= first_batch; ++i, ++tmp_it) { // only displayed lines
			matches[i - from] = search_a(*tmp_it, str, str_len);
			total += matches[i - from].size();
		}
	else // only count
		for (uint i = from; i <= first_batch; ++i, ++tmp_it)
			total += search_c(*tmp_it, str, str_len);

	for (uint i = first_batch + 1; i < to; ++i, ++tmp_it) // count the rest
		total += search_c(*tmp_it, str, str_len);

	clear_header();
	snprintf(lnbuf, lnbf_cpt, "%lu matches", total);
	print2header(lnbuf, 1);

	if (mode == 'c')
		return;
	str_len -= mbcnt(str, str_len); // get displayed characters

	// displayed x, previous byte, previous print byte, previous iterated occurrence
	vector<uint> dix(first_batch), previ(first_batch), prevpr(first_batch), prevx(first_batch);
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

		if (j > len) { // unsigned overflow
			if (append)
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

// merge the sorted results of each thread (this is the bottleneck)
vector<uint> mergei(const vector<vector<uint>> &indices, const bool append)
{
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

// each thread searches with this
void searchch_a(const gap_buf &buf, char ch, uint st, uint end, vector<uint> &matches)
{
	for (uint i = st; i < end; ++i)
		if (at(buf, i) == ch)
			matches.push_back(i);
}

// each thread searches with this
void searchch_c(const gap_buf &buf, char ch, uint st, uint end, uint &count)
{
	for (uint i = st; i < end; ++i)
		if (at(buf, i) == ch)
			++count;
}

// wrapper for searchch() to launch with multi-threaded
vector<uint> mt_search(const gap_buf &buf, char ch, const bool append)
{
	uint nthreads = thread::hardware_concurrency();
	if (nthreads == 0 || buf.len() < 1e6)
		nthreads = 1;
	uint chunk = buf.len() / nthreads;
	vector<thread> threads(nthreads);
	vector<vector<uint>> indices(nthreads); // each thread's result

	for (uint i = 0; i < nthreads; ++i) {
		uint st = i * chunk;
		uint end = min((i + 1) * chunk, buf.len());

		if (append)
			threads.emplace_back(searchch_a, ref(buf), ch, st, end, ref(indices[i]));
		else {
			indices[i].push_back(0);
			threads.emplace_back(searchch_c, ref(buf), ch, st, end, ref(indices[i][0]));
		}
	}

	for (auto &thread : threads)
		if (thread.joinable())
			thread.join();

	vector<uint> matches = mergei(indices, append);
	return matches;
}

// search for str in buf, return vector of occurences
vector<uint> search_a(const gap_buf &buf, const char *str, ushort len)
{
	vector<uint> matches;
	if (len > buf.len())
		return matches;

	if (len == 1)
		matches = mt_search(buf, str[0], 1);
		//searchch(buf, str[0], 0, buf.len, matches); // single-thread
	else
		matches = bm_search(buf, str, len, 1);

	return matches;
}

uint search_c(const gap_buf &buf, const char *str, ushort len)
{
	if (len > buf.len())
		return 0;

	vector<uint> matches;
	if (len == 1)
		matches = mt_search(buf, str[0], 0);
	else
		matches = bm_search(buf, str, len, 0);
	return matches[0];
}
