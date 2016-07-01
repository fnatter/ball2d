/*
This file contains global variables for each project
*/

#include "vbetwo.h"
#include "gformats.h"

// vbetwo.h
VBEINFO vbe_info;
VBE_MODEINFO vbe_modeinfo;
int xres, yres;
unsigned char* contrast_lookup = NULL;

// these are used by assembly routines... (*.asm)
unsigned long BytesPerScanline;
char*	offscreen;
int screensize_dword;
short	LFBselector;
Scanline* scanline_list;
unsigned char* shadinglookie[0x100];

// grtext.h
unsigned short GRTEXT_BGColor = 0xFFFF; // GRTEXT_BGTRANSPARENT
int textcurx = 0, textcury = 0;
char charcellx, charcelly;
unsigned char charpattern[128][8];

// gformats.h
DIB_FILE_TYPE GF_DIB_type = DIB_FILE_DOES_NOT_EXIST;

// palettes.h
unsigned char NEAREST[15];
