#include "vbetwo.h"
#include "palettes.h"
#include "grtext.h"
#include "gformats.h"
#include "fifo99.h"
#include "fileio.h"
//#include "timer.h"
#include "lcstdlib.h" // xmalloc/xfree
#include <string.h>
#include <ctype.h>
#include <math.h>

const NEARESTCC tokencol = CCWHITE, textcol = CCGRAY, instrcol = CCDARKGRAY,
  palptrcol = CCWHITE, scrollblockcol = CCGRAY, scrollbarframecol = CCDARKGRAY,
  scrollbarfillcol = CCDARKGRAY, mainframecol = CCDARKGRAY, viewwholescrframecol = CCWHITE,
  msgcol = CCDARKGREEN, msgtokencol = CCWHITE;

const int PALWIDTH = 50, FRAMEPALSPACING = 10, TOPINFOSPACE = 4 * 10,
  SCROLLBARSIZE = 20, INSTRTEXTSPACE = 0, MIN_VISUALBMW = 2,
  MIN_VISUALBMH = 2, MSG_LEN = 50, INSTRSPACING = 20, custompal = 3;
const int MIN_CELLX = 2, MIN_CELLY = 2;
int newbmw, newbmh, scrollx, scrolly, cellx, celly;
int MINX, MINY, MAXX, MAXY, BMWIDTH, BMHEIGHT, rasterx, rastery,
  VISUALBMW, VISUALBMH, MAX_VISUALX, MAX_VISUALY;
char localstr[33];
char szcustompal[13];
char* bitmap, *newimage;
char* pal[4];
unsigned char rastercol = 0, currentpal = 0, caretvisible = 1, drawcol = 0x80,
  raster_squared = 0;
char szfilename[13];
const PAL_FADE_SPEED fadespeed = FAST;

// prototypes
void showmsg(const char* szmsg);
void showmsgpart(const char* szmsgpart);
PAL_MODE ask_palmode();
bool ask_load_it();
void showpal();
void drawcell(int x, int y, unsigned char color);
void drawraster(unsigned char color);
void fillviewport();
void showmainframe();
void initraster(int scrX, int scrY, int visX, int visY);
void drawinfo();
void drawframe();
bool readfilename(const char* context, const char* ending, bool trying_to_load);
void selectnc();
void view_image_wholescreen();
void load_n_rearrange();
void change_size();
inline void drawcaret(unsigned char color);
void countcolors();
void invert_colors();
void load_sprite();
void save_sprite();
void save_DIB();
void load_DIB();
void redraw();

// definitions

inline void showmsgpart(const char* szmsgpart)
{
  settextsize(8, 8);
  showstrxy(0, 30, szmsgpart, NEAREST[msgcol]);
};

void showmsg(const char* szmsg)
{
  int i;
  settextsize(8, 8);
  showstrxy(0, 30, szmsg, NEAREST[msgcol]);
  for (i = MSG_LEN - strlen(szmsg); i > 0; i--)
    showstr(" ", 0);
};

PAL_MODE ask_palmode()
{
  showmsg("");
  showmsgpart("");
  showstr("T", NEAREST[msgtokencol]);
  showstr("ranslate or ", NEAREST[msgcol]);
  showstr("U", NEAREST[msgtokencol]);
  showstr("se palette (T/U) ?", NEAREST[msgcol]);
  showstr("\t\t", 0);
  flipoffscreenmem();
  int t, u;
  do
    {
      t = getkey(KEY_T);
      u = getkey(KEY_U);
    } while (!t && !u);
  if (t)
    return TRANSLATE_PAL;
  else
    return USE_PAL;
};

bool ask_scale_it()
{
  int i, ii;
  showmsg("");
  showmsgpart(szfilename);
  showstr(" is (", NEAREST[msgcol]);
  itoa(newbmw, localstr, 10);
  showstr(localstr, NEAREST[msgtokencol]);
  showstr("x", NEAREST[msgcol]);
  itoa(newbmh, localstr, 10);
  showstr(localstr, NEAREST[msgtokencol]);
  showstr("), ", NEAREST[msgcol]);
  showstr("s", NEAREST[msgtokencol]);
  showstr("cale it to (", NEAREST[msgcol]);
  itoa(BMWIDTH, localstr, 10);
  showstr(localstr, NEAREST[msgtokencol]);
  showstr("x", NEAREST[msgcol]);
  itoa(BMHEIGHT, localstr, 10);
  showstr(localstr, NEAREST[msgtokencol]);
  showstr(") or ", NEAREST[msgcol]);
  showstr("c", NEAREST[msgtokencol]);
  showstr("hange size of buffer ? (S/C)", NEAREST[msgcol]);
  showstr("\t", 0);
  flipoffscreenmem(); //SIGSEGV!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  do	{
    i = getkey(KEY_S);
    ii = getkey(KEY_C) || getkey(KEY_ESC);
  } while (!i && !ii);
  if (i)
    return true;
  else
    return false;
};

void showpal()
{
  int row;
  for (row = 0; row < 0x100; row++)
    hline(xres - PALWIDTH, xres, row, (unsigned char)row);
  hline(xres - PALWIDTH, xres - PALWIDTH + 5, drawcol, NEAREST[palptrcol]);
};

inline void drawcell(int x, int y, unsigned char color)
{
  fillrect(MINX + (x * cellx) + 1, MINY + (y * celly) + 1, (int)cellx - 2, (int)celly - 2, color);
};

void drawraster(unsigned char color)
{
  int loop;
  for (loop = 0; loop <= VISUALBMH; loop++)
    hline(MINX, MINX + (cellx * VISUALBMW), MINY + (loop * celly), color);
  for (loop = 0; loop <= VISUALBMW; loop++)
    vline(MINY, MINY + (celly * VISUALBMH), MINX + (loop * cellx), color);
};

void fillviewport()
{
  int x, y;
  int k;
  for (y = 0; y < VISUALBMH; y++)
    for (x = 0; x < VISUALBMW; x++)
      {
	k = 4 + ((y + scrolly) * BMWIDTH) + x + scrollx;
	drawcell(x, y, bitmap[k]);
      }
};

void showmainframe()
{
  rectangle(MINX - 1, MINY - 1, (VISUALBMW * cellx) + 2, (VISUALBMH * celly) + 2, NEAREST[mainframecol]);
  rectangle(MINX + (VISUALBMW * cellx) + 1, MINY - 1, SCROLLBARSIZE,  (VISUALBMH * celly) + 2, NEAREST[scrollbarframecol]);
  rectangle(MINX - 1, MINY + (VISUALBMH * celly) + 1, (VISUALBMW * cellx) + 2, SCROLLBARSIZE, NEAREST[scrollbarframecol]);
  fillrect(MINX, MINY + (VISUALBMH * celly) + 2,
	   (VISUALBMW * cellx), SCROLLBARSIZE - 2, NEAREST[scrollbarfillcol]);
  fillrect(MINX + (VISUALBMW * cellx) + 2, MINY,
	   SCROLLBARSIZE - 2, (VISUALBMH * celly), NEAREST[scrollbarfillcol]);
  int start; // startpoint and length in pels from MINX
  float len1;
  len1 = (VISUALBMW * cellx) / (float)(BMWIDTH - VISUALBMW + 1);
  start = (int)(len1 * scrollx);
  fillrect(MINX + start, MINY + (VISUALBMH * celly) + 2,
	   (int)len1, SCROLLBARSIZE - 2,
	   NEAREST[scrollblockcol]); //HSCROLLBAR
  len1 = (VISUALBMH * celly) / (float)(BMHEIGHT - VISUALBMH + 1);
  start = (int)(len1 * scrolly);
  fillrect(MINX + (VISUALBMW * cellx) + 2, MINY + start,
	   SCROLLBARSIZE - 2, (int)len1,
	   NEAREST[scrollblockcol]); // VSCROLLBAR
};

void initraster(int scrX, int scrY, int visX, int visY)
{
  if (visX > BMWIDTH)
    VISUALBMW = BMWIDTH;
  else
    VISUALBMW = visX;
  if (visY > BMHEIGHT)
    VISUALBMH = BMHEIGHT;
  else
    VISUALBMH = visY;
  if (VISUALBMW > MAX_VISUALX)
    VISUALBMW = MAX_VISUALX;
  if (VISUALBMH > MAX_VISUALY)
    VISUALBMH = MAX_VISUALY;
  cellx = (int)((MAXX - MINX) / (float)VISUALBMW);
  celly = (int)((MAXY - MINY) / (float)VISUALBMH);
  if (raster_squared)
    {
      if (cellx > celly)
	cellx = celly;
      else
	celly = cellx;
    }
  if (scrX < BMWIDTH - VISUALBMW)
    scrollx = scrX;
  else
    scrollx = BMWIDTH - VISUALBMW;
  if (scrY < BMHEIGHT - VISUALBMH)
    scrolly = scrY;
  else
    scrolly = BMHEIGHT - VISUALBMH;
  rasterx = 0;
  rastery = 0;
};

void drawinfo()
{
  int i, j;
  char r, g, b;
  settextsize(8, 8);
  if (currentpal == 0)
    showstrxy(0, 0, "universal pal index ", NEAREST[CCGRAY]);
  if (currentpal == 1)
    showstrxy(0, 0, "linear shadepal index ", NEAREST[CCGRAY]);
  if (currentpal == 2)
    showstrxy(0, 0, "cosine shadepal index ", NEAREST[CCGRAY]);
  if (currentpal == custompal)
    {
      showstrxy(0, 0, "'", NEAREST[CCGRAY]);
      showstr(szcustompal, NEAREST[msgtokencol]);
      showstr("' index ", NEAREST[CCGRAY]);
    }
  itoa(drawcol, localstr, 10);
  showstr(localstr, NEAREST[CCBROWN]);
  showstr("\tMAKEBM V10^(-7)\n", NEAREST[CCORANGE]);
  showstrxy(0, 10, " X = ", NEAREST[textcol]);
  itoa(rasterx, localstr, 10);
  showstr(localstr, NEAREST[tokencol]);
  showstr(" Y = ", NEAREST[textcol]);
  itoa(rastery, localstr, 10);
  showstr(localstr, NEAREST[tokencol]);
  showstr(" y*maxx + x = ", NEAREST[textcol]);
  j = ((rastery + scrolly) * BMWIDTH) + rasterx + scrollx;
  itoa(j, localstr, 10);
  showstr(localstr, NEAREST[tokencol]);
  showstr(" bitmap[i] = ", NEAREST[textcol]);
  itoa((unsigned char)bitmap[4 + j], localstr, 10);
  showstr(localstr, NEAREST[tokencol]);
  showstr(" = 0x", NEAREST[textcol]);
  i = ((int)bitmap[4 + j]) & 0x00FF;
  itoa(i, localstr, 16);
  showstr(localstr, NEAREST[tokencol]);
  showstr(" [", NEAREST[textcol]);
  itoa(*(pal[currentpal] + PAL_RED(i)), localstr, 10);
  showstr(localstr, NEAREST[tokencol]);
  showstr(",", NEAREST[textcol]);
  itoa(*(pal[currentpal] + PAL_GREEN(i)), localstr, 10);
  showstr(localstr, NEAREST[tokencol]);
  showstr(",", NEAREST[textcol]);
  itoa(*(pal[currentpal] + PAL_BLUE(i)), localstr, 10);
  showstr(localstr, NEAREST[tokencol]);
  showstr("]\t", NEAREST[textcol]);
  itoa(scrollx, localstr, 10);
  showstrxy(0, 20, "scrollX = ", NEAREST[textcol]);
  showstr(localstr, NEAREST[tokencol]);
  itoa(scrolly, localstr, 10);
  showstr(" scrollY = ", NEAREST[textcol]);
  showstr(localstr, NEAREST[tokencol]);
  itoa(VISUALBMW, localstr, 10);
  showstr(" visualX = ", NEAREST[textcol]);
  showstr(localstr, NEAREST[tokencol]);
  itoa(VISUALBMH, localstr, 10);
  showstr(" visualY = ", NEAREST[textcol]);
  showstr(localstr, NEAREST[tokencol]);
  showstr(" '", NEAREST[textcol]);
  showstr(szfilename, NEAREST[tokencol]);
  showstr("' (", NEAREST[textcol]);
  itoa(BMWIDTH, localstr, 10);
  showstr(localstr, NEAREST[tokencol]);
  showstr("x", NEAREST[textcol]);
  itoa(BMHEIGHT, localstr, 10);
  showstr(localstr, NEAREST[tokencol]);
  showstr(")\t", NEAREST[textcol]);
  r = *(pal[currentpal] + PAL_RED(drawcol));
  g = *(pal[currentpal] + PAL_GREEN(drawcol));
  b = *(pal[currentpal] + PAL_BLUE(drawcol));
  settextsize(PALWIDTH / 3, 10);
  showstrxy(xres - PALWIDTH, 256, "rgb", 70);
  settextsize(PALWIDTH / 2 - 1, 10);
  itoa(r, localstr, 10);
  if (strlen(localstr) == 1)
    {
      localstr[2] = localstr[0];
      localstr[0] = '0';
      localstr[1] = localstr[2];
      localstr[2] = '\0';
    }
  showstrxy(xres - PALWIDTH, 300, localstr, NEAREST[CCBROWN]);
  itoa(g, localstr, 10);
  if (strlen(localstr) == 1)
    {
      localstr[2] = localstr[0];
      localstr[0] = '0';
      localstr[1] = localstr[2];
      localstr[2] = '\0';
    }
  showstrxy(xres - PALWIDTH, 310, localstr, NEAREST[CCBROWN]);
  itoa(b, localstr, 10);
  if (strlen(localstr) == 1)
    {
      localstr[2] = localstr[0];
      localstr[0] = '0';
      localstr[1] = localstr[2];
      localstr[2] = '\0';
    }
  showstrxy(xres - PALWIDTH, 320, localstr, NEAREST[CCBROWN]);
};

void redraw()
{
  fadepalout(fadespeed);
  clearoffscreenmem();
  flipoffscreenmem();
  setpal(pal[currentpal]);
  updatecolors();
  setblackpal();
  showpal();
  showmainframe();
  drawinfo();
  drawcaret(drawcol);
  fillviewport();
  drawraster(rastercol);
  flipoffscreenmem();
  fadepalinto(pal[currentpal], fadespeed);
};

bool readfilename(const char* context, const char* ending, bool trying_to_load)
{
  int i = 0, oldi = -1;
  szfilename[0] = '\0';
  bool escapism = false, positive;
  clearallkeys();
  while ((!getkey(KEY_ENTER) || i == 0) && !escapism)
    {
      if (getkey(KEY_A) && i < 8) szfilename[i++] = 'A';
      if (getkey(KEY_B) && i < 8) szfilename[i++] = 'B';
      if (getkey(KEY_C) && i < 8) szfilename[i++] = 'C';
      if (getkey(KEY_D) && i < 8) szfilename[i++] = 'D';
      if (getkey(KEY_E) && i < 8) szfilename[i++] = 'E';
      if (getkey(KEY_F) && i < 8) szfilename[i++] = 'F';
      if (getkey(KEY_G) && i < 8) szfilename[i++] = 'G';
      if (getkey(KEY_H) && i < 8) szfilename[i++] = 'H';
      if (getkey(KEY_I) && i < 8) szfilename[i++] = 'I';
      if (getkey(KEY_J) && i < 8) szfilename[i++] = 'J';
      if (getkey(KEY_K) && i < 8) szfilename[i++] = 'K';
      if (getkey(KEY_L) && i < 8) szfilename[i++] = 'L';
      if (getkey(KEY_M) && i < 8) szfilename[i++] = 'M';
      if (getkey(KEY_N) && i < 8) szfilename[i++] = 'N';
      if (getkey(KEY_O) && i < 8) szfilename[i++] = 'O';
      if (getkey(KEY_P) && i < 8) szfilename[i++] = 'P';
      if (getkey(KEY_Q) && i < 8) szfilename[i++] = 'Q';
      if (getkey(KEY_R) && i < 8) szfilename[i++] = 'R';
      if (getkey(KEY_S) && i < 8) szfilename[i++] = 'S';
      if (getkey(KEY_T) && i < 8) szfilename[i++] = 'T';
      if (getkey(KEY_U) && i < 8) szfilename[i++] = 'U';
      if (getkey(KEY_V) && i < 8) szfilename[i++] = 'V';
      if (getkey(KEY_W) && i < 8) szfilename[i++] = 'W';
      if (getkey(KEY_X) && i < 8) szfilename[i++] = 'X';
      if (getkey(KEY_Y) && i < 8) szfilename[i++] = 'Y';
      if (getkey(KEY_Z) && i < 8) szfilename[i++] = 'Z';
      if (getkey(KEY_1) && i < 8) szfilename[i++] = '1';
      if (getkey(KEY_2) && i < 8) szfilename[i++] = '2';
      if (getkey(KEY_3) && i < 8) szfilename[i++] = '3';
      if (getkey(KEY_4) && i < 8) szfilename[i++] = '4';
      if (getkey(KEY_5) && i < 8) szfilename[i++] = '5';
      if (getkey(KEY_6) && i < 8) szfilename[i++] = '6';
      if (getkey(KEY_7) && i < 8) szfilename[i++] = '7';
      if (getkey(KEY_8) && i < 8) szfilename[i++] = '8';
      if (getkey(KEY_9) && i < 8) szfilename[i++] = '9';
      if (getkey(KEY_SPACE) && i < 8) szfilename[i++] = '0';
      if (getkey(KEY_BACK))
	if (i > 0) szfilename[--i] = '\0';
      if (i != oldi)
	{
	  szfilename[i] = '\0';
	  showmsgpart("Enter file ");
	  showstr(context, NEAREST[msgcol]);
	  showstr(": ", NEAREST[msgcol]);
	  showstr(szfilename, NEAREST[msgtokencol]);
	  showstr(".", NEAREST[msgtokencol]);
	  showstr(ending, NEAREST[msgtokencol]);
	  szfilename[i] = '.';
	  szfilename[i + 1] = ending[0];
	  szfilename[i + 2] = ending[1];
	  szfilename[i + 3] = ending[2];
	  szfilename[i + 4] = '\0';
	  if (trying_to_load)
	    positive = fexists(szfilename);
	  else
	    positive = !fexists(szfilename);
	  if (positive)
	    showstr("  =-)\t\t", NEAREST[msgtokencol]);
	  else
	    showstr("  =-(\t\t", NEAREST[msgtokencol]);
	  flipoffscreenmem();
	}
      oldi = i;
      if (getkey(KEY_ESC))
	escapism = true;
    }
  if (escapism)
    {
      showmsg("");
      szfilename[0] = '\0';
    }
  return !escapism;
};

void selectnc()
{
  unsigned char rgb[3] = {0, 0, 0};
  unsigned char progress = 0;
  unsigned long temp = 256L;
  int enter8bit = 0;
  int i = 0, oldi = 255;
  localstr[0] = '\0';
  while (progress <= 2)
    {
      if (getkey(KEY_1) && i < 3) localstr[i++] = '1';
      if (getkey(KEY_2) && i < 3) localstr[i++] = '2';
      if (getkey(KEY_3) && i < 3) localstr[i++] = '3';
      if (getkey(KEY_4) && i < 3) localstr[i++] = '4';
      if (getkey(KEY_5) && i < 3) localstr[i++] = '5';
      if (getkey(KEY_6) && i < 3) localstr[i++] = '6';
      if (getkey(KEY_7) && i < 3) localstr[i++] = '7';
      if (getkey(KEY_8) && i < 3) localstr[i++] = '8';
      if (getkey(KEY_9) && i < 3) localstr[i++] = '9';
      if (getkey(KEY_0) && i < 3) localstr[i++] = '0';
      if (getkey(KEY_BACK))
	if (i > 0) localstr[--i] = '\0';
      if (oldi != i)
	{
	  localstr[i] = '\0';
	  switch (progress)
	    {
	    case 0: showmsgpart("finding color -> enter red value: ");
	      break;
	    case 1: showmsgpart("finding color -> enter blue value: ");
	      break;
	    case 2: showmsgpart("finding color -> enter green value: ");
	      break;
	    }
	  showstr(localstr, NEAREST[msgtokencol]);
	  if (progress == 0)
	    oldi = NEAREST[msgtokencol];
	  else
	    oldi = NEAREST[msgcol];
	  if (enter8bit)
	    showstr(" [8bit] (find your own NC!)", oldi);
	  else
	    showstr(" [6bit] (find your own NC!)", oldi);
	  showstr("\t", 0);
	  flipoffscreenmem();
	  temp = strtoul(localstr, 0L, 10);
	  rgb[progress] = (unsigned char)temp;
	  oldi = i;
	}
      if (getkey(KEY_ENTER) && temp <= 255L)
	{
	  if (rgb[progress] > 63)
	    enter8bit = 1;
	  progress++;
	  i = 0;
	  oldi = 255;
	  localstr[0] = '\0';
	}
      if (getkey(KEY_SPACE) && progress == 0)
	{
	  enter8bit ^= 1;
	  oldi = ~i;
	}
    }
  if (enter8bit)
    drawcol = findnearestcolor_8(pal[currentpal], rgb[0], rgb[1], rgb[2]);
  else
    drawcol = findnearestcolor(pal[currentpal], (char)rgb[0], (char)rgb[1], (char)rgb[2]);
  showmsg("");
  showmsgpart("Index found: ");
  itoa(drawcol, localstr, 10);
  showstr(localstr, NEAREST[msgtokencol]);
  showpal();
};

void view_image_wholescreen()
{
  clearoffscreenmem();
  const int wholeMINX = 1, wholeMINY = 1 + 8,
    wholeMAXX = xres - 1, wholeMAXY = yres - 1;
  const int picx = (wholeMAXX - wholeMINX) / 2 - (BMWIDTH / 2), picy = (wholeMAXY - wholeMINY) / 2 - (BMHEIGHT / 2);
  putimage(picx, picy, bitmap);
  rectangle(picx - 1, picy - 1, BMWIDTH + 1, BMHEIGHT + 1, NEAREST[viewwholescrframecol]); // frame
  rectangle(picx + scrollx, picy + scrolly, VISUALBMW - 1, VISUALBMH - 1, drawcol); // visual FRAME
  settextsize(8);
  while (!getkey(KEY_ENTER) && !getkey(KEY_ESC))
    {
      if (getkey(KEY_PAD6))
	{
	  if (peekkey(KEY_RSHIFT))
	    {
	      if (VISUALBMW < MAX_VISUALX)
		VISUALBMW++;
	    }
	  else
	    {
	      if (scrollx < BMWIDTH - VISUALBMW)
		scrollx++;
	    }
	}
      if (getkey(KEY_PAD2))
	{
	  if (peekkey(KEY_RSHIFT))
	    {
	      if (VISUALBMH < MAX_VISUALY)
		VISUALBMH++;
	    }
	  else
	    {
	      if (scrolly < BMHEIGHT - VISUALBMH)
		scrolly++;
	    }
	}
      if (getkey(KEY_PAD4))
	{
	  if (scrollx > 0)
	    scrollx--;
	  if (peekkey(KEY_RSHIFT))
	    {
	      if (VISUALBMW > MIN_VISUALBMW)
		VISUALBMW--;
	    }
	}
      if (getkey(KEY_PAD8))
	{
	  if (scrolly > 0)
	    scrolly--;
	  if (peekkey(KEY_RSHIFT))
	    {
	      if (VISUALBMH > MIN_VISUALBMH)
		VISUALBMH--;
	    }
	}
      if (getkey(KEY_PAD5))
	{
	  scrollx = 0;
	  scrolly = 0;
	  VISUALBMW = (BMWIDTH / 2 <? MAX_VISUALX);
	  VISUALBMH = (BMHEIGHT / 2 <? MAX_VISUALY);
	}
      if (getkey(KEY_PAD3))
	{
	  scrollx = 0;
	  scrolly = 0;
	  VISUALBMW = (BMWIDTH <? MAX_VISUALX);
	  VISUALBMH = (BMHEIGHT <? MAX_VISUALY);
	}
      if (getkey(KEY_S))
	raster_squared ^= 1;
      putimage(picx, picy, bitmap);
      rectangle(picx + scrollx, picy + scrolly, VISUALBMW - 1, VISUALBMH - 1, drawcol); // visual FRAME
      showstrxy(1, 1, "scrollX = ", NEAREST[CCGRAY]);
      itoa(scrollx, localstr, 10);
      showstr(localstr, NEAREST[CCWHITE]);
      showstr(" scrollY = ", NEAREST[CCGRAY]);
      itoa(scrolly, localstr, 10);
      showstr(localstr, NEAREST[CCWHITE]);
      showstr(" visualX = ", NEAREST[CCGRAY]);
      itoa(VISUALBMW, localstr, 10);
      showstr(localstr, NEAREST[CCWHITE]);
      showstr(" visualY = ", NEAREST[CCGRAY]);
      itoa(VISUALBMH, localstr, 10);
      showstr(localstr, NEAREST[CCWHITE]);
      showstr(" (", NEAREST[CCGRAY]);
      floatstr(100.0 * (VISUALBMW * VISUALBMH) /(float)(BMWIDTH * BMHEIGHT), localstr, 2);
      showstr(localstr, NEAREST[CCWHITE]);
      showstr("%) squared: ", NEAREST[CCGRAY]);
      if (raster_squared)
	showstr("yes", NEAREST[CCWHITE]);
      else
	showstr("no", NEAREST[CCWHITE]);
      showstr("\t\t", 0);
      flipoffscreenmem();
    }
  initraster(scrollx, scrolly, VISUALBMW, VISUALBMH);
  redraw();
};

void scale_bitmap()
{
  bool valid_factor = false, escapism = false, plusminuschanged = false;
  float factor = 100.0F;
  floatstr(factor, localstr, 1);
  int i = 5, oldi = 0;
  char tempstr[31];
  while ((!getkey(KEY_ENTER) || !valid_factor) && !escapism)
    {
      if (getkey(KEY_1)) localstr[i++] = '1';
      if (getkey(KEY_2)) localstr[i++] = '2';
      if (getkey(KEY_3)) localstr[i++] = '3';
      if (getkey(KEY_4)) localstr[i++] = '4';
      if (getkey(KEY_5)) localstr[i++] = '5';
      if (getkey(KEY_6)) localstr[i++] = '6';
      if (getkey(KEY_7)) localstr[i++] = '7';
      if (getkey(KEY_8)) localstr[i++] = '8';
      if (getkey(KEY_9)) localstr[i++] = '9';
      if (getkey(KEY_0)) localstr[i++] = '0';
      if (getkey(KEY_PADPOINT)) localstr[i++] = '.';
      if (getkey(KEY_BACK))
	if (i > 0) localstr[--i] = '\0';
      if (peekkey(KEY_PLUS))
	{
	  floatstr(factor + 1.0F, localstr, 1);
	  plusminuschanged = true;
	}
      if (peekkey(KEY_MINUS))
	{
	  floatstr(factor - 1.0F, localstr, 1);
	  plusminuschanged = true;
	}
      if (getkey(KEY_ESC))
	escapism = true;
      if (oldi != i || plusminuschanged)
	{
	  showmsg("");
	  showmsgpart("scaling bitmap by ");
	  localstr[i] = '\0';
	  sscanf(localstr, "%f", &factor);
	  floatstr(factor, localstr, 1);
	  showstr(localstr, NEAREST[msgtokencol]);
	  newbmw = (int)((0.01F * factor * BMWIDTH) + 0.5F);
	  newbmh = (int)((0.01F * factor * BMHEIGHT) + 0.5F);
	  showstr("% (", NEAREST[msgcol]);
	  itoa(newbmw, tempstr, 10);
	  showstr(tempstr, NEAREST[msgtokencol]);
	  showstr("x", NEAREST[msgcol]);
	  itoa(newbmh, tempstr, 10);
	  showstr(tempstr, NEAREST[msgtokencol]);
	  showstr(")", NEAREST[msgcol]);
	  if (newbmw <= 0 || newbmw > 2024 || newbmh <= 0 || newbmh > 2024)
            valid_factor = false;
	  else
	    valid_factor = true;
	  if (valid_factor)
	    showstr("  =-)\t\t", NEAREST[msgtokencol]);
	  else
	    showstr("  =-(\t\t", NEAREST[msgtokencol]);
	  flipoffscreenmem();
	  oldi = i;
	  plusminuschanged = false;
	}
    }
  showmsg("");
  if (escapism)
    return;
  showmsgpart("scaling bitmap by ");
  showstr(localstr, NEAREST[msgtokencol]);
  showstr("% ...", NEAREST[msgcol]);
  flipoffscreenmem();
  BMWIDTH = newbmw;
  BMHEIGHT = newbmh;
  scaleimage(&bitmap, BMWIDTH, BMHEIGHT);
  initraster(0, 0, VISUALBMW, VISUALBMH);
  redraw(); // flipping it twice!
  showmsgpart("scaling bitmap by ");
  floatstr(factor, localstr, 1);
  showstr(localstr, NEAREST[msgtokencol]);
  showstr("% ...", NEAREST[msgcol]);
  showstr("done.", NEAREST[msgcol]);
  flipoffscreenmem();
};

void change_size()
{
  showmsg("changing size, enter new width: ");
  flipoffscreenmem();
  int i = 0, ii;
  int res = 0xFFFFFFFF;
  bool escapism = false;
  while ((!getkey(KEY_ENTER) || res > xres) && !escapism)
    {
      ii = i;
      if (getkey(KEY_1)) localstr[i++] = '1';
      if (getkey(KEY_2)) localstr[i++] = '2';
      if (getkey(KEY_3)) localstr[i++] = '3';
      if (getkey(KEY_4)) localstr[i++] = '4';
      if (getkey(KEY_5)) localstr[i++] = '5';
      if (getkey(KEY_6)) localstr[i++] = '6';
      if (getkey(KEY_7)) localstr[i++] = '7';
      if (getkey(KEY_8)) localstr[i++] = '8';
      if (getkey(KEY_9)) localstr[i++] = '9';
      if (getkey(KEY_0)) localstr[i++] = '0';
      if (getkey(KEY_BACK))
	if (i > 0) localstr[--i] = '\0';
      if (ii != i)
	{
	  localstr[i] = '\0';
	  showmsgpart("changing size, enter new width: ");
	  showstr(localstr, NEAREST[msgtokencol]);
	  showstr("\t", 0);
	  res = (int)strtol(localstr, 0L, 10);
	  flipoffscreenmem();
	}
      if (getkey(KEY_ESC))
	escapism = true;
    }
  if (escapism)
    {
      showmsg("");
      return;
    }
  newbmw = (int)strtol(localstr, 0L, 10);
  localstr[0] = '\0';
  i = 0;
  clearkey(KEY_ENTER);
  showmsg("changing size, enter new height: ");
  flipoffscreenmem();
  res = 0xFFFFFFFF;
  while ((!getkey(KEY_ENTER) || res > yres) && !escapism)
    {
      ii = i;
      if (getkey(KEY_1)) localstr[i++] = '1';
      if (getkey(KEY_2)) localstr[i++] = '2';
      if (getkey(KEY_3)) localstr[i++] = '3';
      if (getkey(KEY_4)) localstr[i++] = '4';
      if (getkey(KEY_5)) localstr[i++] = '5';
      if (getkey(KEY_6)) localstr[i++] = '6';
      if (getkey(KEY_7)) localstr[i++] = '7';
      if (getkey(KEY_8)) localstr[i++] = '8';
      if (getkey(KEY_9)) localstr[i++] = '9';
      if (getkey(KEY_0)) localstr[i++] = '0';
      if (getkey(KEY_BACK))
	if (i > 0) localstr[--i] = '\0';
      if (ii != i)
	{
	  localstr[i] = '\0';
	  showmsgpart("changing size, enter new height: ");
	  showstr(localstr, NEAREST[msgtokencol]);
	  showstr("\t", 0);
	  res = (int)strtol(localstr, 0L, 10);
	  flipoffscreenmem();
	}
      if (getkey(KEY_ESC))
	escapism = true;
    }
  if (escapism)
    {
      showmsg("");
      return;
    }
  newbmh = (int)strtol(localstr, 0, 10);
  BMWIDTH = newbmw;
  BMHEIGHT = newbmh;
  scaleimage(&bitmap, BMWIDTH, BMHEIGHT);
  initraster(0, 0, VISUALBMW, VISUALBMH);
  redraw();
};

inline void drawcaret(unsigned char color)
{
  rectangle(MINX + (rasterx * cellx), MINY + (rastery * celly), cellx, celly, color);
};

void countcolors()
{
  char* list = (char*)xmalloc(0x100);
  showmsg("");
  showmsgpart("Current bitmap uses ");
  int numColors = createcolorlist(bitmap, list);
  itoa(numColors, localstr, 10);
  showstr(localstr, NEAREST[msgtokencol]);
  showstr(" colors. Press Enter.", NEAREST[msgcol]);
  int i;
  for (i = 0; i < numColors; i++)
    hline(xres - 1 - PALWIDTH, xres - 1, i, list[i]);
  for (; i < 0x100; i++)
    hline(xres - 1 - PALWIDTH, xres - 1, i, 0);
  xfree(list);
  flipoffscreenmem();
  while (!getkey(KEY_ENTER) && !getkey(KEY_ESC)) ;
  showpal();
  showmsg("");
};

void invert_colors()
{
  showmsg("");
  showmsgpart("Inverting colors (warning: not undo-able)...");
  flipoffscreenmem();
  unsigned char invert_table[0x100];
  int i;
  for (i = 0; i < 0x100; i++)
    invert_table[i] = findcontrastingindex(pal[currentpal], i);
  for (i = 4; i < (BMWIDTH * BMHEIGHT) + 4; i++)
    bitmap[i] = invert_table[bitmap[i]];
  showstr("done.", NEAREST[msgcol]);
  showmainframe();
  fillviewport();
};

void load_sprite()
{
  PAL_MODE palmode;
  if (!readfilename("to load sprite from", "spr", true))
    return;
  if (!fexists(szfilename))
    {
      showmsg("");
      showmsgpart("Can't find ");
      showstr(szfilename, NEAREST[msgtokencol]);
      showstr(".", NEAREST[msgcol]);
      return ;
    }
  palmode = ask_palmode();
  showmsg("");
  showmsgpart("loading from ");
  showstr(szfilename, NEAREST[msgtokencol]);
  showstr("...", NEAREST[msgcol]);
  flipoffscreenmem();
  int i;
  if (palmode == USE_PAL)
    {
      if (pal[custompal] != NULL)
	xfree(pal[custompal]);
      pal[custompal] = loadspr_256(szfilename, &newimage, USE_PAL);
    }
  else
    loadspr_256(szfilename, &newimage, TRANSLATE_PAL, pal[currentpal]);
  if (newimage != NULL)
    {
      newbmw = BITMAP_WIDTH(newimage);
      newbmh = BITMAP_HEIGHT(newimage);
      xfree(bitmap);
      if (BMWIDTH == newbmw && BMHEIGHT == newbmh)
	bitmap = newimage;
      else // bitmap has different size
	{
	  bitmap = newimage;
	  if (!ask_scale_it())
	    {
	      BMWIDTH = newbmw;
	      BMHEIGHT = newbmh;
	    }
	  else // scale it..
	    scaleimage(&bitmap, BMWIDTH, BMHEIGHT);
	}
      initraster(0, 0, VISUALBMW, VISUALBMH);
      redraw();
      showstr("done.", NEAREST[msgcol]);
      if (palmode == USE_PAL)
	{
	  for (i = 0; i < 13; i++)
	    szcustompal[i] = szfilename[i];
	  currentpal = custompal;
	  setpal(pal[currentpal]);
	}
    }
  else  // loadspr_256 returned NULL!
    {
      showmsg("");
      showmsgpart("Couldn't load from ");
      showstr(szfilename, NEAREST[msgtokencol]);
      showstr(".", NEAREST[msgcol]);
    }
};

void save_sprite()
{
  if (!readfilename("to save to as a sprite", "spr", false))
    return;
  showmsg("");
  showmsgpart("saving to ");
  showstr(szfilename, NEAREST[msgtokencol]);
  showstr("...", NEAREST[msgcol]);
  flipoffscreenmem();
  savespr_256(szfilename, bitmap, pal[currentpal]);
  showstr("done.", NEAREST[msgcol]);
};

void save_DIB()
{
  if (!readfilename("to save to as a DIB", "bmp", false))
    return;
  showmsg("");
  showmsgpart("saving to ");
  showstr(szfilename, NEAREST[msgtokencol]);
  showstr("...", NEAREST[msgcol]);
  flipoffscreenmem();
  saveDIB_256(szfilename, bitmap, pal[currentpal]);
  showstr("done.", NEAREST[msgcol]);
};

void load_DIB()
{
  if (!readfilename("to load DIB from", "bmp", true))
    return;
  check_DIB(szfilename);
  int i;
  if (GF_DIB_type == DIB_FILE_DOES_NOT_EXIST)
    {
      showmsg("");
      showmsgpart("Can't find ");
      showstr(szfilename, NEAREST[msgtokencol]);
      showstr(".", NEAREST[msgcol]);
      return ;
    }
  if (GF_DIB_type == DIB_FILE_NO_DIB || GF_DIB_type == DIB_FILE_UNKNOWN)
    {
      showmsg("");
      showstr(szfilename, NEAREST[msgtokencol]);
      showstr(" is not a Windows DIB. (or an unknown type)", NEAREST[msgcol]);
      return ;
    }
  if (DIB_FILE_CANT_HANDLE)
    {
      showmsg("Cannot handle ");
      showstr(szfilename, NEAREST[msgtokencol]);
      showstr(" - should be either 1/4/16/32bit or compressed.", NEAREST[msgcol]);
      return ;
    }
  PAL_MODE palmode;
  if (GF_DIB_type == DIB_FILE_8)
    palmode = ask_palmode();
  else // 24-bit bitmap
    palmode = TRANSLATE_PAL;
  showmsg("");
  showmsgpart("loading from ");
  showstr(szfilename, NEAREST[msgtokencol]);
  showstr("...", NEAREST[msgcol]);
  flipoffscreenmem();
  if (palmode == USE_PAL)
    {
      if (pal[custompal] != NULL)
	xfree(pal[custompal]);
      pal[custompal] = loadDIB_256(szfilename, &newimage, USE_PAL);
    }
  else
    loadDIB_256(szfilename, &newimage, TRANSLATE_PAL, pal[currentpal]);
  if (newimage != NULL)
    {
      newbmw = BITMAP_WIDTH(newimage);
      newbmh = BITMAP_HEIGHT(newimage);
      xfree(bitmap);
      if (newbmw == BMWIDTH && newbmh == BMHEIGHT)
	bitmap = newimage;
      else
	{
	  bitmap = newimage;
	  if (!ask_scale_it())
	    {
	      BMWIDTH = newbmw;
	      BMHEIGHT = newbmh;
	    }
	  else // scale it..
	    scaleimage(&bitmap, BMWIDTH, BMHEIGHT);
	}
      initraster(0, 0, VISUALBMW, VISUALBMH);
      redraw();
      showstr("done.", NEAREST[msgcol]);
      if (palmode == USE_PAL)
	{
	  for (i = 0; i < 13; i++)
	    szcustompal[i] = szfilename[i];
	  currentpal = custompal;
	  setpal(pal[currentpal]);
	}
    }
  else // loadDIB_256 returned NULL!
    {
      showmsg("");
      showmsgpart("Couldn't load from ");
      showstr(szfilename, NEAREST[msgtokencol]);
      showstr(".", NEAREST[msgcol]);
    }
};

int main() //					  MAIN
{
  BMWIDTH = 200;
  BMHEIGHT = 200;
  bitmap = Imageblock(BMWIDTH, BMHEIGHT);
  int i;
  memset(bitmap + 4, 0x00, BMWIDTH * BMHEIGHT);
  bitmap[0] = LOW_BYTE(BMWIDTH);
  bitmap[1] = HIGH_BYTE(BMWIDTH);
  bitmap[2] = LOW_BYTE(BMHEIGHT);
  bitmap[3] = HIGH_BYTE(BMHEIGHT);
  szfilename[0] = '\0';

  loadpalette("univ.pal", &pal[0]);
  loadpalette("shadelin.pal", &pal[1]);
  loadpalette("shadecos.pal", &pal[2]);
  pal[custompal] = (char*)xmalloc(0x300);
  strcpy(szcustompal, "StdVGA");
  if (!openVBE(1024, 768))
    return 1;
  MINX = 1;
  MINY = TOPINFOSPACE;
  MAXX = xres - PALWIDTH - FRAMEPALSPACING - SCROLLBARSIZE;
  MAXY = yres - INSTRSPACING - INSTRTEXTSPACE - SCROLLBARSIZE;
  MAX_VISUALX = (int)((MAXX - MINX) / (float)MIN_CELLX);
  MAX_VISUALY = (int)((MAXY - MINY) / (float)MIN_CELLY);
  getpal(pal[custompal]);
  installkeyhandler();
  //installcountingtimerhandler();
  //Timercount = 0;
  settextsize(8, 8);
  GRTEXT_BGColor = 0;
  initraster(0, 0, 200, 200);
  drawcol = findnearestcolor(pal[currentpal], 60, 60, 60);
  redraw();
  while (!peekkey(KEY_ESC))
    {
      if (peekkey(KEY_P))
	{
	  if (getkey(KEY_RSHIFT))
	    {
	      if (currentpal < custompal) currentpal++;
	      else currentpal = 0;
	      if (currentpal == custompal && pal[custompal] == 0)
		currentpal = 0;
	      redraw();
	      showmsg("switched palette.");
	    }
	  if (getkey(KEY_L))
	    {
	      readfilename("to load custom-palette from", "pal", true);
	      showmsg("");
	      showmsgpart("loading from ");
	      showstr(szfilename, NEAREST[msgtokencol]);
	      showstr("...", NEAREST[msgcol]);
	      for (i = 0; i < 13; i++)
		szcustompal[i] = szfilename[i];
	      if (pal[custompal] != NULL)
		xfree(pal[custompal]);
	      loadpalette(szcustompal, pal + custompal);
	      showstr("done.", NEAREST[msgcol]);
	      currentpal = custompal;
	      redraw();
	    }
	  if (getkey(KEY_S))
	    {
	      readfilename("to save current palette to", "pal", false);
	      showmsg("");
	      showmsgpart("saving to ");
	      showstr(szfilename, NEAREST[msgtokencol]);
	      showstr("...", NEAREST[msgcol]);
	      for (i = 0; i < 13; i++)
		szcustompal[i] = szfilename[i];
	      savepalette(szcustompal, pal[currentpal]);
	      showstr("done.", NEAREST[msgcol]);
	      showmainframe();
	    }
	}
      if (getkey(KEY_F1))
	load_sprite();
      if (getkey(KEY_F2))
	save_sprite();
      if (getkey(KEY_F3))
	load_DIB();
      if (getkey(KEY_F4))
	save_DIB();
      if (getkey(KEY_R))
	if (rastercol) drawraster(rastercol = 0);
	else drawraster(rastercol = drawcol);
      if (getkey(KEY_MINUS))
	{
	  showmsg("color--");
	  if (drawcol > 0) drawcol--;
	  showpal();
	}
      if (getkey(KEY_PLUS))
	{
	  showmsg("color++");
	  if (drawcol < 255) drawcol++;
	  showpal();
	}
      if (getkey(KEY_PAD7))
	{
	  if (caretvisible) drawcaret(rastercol);
	  if (scrollx >= VISUALBMW)
	    scrollx -= VISUALBMW;
	  else
	    scrollx = 0;
	  showmainframe();
	  fillviewport();
	  drawcaret(drawcol);
	  caretvisible = 1;
	}
      if (getkey(KEY_PAD1))
	{
	  if (caretvisible) drawcaret(rastercol);
	  if (scrollx <= BMWIDTH - (2 * VISUALBMW))
	    scrollx += VISUALBMW;
	  else
	    scrollx = BMWIDTH - VISUALBMW;
	  showmainframe();
	  fillviewport();
	  drawcaret(drawcol);
	  caretvisible = 1;
	}
      if (getkey(KEY_PAD5))
	{
	  if (caretvisible) drawcaret(rastercol);
	  rasterx = (VISUALBMW / 2) - 1;
	  drawcaret(drawcol);
	  caretvisible = 1;
	}
      if (peekkey(KEY_PAD3))
	{
	  scrolly += VISUALBMH;
	  if (scrolly > BMHEIGHT - VISUALBMH) scrolly = BMHEIGHT - VISUALBMH;
	  showmainframe();
	  fillviewport();
	  drawcaret(drawcol);
	  caretvisible = 1;
	}
      if (peekkey(KEY_PAD9))
	{
	  if (caretvisible) drawcaret(rastercol);
	  scrolly -= VISUALBMH;
	  if (scrolly < 0) scrolly = 0;
	  showmainframe();
	  fillviewport();
	  drawcaret(drawcol);
	  caretvisible = 1;
	}
      if (getkey(KEY_BACK))
	{
	  memset(bitmap + 4, 0x00, BMWIDTH * BMHEIGHT);
	  fillviewport();
	  showmsg("bitmap cleared.");
	}
      if (getkey(KEY_SPACE))
	{
	  i = 4 + ((rastery + scrolly) * BMWIDTH) + rasterx + scrollx;
	  if (bitmap[i] != 0x00)
	    {
	      drawcell(rasterx, rastery, 0);
	      bitmap[i] = 0x00;
	    }
	  else
	    {
	      drawcell(rasterx, rastery, drawcol);
	      bitmap[i] = drawcol;
	    }
	}
      if (peekkey(KEY_CTRL))
	{
	  drawcell(rasterx, rastery, drawcol);
	  bitmap[4 + ((rastery + scrolly) * BMWIDTH) + rasterx + scrollx] = drawcol;
	}
      if (peekkey(KEY_ALT))
	{
	  drawcell(rasterx, rastery, 0);
	  bitmap[4 + ((rastery + scrolly) * BMWIDTH) + rasterx + scrollx] = 0x00;
	}
      if (peekkey(KEY_PAD4) /*&& !(Timercount % 2)*/)
	{
	  if (caretvisible) drawcaret(rastercol);
	  if (rasterx > 0)
	    rasterx--;
	  else if (scrollx > 0)
	    {
	      scrollx--;
	      showmainframe();
	      fillviewport();
	    }
	  drawcaret(drawcol);
	  caretvisible = 1;
	  if (peekkey(KEY_RSHIFT))
	    clearkey(KEY_PAD4);
	}
      if (peekkey(KEY_PAD6) /*&& !(Timercount % 2)*/)
	{
	  if (caretvisible) drawcaret(rastercol);
	  if (rasterx < VISUALBMW - 1)
	    rasterx++;
	  else if (scrollx < BMWIDTH - VISUALBMW)
	    {
	      scrollx++;
	      showmainframe();
	      fillviewport();
	    }
	  drawcaret(drawcol);
	  caretvisible = 1;
	  if (peekkey(KEY_RSHIFT))
	    clearkey(KEY_PAD6);
	}
      if (peekkey(KEY_PAD8) /*&& !(Timercount % 2)*/)
	{
	  if (caretvisible) drawcaret(rastercol);
	  if (rastery > 0)
	    rastery--;
	  else if (scrolly > 0)
	    {
	      scrolly--;
	      showmainframe();
	      fillviewport();
	    }
	  drawcaret(drawcol);
	  caretvisible = 1;
	  if (peekkey(KEY_RSHIFT))
	    clearkey(KEY_PAD8);
	}
      if (peekkey(KEY_PAD2) /*&& !(Timercount % 2)*/)
	{
	  if (caretvisible) drawcaret(rastercol);
	  if (rastery < VISUALBMH - 1)
	    rastery++;
	  else if (scrolly < BMHEIGHT - VISUALBMH)
	    {
	      scrolly++;
	      showmainframe();
	      fillviewport();
	    }
	  drawcaret(drawcol);
	  caretvisible = 1;
	  if (peekkey(KEY_RSHIFT))
	    clearkey(KEY_PAD2);
	}
      if (getkey(KEY_W))
	change_size();
      if (getkey(KEY_S))
	scale_bitmap();
      if (getkey(KEY_F))
	selectnc();
      if (getkey(KEY_C))
	countcolors();
      if (getkey(KEY_I))
	invert_colors();
      if (getkey(KEY_V))
	{
	  if (BMWIDTH < xres - 3 && BMHEIGHT < yres - 20)
	    view_image_wholescreen();
	  else
	    showmsg("Cannot show bitmap whole-screen (too large).");
	}
      if (getkey(KEY_Q))
	saveScreen("screen.spr");
      drawinfo();
      flipoffscreenmem();
    }
  restorekeyhandler();
  //restoretimerhandler();
  xfree(bitmap);
  xfree(pal[0]);
  xfree(pal[1]);
  xfree(pal[2]);
  if (pal[custompal] != NULL)
    xfree(pal[custompal]);
  fadepalout(fadespeed);
  closeVBE();
  return 0;
}

/*	if (Timercount > 8)
	{
	if (caretvisible ^= 1)
	drawcaret(drawcol);
	else
	drawcaret(rastercol);
	Timercount = 0;
	}*/

