#include "gapbuffer.h"

/* small lines are merged if they sum < 256 B (to reduce size of len array)
 * 256 to keep the array 1 byte per line (overhead ratio always <2 (or <1.7 for lines >1B))
 * this limit could be increased if it can be guaranteed that no merged line is > 256 */
#define MAX_CHUNK_SIZE 256
#define connect(a, b) {(a)->next = (b); (b)->prev = (a);}

// small lines are merged to reduce memory overhead
typedef struct chunk {
	gap_buf merged_lines;
	struct chunk *prev;
	struct chunk *next;
	// should be unallocated unless there are 2 lines merged
	uchar *len; // array of lengths of each merged line
	uint num_lines; // count of lines in this chunk
	uchar len_cpt; // capacity of len array (initialized to 1)
	// 3-bytes padding (45 / 48 bytes)
} chunk;
/* Notes:
 * head != tail, head and tail are imaginary nodes.
 * 
 */
typedef struct llist {
	uint nodes; // count of actual nodes allocated
	uint lines; // count of lines (nodes + merged)
	chunk *head; // imaginary node before first usable node
	chunk *tail; // imaginary node after last usable node
} llist;

typedef struct iter {
	gap_buf *orig;
	uint global_pos; // position in the list
	ushort offset; // offset of start of this line to 0
	ushort relative_pos; // pos in the chunk
	chunk *parent() const { return (chunk*)(orig); } // force-casting
	// proxy functions to emulate being a standalone gap buffer
	uint len() const { return parent()->len ? parent()->len[relative_pos] : orig->len(); }
	uint gps() const { return orig->gps - offset; }
	uint gpe() const { return orig->gpe - offset; }
	uint cpt() const { return len() + orig->gpe - orig->gps; }
	void set_gps(uint n) { orig->gps = n + offset; }
	void set_gpe(uint n) { orig->gpe = n + offset; }
	char *buffer() { return orig->buffer() + offset; }
} iter;


void line_offset(iter *it, uint n);
void iterate_bw(iter *it, uint dist); // backward
void iterate_fw(iter *it, uint dist); // forward
void rm_mline(chunk *ch, uint pos, iter *it);
void split_mline(llist *list, chunk *cur_chunk);
void merge_lines(llist *list, iter *a, iter *b);
void insc_after(llist *list, chunk *previous, chunk *new_chunk);
chunk *create_chunk();
void goto_last_line(chunk *a);
void append_len(chunk *a, uint len);
void point2chunk(iter *it, chunk *a);

extern llist text;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
inline void point2begin(iter *it) { *it = {.orig = &text.head->next->merged_lines}; }
#pragma GCC diagnostic pop
