#include "grtext.h"
#include "vbetwo.h"
#include "quitbug.h"
#include <math.h> // for functions in floatstr(float, char*, int)
#include <stdlib.h> //itoa
#include <stdio.h> // fileio

extern int textcurx, textcury;
extern char charcellx, charcelly;
extern unsigned char charpattern[128][8];

void appendString(char* dest, char* suffix)
{
  strcpy(dest + strlen(dest), suffix);
};

void changeCharacter(int ascii, char row1, char row2, char row3, char row4, char row5, char row6, char row7, char row8)
{
  charpattern[ascii][0] = row1;
  charpattern[ascii][1] = row2;
  charpattern[ascii][2] = row3;
  charpattern[ascii][3] = row4;
  charpattern[ascii][4] = row5;
  charpattern[ascii][5] = row6;
  charpattern[ascii][6] = row7;
  charpattern[ascii][7] = row8;
};

void floatstr(float x, char* str, int prec)
{
  double whole;
  int integ, fract, i, precC;
  switch (prec)
    {
    case 1: precC = 10;break;
    case 2: precC = 100;break;
    case 3: precC = 1000;break;
    case 4: precC = 10000;break;
    case 5: precC = 100000;break;
    case 6: precC = 1000000;break;
    case 7: precC = 10000000;break;
    default: handleError(ErrorMessage("Invalid precision value given to floatstr() ", "", prec));
    }
  fract = (int)(precC * modf(x, &whole));
  integ = (int)whole;
  if (integ == 0 && fract < 0)
    {
      str[0] = '-';
      itoa(integ, str + 1, 10);
    }
  else
    itoa(integ, str, 10);
  for (i = 0; str[i] != '\0'; i++) ;
  str[i] = '.';
  fract = abs(fract);
  itoa(fract, str + i + 1, 10);
};

void showchar(char character, unsigned char color)
{
  int row, column;
  unsigned char thisrow;
  for (row = 0; row < charcelly; row++)
    {
      thisrow = charpattern[(character & 0x7F)][(8 * row) / charcelly];
      for (column = 0; column < charcellx; column++)
	{
	  if ( thisrow & (1 << ((8 * column) / charcellx)) )
	    putpixel(textcurx + charcellx - column, textcury + row, color);
	  else
	    {
	      if (GRTEXT_BGColor != GRTEXT_BGTRANSPARENT)
		putpixel(textcurx + charcellx - column, textcury + row, (unsigned char)GRTEXT_BGColor);
	    }
	}
    }
  textcurx += charcellx;
  if (textcurx > xres - charcellx - 1)
    {
      textcurx = 0;
      textcury += charcelly;
      if (textcury > yres - charcelly - 1)
	textcury = 0;
    }
};

void showstr(const char* stringz, unsigned char color)
{
  unsigned char chr;
  int i = 0, j;
  chr = stringz[i] & 0x7F;
  while (chr != '\0')
    {
      if (chr == '\n')
	{
	  textcurx = 0;
	  textcury += charcelly;
	  if (textcury > yres - charcelly - 2)
	    textcury = 0;
	}
      else
	{
	  if (chr == '\t')
	    {
	      for (j = GRTEXT_TABLENGTH; j > 0; j--)
		showchar(' ', 0);
	    }
	  else
	    showchar(chr, color);
	}
      i++;
      chr = stringz[i] & 0x7F;
    }
};

bool getcharpatternbit(int character, int x, int y)
{
  if (charpattern[character & 0x7F][y & 0x07] & (1 << (x & 0x07)))
    return true;
  else
    return false;
};

void xorcharpatternbit(int character, int x, int y, bool value)
{
  charpattern[character & 0x7F][y & 0x07] ^= (1 << (x & 0x07));
};

bool load8x8fontpattern(const char* szfilename)
{
  FILE* fontf = fopen(szfilename, "rb");
  if (fontf == 0)
    return false;
  unsigned long size;
  fseek(fontf, 0, SEEK_END);
  fgetpos(fontf, &size);
  fseek(fontf, 0, SEEK_SET);
  if (size != 1024)
    return false;
  fread((char*)charpattern, 1, 1024, fontf);
  fclose(fontf);
  return true;
};

void save8x8fontpattern(const char* szfilename)
{
  FILE* fontf = fopen(szfilename, "wb");
  fwrite((char*)charpattern, 1, 1024, fontf);
  fclose(fontf);
};
