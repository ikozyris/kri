#include "headers/merged_unrolled-list.h"

// point iterator to chunk (global_pos not updated)
void point2chunk(iter *it, chunk *a)
{
	// *it = {.orig = x} not used as it zeroes global_pos
	it->orig = &a->merged_lines;
	it->relative_pos = it->offset = 0;
}

void append_len(chunk *a, uint len)
{
	if (a->len_cpt <= a->num_lines + 1) {
		a->len = (uchar*)realloc(a->len, a->len_cpt * 2);
		memset(a->len + a->len_cpt - 1, 0, a->len_cpt + 1);
		a->len_cpt *= 2;
	}
	a->len[a->num_lines] = len;
	a->num_lines++;
}

// given offset find position
uint line_pos(chunk *ch, uint offset)
{
	uint sum = 0, n = 0;
	while (sum < offset && n < ch->num_lines) {
		sum += ch->len[n];
		n++;
	}
	return sum == offset ? n : n - 1; // TODO: there is a cleaner way
}

// given position find offset
void line_offset(iter *it, uint n)
{
	chunk *a = it->parent();
	uint i = it->relative_pos;
	for (; i < it->relative_pos + n; ++i)
		it->offset += a->len[i];
	it->relative_pos = i;
}

// increase iterator backwards by dist lines
void iterate_bw(iter *it, uint dist)
{
	chunk *a = it->parent();
	it->global_pos -= dist;
	if (dist > it->relative_pos) {
		dist -= it->relative_pos;
		a = a->prev;
		while (dist > a->num_lines) {
			dist -= a->num_lines;
			a = a->prev;
		}
		it->orig = &a->merged_lines;
		it->relative_pos = a->num_lines;
	}
	uint tmp = it->relative_pos;
	it->relative_pos = it->offset = 0;
	line_offset(it, tmp - dist);
}

// increase iterator forwards by dist lines
void iterate_fw(iter *it, uint dist)
{
	chunk *a = it->parent();
	it->global_pos += dist;
	if (dist + it->relative_pos >= a->num_lines) { // go to next chunk
		dist += it->relative_pos;
		do {
			dist -= a->num_lines;
			a = a->next;
		} while (dist > a->num_lines);
		point2chunk(it, a);
	}
	line_offset(it, dist);
}

// remove a line from a chunk
void rm_mline(chunk *ch, uint pos, iter *it) { // FIXME: broken
	if (ch->num_lines == 1) {
		connect(ch->prev, ch->next);
		free(ch);
	} else {
		if (it)
			it->orig->gpe = it->offset + it->len();
		memmove(&ch->len[pos], &ch->len[pos + 1], ch->num_lines - pos);
		ch->num_lines--;
	}
}

// moves actual cursor to last line
void goto_last_mline(chunk *a) { mv_curs(a->merged_lines, a->merged_lines.len() - a->len[a->num_lines]); }

// merged line has grown too much; split last line by moving it to next node (or create new to fit)
// (it doesn't matter which of the merged lines is split as they are <= 256B)
void split_mline(llist *list, chunk *a)
{
	uint last_length = a->len[a->num_lines];

	chunk *next_chunk; // may be newly allocated
	// create new chunk if last line doesn't fit in next chunk
	if (a->next == list->tail || a->next->merged_lines.len() + last_length > MAX_CHUNK_SIZE) {
		next_chunk = create_chunk();
		insc_after(list, a, next_chunk);
	} else { // shift all lengths of next chunk by one to put this length in pos 0
		next_chunk = a->next;
		memmove(&next_chunk->len[1], &next_chunk->len[0], next_chunk->num_lines);
	}
	a->num_lines--;
	apnd_s(next_chunk->merged_lines, a->merged_lines.buffer() + a->merged_lines.len() - last_length, last_length);
	append_len(next_chunk, last_length);
	// delete last line
	a->merged_lines.gpe = a->merged_lines.cpt() - 1;
}

// merge b into a
void mergeba(iter *a, iter *b)
{
	chunk *a_ch = a->parent(), *b_ch = b->parent();
	insert_s(a_ch->merged_lines, b_ch->merged_lines.buffer(), b->len() - 1);
	a_ch->len[a->relative_pos] = a->len() + b->len() - 1;
	rm_mline(b_ch, b->relative_pos, nullptr);
}

// TODO: this is a mess
// merge a and b, b is invalidated
void merge_lines(llist *list, iter *a, iter *b)
{
	chunk *a_ch = a->parent(), *b_ch = b->parent();
	if (a_ch == b_ch) {
		eras(a_ch->merged_lines);
		uint rel_pos = b->relative_pos;
		a_ch->len[rel_pos] += a_ch->len[rel_pos - 1] - 1;
		memmove(&a_ch->len[rel_pos - 1], &a_ch->len[rel_pos], a_ch->num_lines - rel_pos);
		a_ch->num_lines--;
	} else { // need to move at least one line to another chunk
		bool use_a = a_ch->merged_lines.len() + b->len() < MAX_CHUNK_SIZE || a_ch->num_lines <= 1;
		bool use_b = b_ch->merged_lines.len() + a->len() < MAX_CHUNK_SIZE || b_ch->num_lines <= 1;
		if (use_a && use_b) { // merge small into large
			if (a_ch->merged_lines.len() < b_ch->merged_lines.len())
				use_b = false;
			else
				use_a = false;
		}
		if (use_a)
			mergeba(b, a);
		else if (use_b)
			mergeba(a, b);
		else { // 3rd case: none fits; create new chunk
			chunk *new_chunk = create_chunk();
			// combine a and b's lines (remove a's last char as a newline)
			apnd_s(new_chunk->merged_lines, a_ch->merged_lines.buffer() + a->offset, a->len() - 1);
			rm_mline(a_ch, a->relative_pos, nullptr);
			apnd_s(new_chunk->merged_lines, b_ch->merged_lines.buffer() + b->offset, b->len());
			rm_mline(b_ch, b->relative_pos, nullptr);

			append_len(new_chunk, a->len() + b->len() - 1);
			insc_after(list, a_ch, new_chunk);
			point2chunk(a, a_ch);
		}
	}
}

// insert chunk after
void insc_after(llist *list, chunk *previous, chunk *new_chunk)
{
	connect(new_chunk, previous->next);
	connect(previous, new_chunk);
	list->nodes++;
}

chunk *create_chunk()
{
	chunk *new_chunk = (chunk*)malloc(sizeof(chunk));
	// len is actually unallocated, but len_cpt=1 makes 1st realloc easier
	new_chunk->len_cpt = 1;
	new_chunk->len = nullptr;
	new_chunk->num_lines = 0;
	//*new_chunk = {.len_cpt = 1, .num_lines = 1}; // TODO: use this in C
	init(new_chunk->merged_lines);
	return new_chunk;
}
