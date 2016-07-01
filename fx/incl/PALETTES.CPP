//#define TEST_PALETTES

#include "palettes.h"
#include "fileio.h" // SafeIO
#include <pc.h>
#include <math.h> // for 'sqrt' in 'createshadinglookup'
#include "lcstdlib.h" // xmalloc/xfree


extern unsigned char NEAREST[15];

static void linear_interpolate_colors(char* pal, int firstIndex, int lastIndex, int r1, int g1, int b1, int r2, int g2, int b2);
static void cosine_interpolate_colors(char* pal, int firstIndex, int lastIndex, int r1, int g1, int b1, int r2, int g2, int b2);


unsigned char findnearestcolor(char* currentpal, RGBcolor rgb)
{
  return findnearestcolor(currentpal, rgb.red, rgb.green, rgb.blue);
};

unsigned char findnearestbasecolor(char* currentpal, char r, char g, char b)
{
  unsigned char index = findnearestcolor(currentpal, r, g, b);
  return (index / SHADES_PER_COLOR) * SHADES_PER_COLOR;
};

unsigned char findnearestbasecolor_8(char* currentpal, unsigned char r, unsigned char g, unsigned char b)
{
  unsigned char index = findnearestcolor_8(currentpal, r, g, b);
  return (index / SHADES_PER_COLOR) * SHADES_PER_COLOR;
};

/*unsigned char findnearestcolor_8(char* currentpal, unsigned char r, unsigned char g, unsigned char b)
{
int i;
unsigned char bestindex = 0; // suppresses warning
int red = (int)r, green = (int)g, blue = (int)b, errorr, errorg, errorb, palidx = 0;
unsigned error, lowesterror = 0xFFFFFFFF;
for (i = 0; i < 0x100; i++)
        {
        errorr = COLOR_6to8(currentpal[palidx++]) - red;
        errorg = COLOR_6to8(currentpal[palidx++]) - green;
        errorb = COLOR_6to8(currentpal[palidx++]) - blue;
        error = (errorr * errorr) + (errorg * errorg) + (errorb * errorb);
        if (error < lowesterror) // inequation:       sqrt(errorr^2 + errorg^2 + errorb^2)   <   sqrt(lowesterrorr^2 + lowesterrorg^2 + lowesterrorb^2)
                {                // is equivalent to: errorr^2 + errorg^2 + errorb^2         <   lowesterrorr^2 + lowesterrorg^2 + lowesterrorb^2
                bestindex = i;
                lowesterror = error;
                }
        }
return bestindex;
};*/

unsigned char findcontrastingindex(char* currentpal, unsigned char index)
{
  return findnearestcolor(currentpal, 63 - currentpal[index * 3], 63 - currentpal[(index * 3) + 1], 63 - currentpal[(index * 3) + 2]);
};// "proof": transform r, g, b such that the center of their coordinate system is at 0, 0, 0 (v' = v - 31.5)
// now consider a bounding box around this coordinate system of v'; to get a vector with equal
// magnitude (brightness), but opposite direction (mixture of colors) you simply have to negate it: v'_opposite = -v'
// then transforming v'opposite to v_opposity yields: v_opposite = -v' + 31.5 = -(v - 31.5) + 31.5 = 63 - v (what a useless proof)

char* findtransitiontable(char* destpal, char* sourcepal)
{
  char* res = (char*)xmalloc(0x100);
  int i;
  for (i = 0; i < 0x100; i++)
    res[i] = findnearestcolor(destpal, sourcepal[PAL_RED(i)], sourcepal[PAL_GREEN(i)], sourcepal[PAL_BLUE(i)]);
  return res;
};

unsigned char* createshadinglookup(char* pal, int noShades, unsigned char index)
{
  unsigned char* shades = (unsigned char*)xmalloc(noShades);
  float r, g, b, dr, dg, db, targetr, targetg, targetb, startr, startg, startb;
  r = (float)pal[PAL_RED(index)];
  g = (float)pal[PAL_GREEN(index)];
  b = (float)pal[PAL_BLUE(index)];
  float distfromblack, scale;
  distfromblack = sqrt(r * r + g * g + b * b); // maximal 3^(0.3333) * 2^4
  float newdistfromblack = distfromblack - (0.99F * distfromblack);
  float extdistfromblack = (0.05F * distfromblack);
  if (distfromblack != 0.0)
    scale = ((newdistfromblack > extdistfromblack) ? (newdistfromblack) : (extdistfromblack)) / distfromblack;
  else
    scale = 0.0F;
  targetr = scale * r;
  targetg = scale * g;
  targetb = scale * b;
  newdistfromblack = distfromblack + (0.99F * distfromblack);
  extdistfromblack = 63 * sqrt(3);
  if (distfromblack != 0.0)
    scale = ((newdistfromblack < extdistfromblack) ? (newdistfromblack) : (extdistfromblack)) / distfromblack;
  else
    scale = 0.0F;
  startr = scale * r;
  startg = scale * g;
  startb = scale * b;
  dr = (targetr - startr) / (float)noShades;
  dg = (targetg - startg) / (float)noShades;
  db = (targetb - startb) / (float)noShades;
  r = startr;
  g = startg;
  b = startb;
  int i;
  for (i = 0; i < noShades; i++)
    { // from white over index 2 black
      r += dr;
      g += dg;
      b += db;
      shades[noShades - 1 - i] = findnearestcolor(pal, (char)(r + 0.5F), (char)(g + 0.5F), (char)(b + 0.5F));
    }
  return shades;
};

void createContrastTable(char* pal, unsigned char* table)
{
  int i;
  for (i = 0; i < 0x100; i++)
    table[i] = findcontrastingindex(pal, i);
};

void updatecolors()
{
  char currentpal[0x300];
  getpal(currentpal);
  NEAREST[CCRED] = findnearestcolor(currentpal, 63, 0, 0);
  NEAREST[CCDARKRED] = findnearestcolor(currentpal, 20, 0, 0);
  NEAREST[CCGREEN] = findnearestcolor(currentpal, 0, 63, 0);
  NEAREST[CCDARKGREEN] = findnearestcolor(currentpal, 0, 20, 0);
  NEAREST[CCBLUE] = findnearestcolor(currentpal, 0, 0, 63);
  NEAREST[CCDARKBLUE] = findnearestcolor(currentpal, 0, 0, 20);
  NEAREST[CCYELLOW] = findnearestcolor(currentpal, 63, 63, 0);
  NEAREST[CCORANGE] = findnearestcolor(currentpal, 63, 32, 0);
  NEAREST[CCVIOLET] = findnearestcolor(currentpal, 63, 0, 63);
  NEAREST[CCCYAN] = findnearestcolor(currentpal, 0, 63, 63);
  NEAREST[CCBROWN] = findnearestcolor(currentpal, 32, 11, 11);
  NEAREST[CCWHITE] = findnearestcolor(currentpal, 63, 63, 63);
  NEAREST[CCGRAY] = findnearestcolor(currentpal, 32, 32, 32);
  NEAREST[CCDARKGRAY] = findnearestcolor(currentpal, 10, 10, 10);
  NEAREST[CCBLACK] = findnearestcolor(currentpal, 0, 0, 0);
};

void savepalette(const char* szfilename, char* pal, int colors)
{
  SafeIO palf(szfilename, "wb");
  char currentpal[0x300];
  if (pal == 0L)
    {
      getpal(currentpal);
      palf.writeb(currentpal, 0x300);
    }
  else
    palf.writeb(pal, colors * 3);
  palf.close();
};

int loadpalette(const char* szfilename, char** nullPpal)
{
  SafeIO palf(szfilename, "rb");
  unsigned long size = palf.size();
  if (size % 3)
    return 0;
  *nullPpal = (char*)xmalloc(size);
  palf.readb(*nullPpal, size);
  palf.close();
  return size / 3;
};

void createsmoothpal(char* pal)
{
  int i, r = 0, g = 0, b = 0, col = 0;
  outportb(0x03C8, 0);
  for(i = 0; i < 64; i++)      // BLACK - WHITE
    {
      pal[col++] = r;
      pal[col++] = g;
      pal[col++] = b;
      if (r < 63) r++;
      if (g < 63) g++;
      if (b < 63) b++;
    }
  for(i = 0; i < 64; i++)      // WHITE - YELLOW
    {
      pal[col++] = r;
      pal[col++] = g;
      pal[col++] = b;
      if (b > 0) b--;
    }
  for(i = 0; i < 64; i++)      // YELLOW - DARK BLUE
    {
      pal[col++]=r;
      pal[col++]=g;
      pal[col++]=b;
      if (r > 0) r--;
      if (g > 0) g--;
      if (b < 63 && !(i % 2)) b++;
    }
  for(i = 0; i < 64; i++)      // DARKBLUE - GREEN
    {
      pal[col++] = r;
      pal[col++] = g;
      pal[col++] = b;
      if (g < 63) g++;
      if (b > 0 && !(i % 2)) b--;
    }
  for(i = 0; i < 64; i++)      // GREEN - DARK RED
    {
      pal[col++] = r;
      pal[col++] = g;
      pal[col++] = b;
      if (r < 63 && !(i % 2)) r++;
      if (g > 0) g--;
    }
  for(i = 0; i < 64; i++)      // DARKRED - 63 00 31
    {
      pal[col++] = r;
      pal[col++] = g;
      pal[col++] = b;
      if (r < 63 && !(i % 2)) r++;
      if (b < 63 && !(i % 2)) b++;
    }
  for(i = 0; i < 64; i++)      // 63 00 31 - 00 31 63
    {
      pal[col++] = r;
      pal[col++] = g;
      pal[col++] = b;
      if (b < 63 && !(i % 2)) b++;
      if (g < 63 && !(i % 2)) g++;
      if (r > 0) r--;
    }
  for(i = 0; i < 64; i++)      // 00 31 63 - BLACK (00 00 00)
    {
      pal[col++] = r;
      pal[col++] = g;
      pal[col++] = b;
      if (b > 0) b--;
      if (g > 0 && !(i % 2)) g--;
    }
};

static void linear_interpolate_colors(char* pal, int firstIndex, int lastIndex, int r1, int g1, int b1, int r2, int g2, int b2)
{
  float dr, dg, db, r, g, b;
  int i;
  r = (float)COLOR_8to6(r1);
  g = (float)COLOR_8to6(g1);
  b = (float)COLOR_8to6(b1);
  dr = (COLOR_8to6(r2) - r) / (float)(lastIndex - firstIndex);
  dg = (COLOR_8to6(g2) - g) / (float)(lastIndex - firstIndex);
  db = (COLOR_8to6(b2) - b) / (float)(lastIndex - firstIndex); // not +1 ??
  /*if (dr < 0.0F) // this is done so the shades don't get darker
    dr = 0.0F;
    if (dg < 0.0F)
    dg = 0.0F;
    if (db < 0.0F)
    db = 0.0F;*/
  for (i = firstIndex; i <= lastIndex; i++)
    {
      pal[PAL_RED(i)] = (char)(r + 0.5F);
      pal[PAL_GREEN(i)] = (char)(g + 0.5F);
      pal[PAL_BLUE(i)] = (char)(b + 0.5F);
      if ((unsigned)pal[PAL_RED(i)] > 63)
	pal[PAL_RED(i)] = 63;
      if ((unsigned)pal[PAL_GREEN(i)] > 63)
	pal[PAL_GREEN(i)] = 63;
      if ((unsigned)pal[PAL_BLUE(i)] > 63)
	pal[PAL_BLUE(i)] = 63;
      r += dr;
      g += dg;
      b += db;
    }
};

static void cosine_interpolate_colors(char* pal, int firstIndex, int lastIndex, int r1, int g1, int b1, int r2, int g2, int b2)
{
  float dalpha, alpha = 1.5F * M_PI, rmax, gmax, bmax;
  rmax = (float)(COLOR_8to6(r2));
  gmax = (float)(COLOR_8to6(g2));
  bmax = (float)(COLOR_8to6(b2));
  dalpha = (M_PI / 2.0F)/ (float)(lastIndex - firstIndex + 1);
  int i;
  for (i = firstIndex; i <= lastIndex; i++)
    {
      if (alpha < M_PI)
	handleError(ErrorMessage("alpha < PI!", "", alpha * 1000.0F));
      pal[PAL_RED(i)] = (char)(-cos(alpha) * rmax);
      pal[PAL_GREEN(i)] = (char)(-cos(alpha) * gmax);
      pal[PAL_BLUE(i)] = (char)(-cos(alpha) * bmax);
      alpha -= dalpha;
    }
};

void createlinearshadepal(char* shade)
{
  const int baser = 30, baseg = 30, baseb = 30; // note that all colors are 8bit
  const float spacing = 0.05F;
  const int space = (int)((255.0F * (1.0F - spacing)) + 0.5F);
  linear_interpolate_colors(shade, 0, 15, baser, baseg, baseb, space, space, space);
  shade[0] = 0; shade[1] = 0; shade[2] = 0; // these are 6bit
  linear_interpolate_colors(shade, 16, 31, baser, baseg, baseb, space, 0, 0);
  linear_interpolate_colors(shade, 32, 47, baser, baseg, baseb, 0, space, 0);
  linear_interpolate_colors(shade, 48, 63, baser, baseg, baseb, 0, 0, space);
  // plane parallel to gr-plane
  linear_interpolate_colors(shade, 64, 79, baser, baseg, baseb, 64, 191, space);
  linear_interpolate_colors(shade, 80, 95, baser, baseg, baseb, 170, 170, space);
  linear_interpolate_colors(shade, 96, 111, baser, baseg, baseb, 85, 85, space);
  linear_interpolate_colors(shade, 112, 127, baser, baseg, baseb, 191, 64, space);
  // plane parallel to gb-plane
  linear_interpolate_colors(shade, 128, 143, baser, baseg, baseb, space, 64, 191);
  linear_interpolate_colors(shade, 144, 159, baser, baseg, baseb, space, 170, 170);
  linear_interpolate_colors(shade, 160, 175, baser, baseg, baseb, space, 191, 64);
  linear_interpolate_colors(shade, 176, 191, baser, baseg, baseb, space, 85, 85);
  // plane parallel to br-plane
  linear_interpolate_colors(shade, 192, 207, baser, baseg, baseb, 85, space, 85);
  linear_interpolate_colors(shade, 208, 223, baser, baseg, baseb, 191, space, 64);
  linear_interpolate_colors(shade, 224, 239, baser, baseg, baseb, 64, space, 191);
  linear_interpolate_colors(shade, 240, 255, baser, baseg, baseb, 170, space, 170);
};

void createshadepal(char* shade)
{
  const int baser = 30, baseg = 30, baseb = 30; // note that all colors are 8bit
  const float spacing = 0.05F;
  const int space = (int)((255.0F * (1.0F - spacing)) + 0.5F);
  cosine_interpolate_colors(shade, 0, 15, baser, baseg, baseb, space, space, space);
  shade[0] = 0; shade[1] = 0; shade[2] = 0; // these are 6bit
  cosine_interpolate_colors(shade, 16, 31, baser, baseg, baseb, space, 0, 0);
  cosine_interpolate_colors(shade, 32, 47, baser, baseg, baseb, 0, space, 0);
  cosine_interpolate_colors(shade, 48, 63, baser, baseg, baseb, 0, 0, space);
  // plane parallel to gr-plane
  cosine_interpolate_colors(shade, 64, 79, baser, baseg, baseb, 64, 191, space);
  cosine_interpolate_colors(shade, 80, 95, baser, baseg, baseb, 170, 170, space);
  cosine_interpolate_colors(shade, 96, 111, baser, baseg, baseb, 85, 85, space);
  cosine_interpolate_colors(shade, 112, 127, baser, baseg, baseb, 191, 64, space);
  // plane parallel to gb-plane
  cosine_interpolate_colors(shade, 128, 143, baser, baseg, baseb, space, 64, 191);
  cosine_interpolate_colors(shade, 144, 159, baser, baseg, baseb, space, 170, 170);
  cosine_interpolate_colors(shade, 160, 175, baser, baseg, baseb, space, 191, 64);
  cosine_interpolate_colors(shade, 176, 191, baser, baseg, baseb, space, 85, 85);
  // plane parallel to br-plane
  cosine_interpolate_colors(shade, 192, 207, baser, baseg, baseb, 85, space, 85);
  cosine_interpolate_colors(shade, 208, 223, baser, baseg, baseb, 191, space, 64);
  cosine_interpolate_colors(shade, 224, 239, baser, baseg, baseb, 64, space, 191);
  cosine_interpolate_colors(shade, 240, 255, baser, baseg, baseb, 170, space, 170);
};

void rotatepaldown(char* pal, int first, int last)
{
  unsigned char r, g, b;
  int i;
  r = pal[(first * 3) + 0];             //save the first color in r,g,b
  g = pal[(first * 3) + 1];
  b = pal[(first * 3) + 2];
  for (i = (first * 3); i < ((last + 1) * 3); i++)
    pal[i] = pal[i + 3]; //move all colors down one step (3 bytes)
  pal[(last * 3) + 0] = r;                //set the last color to r,g,b
  pal[(last * 3) + 1] = g;
  pal[(last * 3) + 2] = b;
  waitVR();                       //wait for the vertical retrace
  setpal(pal);                    //and set the first 256 colours of pal
  updatecolors();
};

void rotatepalup(char* pal, int first, int last)
{
  unsigned char r, g, b;
  int i;
  r = pal[(last * 3)];             //save the last color in r,g,b
  g = pal[(last * 3) + 1];
  b = pal[(last * 3) + 2];
  for (i = (last * 3); i > ((first + 1) * 3); i--)
    pal[i] = pal[i - 3];        //move all colors up one step (3 bytes)
  pal[(first * 3)] = r;                //set the first color to r,g,b
  pal[(first * 3) + 1] = g;
  pal[(first * 3) + 2] = b;
  waitVR();
  setpal(pal);
  updatecolors();
};

void fadepalinto(char* newpal, PAL_FADE_SPEED speed)
{
  int i, ii;
  char currentpal[0x300];
  getpal(currentpal);
  for (i = 64 / speed; i > 0; i--)
    {
      //        outportb(0x03C8, 0);
      for (ii = 0x2FF; ii >= 0; ii--)
	{
	  //                outportb(0x03C9, currentpal[ii]);
	  if (currentpal[ii] < newpal[ii])
	    currentpal[ii] += (currentpal[ii] + speed <= newpal[ii]) ? (speed) : (currentpal[ii] - newpal[ii]);
	  if (currentpal[ii] > newpal[ii])
	    currentpal[ii] -= (currentpal[ii] - speed >= newpal[ii]) ? (speed) : (newpal[ii] - currentpal[ii]);
	}
      waitVR();
      setpal(currentpal);
    }
  waitVR();
  setpal(newpal); // to be sure the pal is exact
};

void fadepalout(PAL_FADE_SPEED speed)
{
  int i, ii;
  char currentpal[0x300];
  getpal(currentpal);
  for (i = 64 / speed; i > 0; i--)    // 64 steps
    {
      //        outportb(0x03C8, 0);
      for (ii = 0x2FF; ii >= 0; ii--)
	{
	  //                outportb(0x03C9, currentpal[ii]);
	  if (currentpal[ii] > 0) currentpal[ii] -= (currentpal[ii] >= speed) ? (speed) : (currentpal[ii]);
	}
      waitVR();
      setpal(currentpal);
    }
};

#ifdef TEST_PALETTES

#define WIDTH	        100
#define SHADEWIDTH      20

#include "vbetwo.h"
#include "grtext.h"

unsigned char* shadelookups[256];

void drawpal(int select)
{
  int i, j;
  for (i = 0; i < 0x100; i++)
    {
      hline(100, 100 + WIDTH, 100 + i, i);
      for (j = SHADES_PER_COLOR - 1; j >= 0; j--)
	hline(100 + WIDTH + ((SHADES_PER_COLOR - j) * SHADEWIDTH), 100 + WIDTH + ((SHADES_PER_COLOR - j + 1) * SHADEWIDTH), 100 + i, *(shadelookups[i] + j));
    }
  hline(70, 120, 100 + select, NEAREST[CCWHITE]);
};

#include "keyinp.h"
#include <stdio.h>

int main(int noargs, char** args)
{
  /*if (noargs != 2)
    {
    printf("SYNTAX: PALETTES [filename.pal]");
    return 0;
    }*/
  char* pal = (char*)xmalloc(0x300);
  //createlinearshadepal(pal);
  createshadepal(pal);
  int i;
  for (i = 0; i < 0x100; i++)
    shadelookups[i] = createshadinglookup(pal, SHADES_PER_COLOR, i);
  if (!openVBE(800, 600))
    return 1;
  installkeyhandler();
  setpal(pal);
  int index = 0;
  char str[100];
  settextsize(8);
  updatecolors();
  while (!getkey(KEY_ESC))
    {
      clearoffscreenmem();
      if (getkey(KEY_PLUS) && index < 255)
	index++;
      if (getkey(KEY_MINUS) && index > 0)
	index--;
      if (getkey(KEY_SPACE))
	index = findnearestbasecolor(pal, pal[PAL_RED(index)], pal[PAL_GREEN(index)], pal[PAL_BLUE(index)]);
      drawpal(index);
      fillrect(0, 0, 70, 50, index);
      for (i = SHADES_PER_COLOR - 1; i >= 0; i--)
	fillrect(70 + ((SHADES_PER_COLOR - i) * 10), 0, 70 + ((SHADES_PER_COLOR - i + 1) * 10), 50, *(shadelookups[index] + i));
      showstrxy(120, 20, "Index ", NEAREST[CCWHITE]);
      itoa(index, str, 10);
      showstr(str, NEAREST[CCYELLOW]);
      showstr(" - RGB: (", NEAREST[CCWHITE]);
      itoa(pal[PAL_RED(index)], str, 10);
      showstr(str, NEAREST[CCYELLOW]);
      showstr(", ", NEAREST[CCWHITE]);
      itoa(pal[PAL_GREEN(index)], str, 10);
      showstr(str, NEAREST[CCYELLOW]);
      showstr(", ", NEAREST[CCWHITE]);
      itoa(pal[PAL_BLUE(index)], str, 10);
      showstr(str, NEAREST[CCYELLOW]);
      showstr(")", NEAREST[CCWHITE]);
      flipoffscreenmem();
    };
  savepalette("shadecos.pal", pal);
  xfree(pal);
  restorekeyhandler();
  closeVBE();
  return 0;
}

#endif
