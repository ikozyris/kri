#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>	// for setting locale
#include <vector>
#include <thread>
#include <bit>	// for __bit_ceil (cntlz)
#include <signal.h> // for pause in command suspend
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
typedef uint64_t ulong;
typedef uint32_t uint;
typedef uint16_t ushort;
typedef uint8_t uchar;
using namespace std;
