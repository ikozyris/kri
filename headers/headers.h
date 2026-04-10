#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>	// for setting locale
#include <vector>
#include <pthread.h>
#include <bit>	// for __bit_ceil (cntlz)
#include <signal.h> // for pause in command suspend
#include <sys/mman.h>
#include <sys/stat.h>
#include <linux/mman.h>
#include <fcntl.h>
#include <climits>
typedef uint8_t uchar;
using namespace std;
