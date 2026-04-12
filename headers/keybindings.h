#define ctrl(x)         ((x) & 0x1f)
#undef	KEY_ENTER
#define KEY_ENTER       10 // overwrite default

/* EDIT BELOW to change keybindings */

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
#define SWITCH		'r'
#define DELLINE		ctrl('K')
#define SWAP_UP		KEY_SR
#define SWAP_DOWN	KEY_SF
