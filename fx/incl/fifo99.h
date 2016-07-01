#ifndef __FIFO99_H
#define __FIFO99_H

#define KEY_ESC     1
#define KEY_1       2
#define KEY_2       3
#define KEY_3       4
#define KEY_4       5
#define KEY_5       6
#define KEY_6       7
#define KEY_7       8
#define KEY_8       9
#define KEY_9      10
#define KEY_0      11
#define KEY_BACK   14
#define KEY_TAB    15
#define KEY_Q      16
#define KEY_W      17
#define KEY_E      18
#define KEY_R      19
#define KEY_T      20
#define KEY_Z      21
#define KEY_U      22
#define KEY_I      23
#define KEY_O      24
#define KEY_P      25
#define KEY_ENTER  28
#define KEY_CTRL   29
#define KEY_LCTRL  29
#define KEY_A      30
#define KEY_S      31
#define KEY_D      32
#define KEY_F      33
#define KEY_G      34
#define KEY_H      35
#define KEY_J      36
#define KEY_K      37
#define KEY_L      38
#define KEY_LSHIFT 42
#define KEY_Y      44
#define KEY_X      45
#define KEY_C      46
#define KEY_V      47
#define KEY_B      48
#define KEY_N      49
#define KEY_M      50
#define KEY_RSHIFT 54
#define KEY_ALT    56
#define KEY_LALT   56
#define KEY_SPACE  57
#define KEY_CLOCK  58
#define KEY_F1     59
#define KEY_F2     60
#define KEY_F3     61
#define KEY_F4     62
#define KEY_F5     63
#define KEY_F6     64
#define KEY_F7     65
#define KEY_F8     66
#define KEY_F9     67
#define KEY_F10    68
#define KEY_F11    133
#define KEY_F12    134
#define KEY_NLOCK  69
#define KEY_SLOCK  70
#define KEY_PAD7   71
#define KEY_PAD8   72
#define KEY_PAD9   73
#define KEY_MINUS  74
#define KEY_PAD4   75
#define KEY_PAD5   76
#define KEY_PAD6   77
#define KEY_PLUS   78
#define KEY_PAD1   79
#define KEY_PAD2   80
#define KEY_PAD3   81
#define KEY_PAD0   82
#define KEY_PADPOINT 83

extern bool KeyHandlerInstalled;

void installkeyhandler();
void restorekeyhandler();
int getkey(int scancode);
int peekkey(int scancode);
void clearkey(int scancode);
void clearallkeys();
bool peekanykey();

#endif
