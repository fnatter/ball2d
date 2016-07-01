#ifndef __GRTEXT_H
#define __GRTEXT_H

#define GRTEXT_BGTRANSPARENT	      0xFFFF
#define GRTEXT_TABLENGTH	      6

#include <sys/movedata.h>

extern unsigned short GRTEXT_BGColor;
extern int textcurx, textcury;
extern char charcellx, charcelly;
extern unsigned char charpattern[128][8];

bool load8x8fontpattern(const char* szfilename);
void save8x8fontpattern(const char* szfilename);
bool getcharpatternbit(int character, int x, int y);
void xorcharpatternbit(int character, int x, int y, bool value);
void showchar(char character, unsigned char color);
void showstr(const char* stringz, unsigned char color);
void floatstr(float x, char* str, int prec = 2);
void appendString(char* dest, char* suffix);
void changeCharacter(int ascii, char row1, char row2, char row3, char row4, char row5, char row6, char row7, char row8);

inline int getcharcellwidth()
{
  return charcellx;
};

inline int getcharcellheight()
{
  return charcelly;
};

inline void showstrxy(int x, int y, const char* stringz, unsigned char color)
{
  textcurx = x;
  textcury = y;
  showstr(stringz, color);
};

inline void settextsize(unsigned char cellx, unsigned char celly = 0)
{
  charcellx = cellx;
  if (celly == 0)
    charcelly = cellx;
  else
    charcelly = celly;
};

inline void getBDA8x8charpattern()
{
  //get 128 * 8 bytes (1024) @ 0xF000:0xFA6E, physical_address = 0xF0000 + 0x0FA6E = 0xFFA6E
  dosmemget(0xFFA6E, 1024, charpattern);
};

#endif
