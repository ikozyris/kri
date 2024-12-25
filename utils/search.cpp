#include "headers/search.h"

void find(const char *str)
{
	unsigned short tmp_len = strlen(str);
	if (tmp_len == 0)
		return;
	vector<vector<unsigned>> matches(curnum + 1);
	unsigned total = 0;
	list<gap_buf>::iterator tmp_it = it;
	for (unsigned i = 0; i <= curnum; ++i, ++tmp_it) {
		matches[i] = search(*tmp_it, str, tmp_len);
		total += matches[i].size();
	}
	tmp_len -= mbcnt(str, tmp_len); // get displayed characters

	snprintf(lnbuf, lnbf_cpt, "%u matches     ", total);
	print2header(lnbuf, 1);

	// displayed x, previous byte, previous print byte, previous iterated occurrence
	vector<unsigned> dix(maxy), previ(maxy), prevpr(maxy), prevx(maxy);
	int ch = 0;
	curs_set(0);
	do {
		switch (ch) {
		case KEY_RIGHT:
			tmp_it = it;
			for (unsigned i = 0; i < maxy; ++i, ++tmp_it)
				if (prevpr[i] != 0 && previ[i] != matches[ofy + i].back()) // more occurences remaining
					mvprint_line(i, 0, *tmp_it, prevpr[i], 0);
			break;

		case 0:
		case KEY_LEFT:
			tmp_it = it;
			for (unsigned i = 0; i < maxy; ++i, ++tmp_it) {
				mvprint_line(i, 0, *tmp_it, 0, 0);
				dix[i] = previ[i] = prevpr[i] = prevx[i] = 0;
			}
			break;

		case KEY_DOWN:
			for (unsigned i = 0; i < maxy; ++i)
				dix[i] = previ[i] = prevpr[i] = prevx[i] = 0;
			if (ofy + maxy > curnum)
				break;
			++ofy;
			++it;
			print_text(0);
			break;

		case KEY_UP:
			for (unsigned i = 0; i < maxy; ++i)
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
		for (unsigned i = 0; i < min(maxy, curnum + 1); ++i, ++tmp_it) { // line
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
				wchgat(text_win, tmp_len, A_STANDOUT, 0, 0);
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

	// Galil rule
	for (unsigned i = len - 1; i > 0; i--) {
		if (str[i] != str[pos[i]])
			gs[i] = len - i;
		else
			gs[i] = gs[pos[i]];
	}
	free(pos);
	return gs;
}

vector<unsigned> bm_search(const gap_buf &buf, const char *str, unsigned short len)
{
	vector<unsigned> matches;
	// heuristics
	unsigned *badchar = _badchar(str, len);
	unsigned *goodsuffix = _goodsuffix(str, len);

	for (unsigned i = 0; i < buf.len - len;) {
		unsigned j;

		// check from end of str
		for (j = len - 1; j < len && str[j] == at(buf, i + j); --j);
		
		if (j > len) { // unsigned overflow
			matches.push_back(i);
			i += len; // no overlaps
		} else
			i += max(badchar[(unsigned char)at(buf, i + j)], goodsuffix[j]);
	}
	free(goodsuffix);
	free(badchar);
	return matches;
}

// merge the sorted results of each thread (this is the bottleneck)
vector<unsigned> mergei(const vector<vector<unsigned>> &indices)
{
	vector<unsigned> matches;
	matches.reserve(indices[0].size());
	for (const auto &vec : indices)
		matches.insert(matches.end(), vec.begin(), vec.end());
	return matches;
}

// each thread searches with this
void searchch(const gap_buf &buf, char ch, unsigned st, unsigned end, vector<unsigned> &matches)
{
	for (unsigned i = st; i < end; ++i)
		if (at(buf, i) == ch)
			matches.push_back(i);
}

// wrapper for searchch() to launch with multi-threaded
vector<unsigned> mt_search(const gap_buf &buf, char ch)
{
	unsigned nthreads = thread::hardware_concurrency();
	if (nthreads == 0 || buf.len < 1e6)
		nthreads = 1;
	unsigned chunk = buf.len / nthreads;
	vector<thread> threads(nthreads);
	vector<vector<unsigned>> indices(nthreads); // each thread's result

	for (unsigned i = 0; i < nthreads - 1; ++i) {
		unsigned st = i * chunk;
		unsigned end = (i + 1) * chunk;

		threads.emplace_back(searchch, ref(buf), ch, st, end, ref(indices[i]));
	}
	// last thread does the remaining (until buf.len)
	threads.emplace_back(searchch, ref(buf), ch, (nthreads - 1) * chunk, buf.len, ref(indices[nthreads - 1]));

	for (auto &thread : threads)
		if (thread.joinable())
			thread.join();

	vector<unsigned> matches = mergei(indices);
	return matches;
}

// search for str in buf, return <pos, color(pos;s)>
vector<unsigned> search(const gap_buf &buf, const char *str, unsigned short len)
{
	vector<unsigned> matches;
	if (len == 1)
		matches = mt_search(buf, str[0]);
		//searchch(buf, str[0], 0, buf.len, matches);
	else
		matches = bm_search(buf, str, len);

	return matches;
}

