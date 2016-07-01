/*
This file contains global variables for each project
*/

#include "vbetwo.h"

// vbetwo.h
VBEINFO vbe_info;
VBE_MODEINFO vbe_modeinfo;
int xres, yres;
unsigned char* contrast_lookup = 0;

// these are used by assembly routines... (*.asm)
unsigned long BytesPerScanline;
char*	offscreen;
int screensize_dword;
short	LFBselector;
Scanline* scanline_list;
unsigned char* shadinglookie[0x100];

// fifo99.h
bool KeyHandlerInstalled = false;

// grtext.h
unsigned short GRTEXT_BGColor = 0xFFFF; // GRTEXT_BGTRANSPARENT
int textcurx = 0, textcury = 0;
char charcellx, charcelly;
unsigned char charpattern[128][8];

// palettes.h
unsigned char NEAREST[15];

#include "function.h"
double VARIABLE_X, VARIABLE_Y;
RECORD_ERROR_VAL record_Error; // used for record_expression(...)
DERIVE_ERROR_VAL derive_Error; // used for derive_expression(...)
EVALUATE_ERROR_VAL evaluate_Error; // used for evaluate_expression(...)
