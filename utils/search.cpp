#include "headers/search.h"

// mode: thi-0/all-1 line | cnt-0/shw-1 results | (bits)
void find(const char *str, unsigned char mode)
{
	unsigned short str_len = strlen(str);
	if (str_len == 0)
		return;
	const unsigned first_batch = min(maxy, curnum + 1);
	vector<vector<unsigned>> matches(first_batch + 1);
	list<gap_buf>::iterator tmp_it = it;
	unsigned total = 0;
	if (mode & 2) { // all lines
		if (mode & 1) // append
			for (unsigned i = 0; i < first_batch; ++i, ++tmp_it) {
				matches[i] = search_a(*tmp_it, str, str_len);
				total += matches[i].size();
			}
		else // only count
			for (unsigned i = 0; i < first_batch; ++i, ++tmp_it)
				total += search_c(*tmp_it, str, str_len);
		for (unsigned i = first_batch; i <= curnum; ++i, ++tmp_it) // count the rest
			total += search_c(*tmp_it, str, str_len);
	} else { // this line
		if (mode & 1) {// show results
			matches[0] = search_a(*it, str, str_len);
			total = matches[0].size();
		} else
			total = search_c(*it, str, str_len);
	}
	snprintf(lnbuf, lnbf_cpt, "%u matches     ", total);
	print2header(lnbuf, 1);
	if ((mode & 1) == 0)
		return;
	str_len -= mbcnt(str, str_len); // get displayed characters

	// displayed x, previous byte, previous print byte, previous iterated occurrence
	vector<unsigned> dix(first_batch), previ(first_batch), prevpr(first_batch), prevx(first_batch);
	int ch = 0;
	curs_set(0);
	do {
		switch (ch) {
		case KEY_RIGHT:
			tmp_it = it;
			for (unsigned i = 0; i < first_batch; ++i, ++tmp_it)
				if (prevpr[i] != 0 && previ[i] != matches[ofy + i].back()) // more occurrences remaining
					mvprint_line(i, 0, *tmp_it, prevpr[i], 0);
			break;

		case 0:
		case KEY_LEFT:
			tmp_it = it;
			for (unsigned i = 0; i < first_batch; ++i, ++tmp_it) {
				mvprint_line(i, 0, *tmp_it, 0, 0);
				dix[i] = previ[i] = prevpr[i] = prevx[i] = 0;
			}
			break;

		case KEY_DOWN:
			for (unsigned i = 0; i < first_batch; ++i)
				dix[i] = previ[i] = prevpr[i] = prevx[i] = 0;
			if (ofy + maxy > curnum)
				break;
			++ofy;
			++it;
			print_text(0);
			++tmp_it;
			matches.emplace_back(search_a(*tmp_it, str, str_len));
			break;

		case KEY_UP:
			for (unsigned i = 0; i < first_batch; ++i)
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
		for (unsigned i = 0; i < first_batch; ++i, ++tmp_it) { // line
			for (unsigned j = prevx[i]; j < matches[ofy + i].size(); ++j) { // occurrence
				calc_offset_act(matches[ofy + i][j], previ[i], *tmp_it);
				prevx[i] = j;
				if (dix[i] + flag >= maxx - 1 + prevpr[i]) {
					prevpr[i] = matches[ofy + i][j] - matches[ofy + i][j] % (maxx - 1);
					break;
				}
				previ[i] = matches[ofy + i][j];
				dix[i] += flag;
				wmove(text_win, i, dix[i] % (maxx - 1));
				wchgat(text_win, str_len, A_STANDOUT, 0, 0);
			}
		}
	} while ((ch = wgetch(text_win)));
exit:
	curs_set(1);
	reset_view();
}

unsigned *_badchar(const char *str, unsigned short len)
{
	unsigned *badchar = (unsigned*)malloc(256 * sizeof(unsigned));
	for (unsigned i = 0; i < 256; ++i) // BMH table
		badchar[i] = len;
	for (unsigned i = 0; i < len; i++)
		badchar[(unsigned char)str[i]] = len - i - 1;

	return badchar;
}

unsigned *_goodsuffix(const char *str, unsigned short len)
{
	unsigned *gs = (unsigned*)malloc(len * sizeof(unsigned));
	int *pos = (int*)malloc(len * sizeof(int));
	fill(pos, pos + len, -1);

	for (unsigned i = 1; i < len; i++) {
		int j = pos[i - 1];
		while (j >= 0 && str[i] != str[j])
			j = pos[j];
		pos[i] = j + 1;
	}

	gs[0] = len;
	for (unsigned i = 1; i < len; i++)
		gs[i] = len - pos[i];

	for (unsigned i = len - 1; i > 0; i--) {
		if (str[i] != str[pos[i]])
			gs[i] = len - i;
		else
			gs[i] = gs[pos[i]];
	}
	free(pos);
	return gs;
}

vector<unsigned> bm_search(const gap_buf &buf, const char *str, unsigned short len, bool append)
{
	vector<unsigned> matches;
	unsigned count = 0;
	// heuristics
	unsigned *badchar = _badchar(str, len);
	unsigned *goodsuffix = _goodsuffix(str, len);

	for (unsigned i = 0; i < buf.len - len;) {
		unsigned j;

		// check from end of str
		for (j = len - 1; j < len && str[j] == at(buf, i + j); --j);

		if (j > len) { // unsigned overflow
			if (append)	
				matches.push_back(i);
			else
				++count;
			i += len; // no overlaps
		} else
			i += max(badchar[(unsigned char)at(buf, i + j)], goodsuffix[j]);
	}
	free(goodsuffix);
	free(badchar);
	if (!append)
		matches.push_back(count);
	return matches;
}

// merge the sorted results of each thread (this is the bottleneck)
vector<unsigned> mergei(const vector<vector<unsigned>> &indices, const bool append)
{
	vector<unsigned> matches;
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
void searchch_a(const gap_buf &buf, char ch, unsigned st, unsigned end, vector<unsigned> &matches)
{
	for (unsigned i = st; i < end; ++i)
		if (at(buf, i) == ch)
			matches.push_back(i);
}

// each thread searches with this
void searchch_c(const gap_buf &buf, char ch, unsigned st, unsigned end, unsigned &count)
{
	for (unsigned i = st; i < end; ++i)
		if (at(buf, i) == ch)
			++count;
}

// wrapper for searchch() to launch with multi-threaded
vector<unsigned> mt_search(const gap_buf &buf, char ch, const bool append)
{
	unsigned nthreads = thread::hardware_concurrency();
	if (nthreads == 0 || buf.len < 1e6)
		nthreads = 1;
	unsigned chunk = buf.len / nthreads;
	vector<thread> threads(nthreads);
	vector<vector<unsigned>> indices(nthreads); // each thread's result

	for (unsigned i = 0; i < nthreads; ++i) {
		unsigned st = i * chunk;
		unsigned end = min((i + 1) * chunk, buf.len);

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

	vector<unsigned> matches = mergei(indices, append);
	return matches;
}

// search for str in buf, return <pos, color(pos;s)>
vector<unsigned> search_a(const gap_buf &buf, const char *str, unsigned short len)
{
	vector<unsigned> matches;
	if (len == 1)
		matches = mt_search(buf, str[0], 1);
		//searchch(buf, str[0], 0, buf.len, matches);
	else
		matches = bm_search(buf, str, len, 1);

	return matches;
}

unsigned search_c(const gap_buf &buf, const char *str, unsigned short len)
{
	vector<unsigned> matches;
	if (len == 1)
		matches = mt_search(buf, str[0], 0);
	else
		matches = bm_search(buf, str, len, 0);
	return matches[0];
}
