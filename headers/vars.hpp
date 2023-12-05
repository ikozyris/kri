#include "gapbuffer.hpp"
#define DEFAULT_LINES 128

WINDOW *header_win, *ln_win, *text_win;

#include <deque>
unsigned txt_cpt = DEFAULT_LINES; // deque does not expose this property itself
std::deque<gap_buf> text(DEFAULT_LINES);
std::deque<gap_buf>::iterator it;
//gap_buf text[DEFAULT_LINES];

wchar_t s[2];
unsigned char y, x;
// offset in y axis of text and screen
long signed int ofy;
unsigned ry;
unsigned char prevy;
unsigned maxy, maxx; // to store the maximum rows and columns
int ch;
char filename[FILENAME_MAX];

// indx: tmp for lenght of line
// curnum: total lines
size_t indx, curnum;
signed int mx;
