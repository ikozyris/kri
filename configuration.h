#define ctrl(x)         ((x) & 0x1f)
#undef	KEY_ENTER
#define KEY_ENTER       10 // overwrite default

/* Edit below to customize the editor
 * All configurations are made at compile time
 * to reduce perfomance and size overhead */

// keybindings
#define ENTER		KEY_ENTER
#define SAVE		ctrl('S')
#define EXIT		ctrl('X')
#define UP		KEY_UP
#define DOWN		KEY_DOWN
#define LEFT		KEY_LEFT
#define RIGHT		KEY_RIGHT
#define BACKSPACE	KEY_BACKSPACE
#define DELETE		KEY_DC
#define HOME		ctrl('A')
#define END		ctrl('E')
#define INFO		'i' // with alt
#define REFRESH		ctrl('R')
#define KEY_TAB		9
#define CMD		'c'
#define DELLINE		ctrl('K')

// customization options
//#define NO_LINES	// disable line numbering
#ifdef NO_LINES
#define LN_WIDTH 0
#else
#define LN_WIDTH 4	// digits of line numbering
#endif

#define HIGHLIGHT	// enable syntax highlighting
