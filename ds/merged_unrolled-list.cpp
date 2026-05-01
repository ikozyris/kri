#include "headers/merged_unrolled-list.h"

// point iterator to chunk (global_pos not updated)
void point2chunk(iter *it, const chunk *a)
{
	// *it = {.orig = x} not used as it zeroes global_pos
	it->orig = const_cast<gap_buf*>(&a->merged_lines);
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
uint line_pos(const chunk *ch, uint offset)
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
	const chunk *a = it->parent();
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
		} while (dist >= a->num_lines);
		point2chunk(it, a);
	}
	line_offset(it, dist);
}

// remove a line from a chunk
void rm_mline(chunk *ch, uint pos, const iter *it) {
	if (ch->num_lines == 1) {
		connect(ch->prev, ch->next);
		free(ch);
	} else {
		mv_curs(ch->merged_lines, it->offset);
		ch->merged_lines.gpe += it->len();
		if (pos < ch->num_lines - 1)
			memmove(&ch->len[pos], &ch->len[pos + 1], ch->num_lines - pos - 1);
		ch->num_lines--;
	}
}

// moves actual cursor to last line
//void goto_last_mline(chunk *a) { mv_curs(a->merged_lines, a->merged_lines.len() - a->len[a->num_lines - 1]); }

// merged line has grown too much; split last line by moving it to next node (or create new to fit)
// (it doesn't matter which of the merged lines is split as they are <= 256B)
void split_mline(llist *list, chunk *a)
{
	uint last_length = a->len[a->num_lines - 1];

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
	mv_curs(a->merged_lines, a->merged_lines.len() - last_length);
	insert_s(next_chunk->merged_lines, a->merged_lines.buffer() + a->merged_lines.cpt() - last_length, last_length);
	append_len(next_chunk, last_length);
	// delete last line
	a->merged_lines.gpe = a->merged_lines.cpt() - 1;
}

// merge src into dest
void merge_chunks(const iter *src, iter *dest, bool append)
{
	chunk *dest_ch = dest->parent(), *src_ch = src->parent();
	if (append) { // append src to dest
		mv_curs(dest_ch->merged_lines, dest->offset + dest->len());
		eras(dest_ch->merged_lines);
		copy_buffer(*src->orig, dest_ch->merged_lines, src->offset, src->offset + src->len());
	} else { // prepend src to dest
		mv_curs(dest_ch->merged_lines, dest->offset);
		copy_buffer(*src->orig, dest_ch->merged_lines, src->offset, src->offset + src->len() - 1);
	}
	if (dest_ch->len)
		dest_ch->len[dest->relative_pos] = dest->len() + src->len() - 1;
	rm_mline(src_ch, src->relative_pos, src);
}

// TODO: this is a mess
// merge a and b, b is invalidated
void merge_lines(llist *list, iter *a, iter *b)
{
	chunk *a_ch = a->parent(), *b_ch = b->parent();
	uint len_a = a->len(), len_b = b->len();

	// merge b into a in-place
	if (a_ch == b_ch && len_a + len_b - 1 < MAX_CHUNK_SIZE) {
		eras(a_ch->merged_lines);
		uint rel_pos = b->relative_pos;
		a_ch->len[rel_pos] += a_ch->len[rel_pos - 1] - 1;
		memmove(&a_ch->len[rel_pos - 1], &a_ch->len[rel_pos], a_ch->num_lines - rel_pos);
		a_ch->num_lines--;
	} else { // need to move at least one line to another chunk
		bool use_a = a_ch->merged_lines.len() + len_b < MAX_CHUNK_SIZE || a_ch->num_lines <= 1;
		bool use_b = b_ch->merged_lines.len() + len_a < MAX_CHUNK_SIZE || b_ch->num_lines <= 1;
		// prefer merging small into large when both are possible
		if (use_a && (!use_b || a_ch->merged_lines.len() >= b_ch->merged_lines.len()))
			merge_chunks(b, a, true);
		else if (use_b) {
			merge_chunks(a, b, false);
			*a = *b;
			a->global_pos--;
		} else { // create new chunk
			chunk *new_chunk = create_chunk();
			insc_after(list, a_ch, new_chunk);

			copy_buffer(*a->orig, new_chunk->merged_lines, a->offset, a->offset + len_a - 1);
			copy_buffer(*b->orig, new_chunk->merged_lines, b->offset, b->offset + len_b);

			rm_mline(b_ch, b->relative_pos, b);
			rm_mline(a_ch, a->relative_pos, a);

			if (len_a + len_b - 1 < MAX_CHUNK_SIZE) // in a merged line
				append_len(new_chunk, len_a + len_b - 1);
			else // standalone line
				new_chunk->num_lines = 1;

			point2chunk(a, new_chunk);
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
	new_chunk->num_lines = 0; // cannot be 1 as it breaks append_len during reading
	//*new_chunk = {.len_cpt = 1, .num_lines = 0}; // TODO: use this in C
	init(new_chunk->merged_lines);
	return new_chunk;
}
