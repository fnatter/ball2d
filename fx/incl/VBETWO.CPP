//#define TEST_VBE

#include "vbetwo.h"
#include "quitbug.h"
#include "palettes.h" // for cc-constants, NEAREST & SHADES_PER_COLOR
#include "grtext.h" // for getBDA.. and setttextsize
#include "gformats.h" // for BITMAP_WIDTH & HEIGHT etc.
#include <dpmi.h>
#include <go32.h>
#include "lcstdlib.h" // xfree/xmalloc
#include <stdio.h>
#include <math.h>
#include <string.h> // strncpy, memset

/*
  for line clipping (see void clipline(...) for details)
 */
struct outcode
{
  unsigned all;
  unsigned left:4;
  unsigned right:4;
  unsigned bottom:4;
  unsigned top:4;
};


static bool initVBE();
static bool getVBEModeInfo(unsigned	short	mode);
static void safe_int(int vect, __dpmi_regs* pregs);

static int CLIP_LEFT, CLIP_RIGHT, CLIP_TOP, CLIP_BOTTOM;
/* set these using setClipRect */

// these are used by assembly routines...
extern unsigned long BytesPerScanline;
extern char*	offscreen;
extern int screensize_dword;
extern short	LFBselector;
extern Scanline* scanline_list;
extern unsigned char* shadinglookie[0x100];

extern VBEINFO vbe_info;
extern VBE_MODEINFO vbe_modeinfo;
extern unsigned char* contrast_lookup;
static VBE_PM_Interface* vbe_PMI;
static unsigned short* AvailableModes;
static unsigned long LFB_linear_address;
static unsigned char BPP;
static int LogicalBytesPerScanline, LogicalPixelsPerScanline,
  DisplayStartX, DisplayStartY;
static void* PMI_setDisplayStart;
static void* PMI_setPrimaryPalette;


void setClipRect(int left, int top, int right, int bottom)
{
  CLIP_LEFT = left;
  CLIP_RIGHT = right;
  CLIP_TOP = top;
  CLIP_BOTTOM = bottom;
};

static void safe_int(int vect, __dpmi_regs* pregs)
{
  if (__dpmi_int(vect, pregs) != 0)
    handleError(ErrorMessage("Error issuing RM-int (dpmi failed)", "", vect));
};

void saveScreen(const char* szfilename, char* pal)
{
  FILE* scr = fopen(szfilename, "wb");
  short x_res = (short)xres, y_res = (short)yres;
  char curpal[0x300];
  if (pal == NULL)
    {
      getpal(curpal);
      fwrite(curpal, 0x300, 1, scr);
    }
  else
    fwrite(pal, 0x300, 1, scr);
  fwrite(&x_res, sizeof(short), 1, scr);
  fwrite(&y_res, sizeof(short), 1, scr);
  fwrite(offscreen, screensize_dword, 4, scr);
  fclose(scr);
};

static bool initVBE()
{
  __dpmi_regs	regs;
  strncpy(vbe_info.VESASignature, "VESA", 4);
  dosmemput(&vbe_info, sizeof(VBEINFO), __tb);
  regs.x.ax = 0x4F00;
  regs.x.es = __tb / 16;
  regs.x.di = 0x0000;
  safe_int(0x10, &regs);
  dosmemget(__tb, sizeof(VBEINFO), &vbe_info);
  if (regs.x.ax != 0x004F	|| vbe_info.VBEVersion < 0x0200)
    return false;
  int modelist_physical =	(vbe_info.VideoModePtrSegment	* 16)	+ vbe_info.VideoModePtrOffset;
  int lenbytes = 0;
  unsigned short word;
  do
    {
      dosmemget(modelist_physical +	lenbytes, 2, &word);
      lenbytes +=	2;
    } while (word != 0xFFFF);
  AvailableModes = (unsigned short*)xmalloc(lenbytes);
  dosmemget(modelist_physical, lenbytes, AvailableModes);
  regs.x.ax =	0x4F0A;
  regs.h.bl =	0x00;
  safe_int(0x10, &regs);
  VBE_PM_Interface*	vbe_PMI = (VBE_PM_Interface*)xmalloc(regs.x.cx);
  dosmemget((regs.x.es * 16) + regs.x.di, regs.x.cx, vbe_PMI);
  PMI_setDisplayStart = (void*)((char*)vbe_PMI + vbe_PMI->setDisplayStart);
  PMI_setPrimaryPalette =	(void*)((char*)vbe_PMI + vbe_PMI->setPrimaryPalette);
  return true;
};

static bool	getVBEModeInfo(unsigned	short	mode)
{
  __dpmi_regs	regs;
  regs.x.ax =	0x4F01;
  regs.x.cx =	mode;
  regs.x.es =	__tb / 16;
  regs.x.di =	0x0000;
  safe_int(0x10, &regs);
  dosmemget(__tb, sizeof(VBE_MODEINFO), &vbe_modeinfo);
  if (regs.x.ax != 0x004F	|| !(vbe_modeinfo.ModeAttributes & vbeModeAvailablewithLFB))
    return false;
  else
    return true;
};

bool openVBE(int width,	int height,	unsigned char bpp	= 8)
{
  if (!initVBE())
    {
      printf("\nNo VBE 2.0 loaded.\n");
      abort();
    }
  int modeidx	= 0;
  unsigned short mode;
  while	(1)
    {
      mode = AvailableModes[modeidx++];
      if (mode ==	0xFFFF)
	return false;
      if (getVBEModeInfo(mode) == false)
	continue;
      if (vbe_modeinfo.XResolution != width)
	continue;
      if (vbe_modeinfo.YResolution != height)
	continue;
      if (vbe_modeinfo.BitsPerPixel	!= bpp)
	continue;
      break;
    }
  getBDA8x8charpattern();
  settextsize(8, 8);
  xres = vbe_modeinfo.XResolution;
  yres = vbe_modeinfo.YResolution;
  BPP =	vbe_modeinfo.BitsPerPixel;
  CLIP_LEFT = 0;
  CLIP_TOP = 0;
  CLIP_RIGHT = xres - 1;
  CLIP_BOTTOM = yres - 1;
  BytesPerScanline = vbe_modeinfo.BytesPerScanline;
  unsigned long screensize = BytesPerScanline * yres;
  if (screensize % 4)
    handleError(ErrorMessage("Screensize [PARAM1] isn't dword-aligned.", "", screensize));
  offscreen =	(char*)xmalloc(screensize);
  screensize_dword = screensize	/ 4;
  if (sizeof(Scanline) != 32)
    handleError(ErrorMessage("Compiler DOES pad 'Scanline's at the end!!!", "", sizeof(Scanline)));
  scanline_list = (Scanline*)xmalloc(yres * sizeof(Scanline));
  contrast_lookup = (unsigned char*)xmalloc(0x100);
  __dpmi_meminfo meminf;
  meminf.address = vbe_modeinfo.PhysicalBasePtr;
  meminf.size	= screensize;
  if (__dpmi_physical_address_mapping(&meminf) ==	- 1)
    handleError(ErrorMessage("physical-address mapping failed",	"", meminf.address));
  LFB_linear_address = meminf.address;
  LFBselector	= __dpmi_allocate_ldt_descriptors(1);
  if (LFBselector == -1)
    handleError(ErrorMessage("allocating ldt-descriptors failed"));
  if (__dpmi_set_segment_base_address(LFBselector, LFB_linear_address) ==	-1)
    handleError(ErrorMessage("setting selector base address to [PARAM1] failed", "", LFB_linear_address));
  if (__dpmi_set_segment_limit(LFBselector,	meminf.size) == -1)
    handleError(ErrorMessage("setting selector limit to [PARAM1] failed", "", meminf.size));
  __dpmi_regs	regs;
  regs.x.ax =	0x4F02;
  regs.x.bx =	mode | VBE_MODEFLAG_LFB;
  safe_int(0x10, &regs);
  if (regs.x.ax != 0x004F)
    return false;
  //if ((LogicalPixelsPerScanline = getLogicalScanlineWidthPixels()) == 0)
  //      handleError(ErrorMessage("Couldn't get logical scanline-width."));
  LogicalBytesPerScanline	= BytesPerScanline;
  //if (setLogicalScanlineWidthPixels(LogicalPixelsPerScanline = xres) != 1)
  //      handleError(ErrorMessage("Couldn't set logical scanline-width.", "", xres));
  //if (getDisplayStart(&DisplayStartX, &DisplayStartY) != 1)
  //      handleError(ErrorMessage("Couldn't get display-start."));
  DisplayStartX = 0;
  DisplayStartY = 0;
  //if (setDisplayStart(DisplayStartX, DisplayStartY) != 1)
  //      handleError(ErrorMessage("Couldn't set display-start.", "", DisplayStartX, DisplayStartY));
  /*
    the following is taken from someone's vesa-driver for s3-cards;
    I didn't notice any difference back when I had an s3-card
   */
  //come_on_push_it(); // S3 (and only S3!) S-P-E-E-D-U-P
  updatecolors();
  return true;
};

void closeVBE()
{
  __dpmi_regs	regs;
  regs.x.ax =	0x4F02;
  regs.x.bx =	0x0003;
  safe_int(0x10, &regs);
  if (contrast_lookup != NULL)
    xfree(contrast_lookup);
  xfree(AvailableModes);
  xfree(offscreen);
  xfree(vbe_PMI);
  xfree(scanline_list);
  if (__dpmi_free_ldt_descriptor(LFBselector) == -1)
    handleError(ErrorMessage("failure when freeing LFB-selector ([PARAM1])", "", LFBselector));
  __dpmi_meminfo meminf;
  meminf.address = LFB_linear_address;
  /*  if (__dpmi_free_physical_address_mapping(&meminf) == -1)
    handleError(ErrorMessage("failure when freeing physical
address mapping (LFB) [PARAM1] = linear address", "", LFB_linear_address));*/
};

void init_shadinglookups(char* shadepal)
{
  int i;
  for (i = 0; i < 0x100; i++)
    shadinglookie[i] = createshadinglookup(shadepal, SHADES_PER_COLOR, i);
};

void try_free_shadinglookups()
{
  int i;
  for (i = 0; i < 0x100; i++)
    if (shadinglookie[i] != NULL)
      xfree(shadinglookie[i]);
};

void clip_fillcircle(int cx,	int cy, int	cr, unsigned char	col)
{
  int	dec =	3 - (cr << 2),
    ix  =	0,
    iy  =	cr;
  while	(ix <= iy)
    {
      clip_hline(cx - ix, cx	+ ix,	cy + iy, col);
      clip_hline(cx - ix, cx	+ ix,	cy - iy, col);
      clip_hline(cx - iy, cx	+ iy,	cy + ix, col);
      clip_hline(cx - iy, cx	+ iy,	cy - ix, col);
      if (dec < 0)
	dec += (ix << 2) + 6;
      else
	{
	  dec += ((ix-iy) << 2) +	10;
	  iy--;
	}
      ix++;
    }
};

void fillcircle(int cx,	int cy, int	cr, unsigned char	col)
{
  int	dec =	3 - (cr << 2),
    ix  =	0,
    iy  =	cr;
  while	(ix <= iy)
    {
      hline(cx - ix, cx	+ ix,	cy + iy, col);
      hline(cx - ix, cx	+ ix,	cy - iy, col);
      hline(cx - iy, cx	+ iy,	cy + ix, col);
      hline(cx - iy, cx	+ iy,	cy - ix, col);
      if (dec < 0)
	dec += (ix << 2) + 6;
      else
	{
	  dec += ((ix-iy) << 2) +	10;
	  iy--;
	}
      ix++;
    }
};

void circle(int cx, int	cy, int cr,	unsigned char col)
{
  int	dec =	3 - (cr << 2),
    ix  =	0,
    iy  =	cr;
  while	(ix <= iy)
    {
      putpixel(cx	+ ix,	cy + iy, col);
      putpixel(cx	+ ix,	cy - iy, col);
      putpixel(cx	- ix,	cy + iy, col);
      putpixel(cx	- ix,	cy - iy, col);
      putpixel(cx	+ iy,	cy + ix, col);
      putpixel(cx	+ iy,	cy - ix, col);
      putpixel(cx	- iy,	cy + ix, col);
      putpixel(cx	- iy,	cy - ix, col);
      if (dec < 0)
	dec += (ix << 2) + 6;
      else
	{
	  dec += ((ix-iy) << 2) +	10;
	  iy--;
	}
      ix++;
    }
};

void clipcircle(int cx, int	cy, int cr,	unsigned char col)
{
  int	dec =	3 - (cr << 2),
    ix  =	0,
    iy  =	cr;
  while	(ix <= iy)
    {
      if ((unsigned)(cx + ix) < (unsigned)xres && (unsigned)(cy + iy) < (unsigned)yres)
      	putpixel(cx	+ ix,	cy + iy, col);
      if ((unsigned)(cx + ix) < (unsigned)xres && (unsigned)(cy - iy) < (unsigned)yres)
      	putpixel(cx	+ ix,	cy - iy, col);
      if ((unsigned)(cx - ix) < (unsigned)xres && (unsigned)(cy + iy) < (unsigned)yres)
      	putpixel(cx	- ix,	cy + iy, col);
      if ((unsigned)(cx - ix) < (unsigned)xres && (unsigned)(cy - iy) < (unsigned)yres)
      	putpixel(cx	- ix,	cy - iy, col);
      if ((unsigned)(cx + iy) < (unsigned)xres && (unsigned)(cy + ix) < (unsigned)yres)
      	putpixel(cx	+ iy,	cy + ix, col);
      if ((unsigned)(cx + iy) < (unsigned)xres && (unsigned)(cy + ix) < (unsigned)yres)
      	putpixel(cx	+ iy,	cy - ix, col);
      if ((unsigned)(cx - iy) < (unsigned)xres && (unsigned)(cy + ix) < (unsigned)yres)
      	putpixel(cx	- iy,	cy + ix, col);
      if ((unsigned)(cx - iy) < (unsigned)xres && (unsigned)(cy - ix) < (unsigned)yres)
      	putpixel(cx	- iy,	cy - ix, col);
      if (dec < 0)
	dec += (ix << 2) + 6;
      else
	{
	  dec += ((ix-iy) << 2) +	10;
	  iy--;
	}
      ix++;
    }
};

void thickcircle(int cx, int cy, int cr, unsigned char col)
{
  int	dec =	3 - (cr << 2),
    ix  =	0,
    iy  =	cr;
  while	(ix <= iy)
    {
      putpixel(cx	+ ix,	cy + iy - 1, col);
      putpixel(cx	+ ix,	cy + iy, col);
      putpixel(cx	+ ix,	cy + iy + 1, col);
      putpixel(cx	+ ix,	cy - iy - 1, col);
      putpixel(cx	+ ix,	cy - iy, col);
      putpixel(cx	+ ix,	cy - iy + 1, col);
      putpixel(cx	- ix,	cy + iy - 1, col);
      putpixel(cx	- ix,	cy + iy, col);
      putpixel(cx	- ix,	cy + iy + 1, col);
      putpixel(cx	- ix,	cy - iy - 1, col);
      putpixel(cx	- ix,	cy - iy, col);
      putpixel(cx	- ix,	cy - iy + 1, col);
      putpixel(cx	+ iy - 1, cy + ix, col);
      putpixel(cx	+ iy,	cy + ix, col);
      putpixel(cx	+ iy + 1, cy + ix, col);
      putpixel(cx	+ iy - 1, cy - ix, col);
      putpixel(cx	+ iy,	cy - ix, col);
      putpixel(cx	+ iy + 1, cy - ix, col);
      putpixel(cx	- iy - 1, cy + ix, col);
      putpixel(cx	- iy,	cy + ix, col);
      putpixel(cx	- iy + 1, cy + ix, col);
      putpixel(cx	- iy - 1, cy - ix, col);
      putpixel(cx	- iy,	cy - ix, col);
      putpixel(cx	- iy + 1, cy - ix, col);
      if (dec < 0)
	dec += (ix << 2) + 6;
      else
	{
	  dec += ((ix-iy) << 2) +	10;
	  iy--;
	}
      ix++;
    }
};

/*
  these (as well as the functions to draw a circle) use bresenham's
  algorithms, which makes it possible to draw lines using only integer
  operations
  it has something to do with finding derivatives, but I did not
  understand it all the way, I just used the algorithm as is
  */

void line(int x1, int y1, int x2, int y2, unsigned char color)
{
  int	 deltax = abs(x2-x1), deltay = abs(y2-y1),
    xincr = (x1 > x2) ? (-1) : (1), yincr = (y1 > y2) ? (-BytesPerScanline) : (BytesPerScanline),
    decinc_right, decinc_rightup, dec, offset = (y1 * BytesPerScanline) + x1;
  if (deltax >= deltay) // cx is independent
    {
      decinc_right = deltay << 1;
      decinc_rightup = decinc_right	- (deltax << 1);
      dec =	decinc_rightup - deltax;
      for (; deltax >= 0; deltax--)
	{
	  offscreen[offset] = color;
	  if (dec > 0)	// pixel going right and up
	    {
	      offset += xincr + yincr;
	      dec += decinc_rightup;
	    }
	  else			// pixel going right
	    {
	      offset += xincr;
	      dec += decinc_right;
	    }
	}
    }
  else
    {		  // cy is independent
      decinc_right = deltax << 1;
      decinc_rightup = decinc_right	- (deltay << 1);
      dec =	decinc_right - deltay;
      for (; deltay >= 0; deltay--)
	{
	  offscreen[offset] = color;
	  if (dec > 0)
	    {
	      offset += xincr + yincr;
	      dec += decinc_rightup;
	    }
	  else
	    {
	      offset += yincr;
	      dec += decinc_right;
	    }
	}
    }
};

void thickline(int x1, int y1, int x2, int y2, unsigned char color)
{
  int	 deltax = abs(x2-x1), deltay = abs(y2-y1),
    xincr = (x1 > x2) ? (-1) : (1), yincr = (y1 > y2) ? (-1) :	(1),
    decinc_right, decinc_rightup, dec,	cx = x1, cy	= y1;
  if (deltax >= deltay) // cx is independent
    {
      decinc_right = deltay << 1;
      decinc_rightup = decinc_right	- (deltax << 1);
      dec =	decinc_rightup - deltax;
      for (; deltax >= 0; deltax--)
	{
	  putpixel(cx, cy -	1, color);
	  putpixel(cx, cy, color);
	  putpixel(cx, cy +	1, color);
	  if (dec > 0)	// pixel going right and up
	    {
	      cx +=	xincr;
	      cy +=	yincr;
	      dec += decinc_rightup;
	    }
	  else			// pixel going right
	    {
	      cx +=	xincr;
	      dec += decinc_right;
	    }
	}
    }
  else
    {		  // cy is independent
      decinc_right = deltax << 1;
      decinc_rightup = decinc_right	- (deltay << 1);
      dec =	decinc_right - deltay;
      for (; deltay >= 0; deltay--)
	{
	  putpixel(cx	- 1, cy, color);
	  putpixel(cx, cy, color);
	  putpixel(cx	+ 1, cy, color);
	  if (dec > 0)
	    {
	      cx +=	xincr;
	      cy +=	yincr;
	      dec += decinc_rightup;
	    }
	  else
	    {
	      cy +=	yincr;
	      dec += decinc_right;
	    }
	}
    }
};

/*
  the following function is taken from the book
  "Computer Graphics, Principles and Practice"
  (it's a very good book IMHO)
 */

void clipline(int x1, int y1, int x2, int y2, unsigned char color)
{
  bool accept, done;
  outcode outcode1, outcode2, outcodeOut;
  int x = 0, y = 0; // suppresses warning
  accept = false;
  done = false;
  memset(&outcode1, 0x00, sizeof(outcode1));
  if (y1 > CLIP_BOTTOM)
    {
      outcode1.top = 0x08;
      outcode1.all += 0x08;
    }
  else if (y1 < CLIP_TOP)
    {
      outcode1.bottom = 0x04;
      outcode1.all += 0x04;
    }
  if (x1 > CLIP_RIGHT)
    {
      outcode1.right = 0x02;
      outcode1.all += 0x02;
    }
  else if (x1 < CLIP_LEFT)
    {
      outcode1.left = 0x01;
      outcode1.all += 0x01;
    }
  memset(&outcode2, 0x00, sizeof(outcode2));
  if (y2 > CLIP_BOTTOM)
    {
      outcode2.top = 0x08;
      outcode2.all += 0x08;
    }
  else if (y2 < CLIP_TOP)
    {
      outcode2.bottom = 0x04;
      outcode2.all += 0x04;
    }
  if (x2 > CLIP_RIGHT)
    {
      outcode2.right = 0x02;
      outcode2.all += 0x02;
    }
  else if (x2 < CLIP_LEFT)
    {
      outcode2.left = 0x01;
      outcode2.all += 0x01;
    }
  do
    {
      if (outcode1.all == 0 && outcode2.all == 0)
	{
	  accept = true;
	  done = true;
	}
      else if ((outcode1.all & outcode2.all) != 0)
	{
	  done = true;
	}
      else
	{
	  if (outcode1.all != 0)
	    outcodeOut = outcode1;
	  else
	    outcodeOut = outcode2;
	  if (outcodeOut.top)
	    {
	      x = x1 + ((x2 - x1) * (CLIP_BOTTOM - y1)) / (y2 - y1);
	      y = CLIP_BOTTOM;
	    }
	  else if (outcodeOut.bottom)
	    {
	      x = x1 + ((x2 - x1) * (CLIP_TOP - y1)) / (y2 - y1);
	      y = CLIP_TOP;
	    }
	  else if (outcodeOut.right)
	    {
	      y = y1 + ((y2 - y1) * (CLIP_RIGHT - x1)) / (x2 - x1);
	      x = CLIP_RIGHT;
	    }
	  else if (outcodeOut.left)
	    {
	      y = y1 + ((y2 - y1) * (CLIP_LEFT - x1)) / (x2 - x1);
	      x = CLIP_LEFT;
	    }
	  if (outcodeOut.all == outcode1.all)
	    {
	      x1 = x;
	      y1 = y;
	      memset(&outcode1, 0x00, sizeof(outcode1));
	      if (y1 > CLIP_BOTTOM)
		{
		  outcode1.top = 0x08;
		  outcode1.all += 0x08;
		}
	      else if (y1 < CLIP_TOP)
		{
		  outcode1.bottom = 0x04;
		  outcode1.all += 0x04;
		}
	      if (x1 > CLIP_RIGHT)
		{
		  outcode1.right = 0x02;
		  outcode1.all += 0x02;
		}
	      else if (x1 < CLIP_LEFT)
		{
		  outcode1.left = 0x01;
		  outcode1.all += 0x01;
		}
	    }
	  else
	    {
	      x2 = x;
	      y2 = y;
	      memset(&outcode2, 0x00, sizeof(outcode2));
	      if (y2 > CLIP_BOTTOM)
		{
		  outcode2.top = 0x08;
		  outcode2.all += 0x08;
		}
	      else if (y2 < CLIP_TOP)
		{
		  outcode2.bottom = 0x04;
		  outcode2.all += 0x04;
		}
	      if (x2 > CLIP_RIGHT)
		{
		  outcode2.right = 0x02;
		  outcode2.all += 0x02;
		}
	      else if (x2 < CLIP_LEFT)
		{
		  outcode2.left = 0x01;
		  outcode2.all += 0x01;
		}
	    }
	}
    } while (!done);
  if (accept)
    line(x1, y1, x2, y2, color);
};


void rectangle(int left, int	top, int width, int height, unsigned char	color)
{
  hline(left,	left + width + 1, top, color);
  hline(left,	left + width + 1, top + height, color);
  vline(top, top + height + 1, left, color);
  vline(top, top + height + 1, left + width, color);
};

void fillrect(int	left,	int top, int width, int	height, unsigned char color)
{
  int y;
  for (y = top; y <= top + height; y++)
    hline(left,	left + width + 1, y, color); // +1 is because hline excludes endpoint
};

/*
the following function is taken from "abe's demoskool",
a tutorial on real-mode graphics programming (pretty old by now)
*/

void drawplasma(void)
{
  int x,y;
  unsigned char col;
  const	float	kx = M_PI*2	/ xres;
  const	float	ky = M_PI*2	/ yres;
  unsigned long offset = 0;
  for(y	= 0; y < yres; y++)
    {
      for(x	= 0; x < xres; x++)
	{
	  /* calculate a color with the following formula */
	  col =	(unsigned char)((sin(x*kx*0.5)*sin(y*ky*0.5))*(127-20)+128);
	  /* try changing the formula and see how the picture changes */
	  offscreen[offset++] = col;
	}
    }
  /*for (y = 0;	y < xres; y++)
    for (x = 0; x < xres; x++)
    putpixel(x, y, ((x * y) >> 2) % 256);*/
};

/*
  the idea of using scanline-lists (scan the whole polygon,
  and then render all lines at once) is taken
  from dr.dobbs journal
 */
void clrhll(int firstY, int lastY)
{
  int i;
  for (i = firstY; i <= lastY; i++)
    {
      scanline_list[i].left = 0x1000;
      scanline_list[i].right = 0x0000;
    }
};

void scanside(Vertex p1, Vertex p2) // use Bresenham's !
{
  int i;
  Vertex temp;
  if (p1.y == p2.y)
    {
      if (p1.x > p2.x)
	{
	  scanline_list[p2.y].left = p2.x;
	  scanline_list[p2.y].right = p1.x;
	}
      else
	{
	  scanline_list[p2.y].left = p1.x;
	  scanline_list[p2.y].right = p2.x;
	}
      return;
    }
  if (p1.y > p2.y)
    {
      temp = p2;
      p2 = p1;
      p1 = temp;
    }
  int dx_dy, x, intx;
  dx_dy = (((long long)(p2.x - p1.x)) << 40) / ((p2.y - p1.y) << 20);
  x = p1.x << 20;
  for (i = p1.y; i < p2.y; i++)
    {
      intx = x >> 20;
      if (intx < scanline_list[i].left)
	scanline_list[i].left = intx;
      if (intx > scanline_list[i].right)
	scanline_list[i].right = intx;
      x += dx_dy;
    }
};

void drawflatpoly(Face* poly)
{
  int minY, maxY, i;
  minY = 0x1000;
  maxY = 0;
  for (i = 0; i < poly->nIntroVerts; i++)
    {
      if (poly->verts[i].y < minY)
	minY = poly->verts[i].y;
      if (poly->verts[i].y > maxY)
	maxY = poly->verts[i].y;
    }
  clrhll(minY, maxY);
  scanside(poly->verts[0], poly->verts[poly->nIntroVerts - 1]);
  for (i = 0; i < poly->nIntroVerts - 1; i++)
    scanside(poly->verts[i], poly->verts[i + 1]);
  drawhll_flat(minY, maxY, poly->basecolor + poly->flatshade);
};

void g_scanside(Vertex p1, Vertex p2)
{
  int i;
  Vertex tempP;
  if (p2.y == p1.y)
    {
      if (p1.x > p2.x)
	{
	  scanline_list[p2.y].left = p2.x;
	  scanline_list[p2.y].g_shadeleft = p2.shade;
	  scanline_list[p2.y].right = p1.x;
	  scanline_list[p2.y].g_shaderight = p1.shade;
	}
      else
	{
	  scanline_list[p2.y].left = p1.x;
	  scanline_list[p2.y].g_shadeleft = p1.shade;
	  scanline_list[p2.y].right = p2.x;
	  scanline_list[p2.y].g_shaderight = p2.shade;
	}
      return;
    }
  if (p1.y > p2.y)
    {
      tempP = p2;
      p2 = p1;
      p1 = tempP;
    }
  int dx_dy, x, dshade_dy, shade, intx;
  intx = (p2.y - p1.y) << 20;
  shade = p1.shade;
  dx_dy = (((long long)(p2.x - p1.x)) << 40) / intx;
  dshade_dy = (((long long)(p2.shade - shade)) << 20) / intx;
  x = p1.x << 20;
  for (i = p1.y; i < p2.y; i++)
    {
      intx = x >> 20;
      if (intx < scanline_list[i].left)
	{
	  scanline_list[i].left = intx;
	  scanline_list[i].g_shadeleft = shade;
	}
      if (intx > scanline_list[i].right)
	{
	  scanline_list[i].right = intx;
	  scanline_list[i].g_shaderight = shade;
	}
      shade += dshade_dy;
      x += dx_dy;
    }
};

#ifndef DJ_RELEASE
static void Cdrawhll_gouraud(int minY, int maxY, unsigned char basecolor)
{
  Scanline* cscan = scanline_list + minY;
  int offset = minY * BytesPerScanline, j, i;
  int color, dcolor_dx;
  long long dcolor;
  for (i = minY; i <= maxY; i++)
    {
      if (cscan->left < cscan->right)
	{
	  offset += cscan->left;
	  j = (cscan->right - cscan->left) << 20;
	  dcolor = cscan->g_shaderight - cscan->g_shadeleft;
	  dcolor_dx = (dcolor << 20) / j;
	  color = cscan->g_shadeleft + (((int)basecolor) << 20);
	  for (j >>= 20; j > 0; j--)
	    {
	      offscreen[offset++] = (color >> 20);
	      color += dcolor_dx;
	    }
	  offset -= cscan->right;
	}
      offset += BytesPerScanline;
      cscan++;
    }
};
#endif

void drawgouraudpoly(Face* poly)
{
  int minY, maxY, i;
  minY = 0x1000;
  maxY = 0;
  for (i = 0; i < poly->nIntroVerts; i++)
    {
      if (poly->verts[i].y < minY)
	minY = poly->verts[i].y;
      if (poly->verts[i].y > maxY)
	maxY = poly->verts[i].y;
    }
  clrhll(minY, maxY);
  g_scanside(poly->verts[0], poly->verts[poly->nIntroVerts - 1]);
  for (i = 0; i < poly->nIntroVerts - 1; i++)
    g_scanside(poly->verts[i], poly->verts[i + 1]);
  drawhll_gouraud(minY, maxY, poly->basecolor);
};

void lineart_scanside(Vertex p1, Vertex p2)
{
  int i;
  Vertex tempP;
  if (p1.y == p2.y)
    {
      if (p1.x > p2.x)
	{
	  scanline_list[p2.y].left = p2.x;
	  scanline_list[p2.y].right = p1.x;
	  scanline_list[p2.y].texelsU_left = p2.u;
	  scanline_list[p2.y].texelsV_left = p2.v;
	  scanline_list[p2.y].texelsU_right = p1.u;
	  scanline_list[p2.y].texelsV_right = p1.v;
	}
      else
	{
	  scanline_list[p2.y].left = p1.x;
	  scanline_list[p2.y].right = p2.x;
	  scanline_list[p2.y].texelsU_left = p1.u;
	  scanline_list[p2.y].texelsV_left = p1.v;
	  scanline_list[p2.y].texelsU_right = p2.u;
	  scanline_list[p2.y].texelsV_right = p2.v;
	}
      return;
    }
  if (p1.y > p2.y)
    {
      tempP = p2;
      p2 = p1;
      p1 = tempP;
    }
  int dx_dy, x, u, v, du_dy, dv_dy, intx;
  intx = (p2.y - p1.y) << 20;
  u = p1.u;
  v = p1.v;
  dx_dy = (((long long)(p2.x - p1.x)) << 40) / intx;
  du_dy = (((long long)(p2.u - u)) << 20) / intx;
  dv_dy = (((long long)(p2.v - v)) << 20) / intx;
  x = p1.x << 20;
  for (i = p1.y; i < p2.y; i++)
    {
      intx = x >> 20;
      if (intx < scanline_list[i].left)
	{
	  scanline_list[i].left = intx;
	  scanline_list[i].texelsU_left = u;
	  scanline_list[i].texelsV_left = v;
	}
      if (intx > scanline_list[i].right)
	{
	  scanline_list[i].right = intx;
	  scanline_list[i].texelsU_right = u;
	  scanline_list[i].texelsV_right = v;
	}
      x += dx_dy;
      u += du_dy;
      v += dv_dy;
    }
};

#ifndef DJ_RELEASE
static void Cdrawhll_lineartexture(int minY, int maxY, int width_shift, unsigned char* pixmapbits)
{
  Scanline* cscan = scanline_list + minY;
  int offset = minY * BytesPerScanline, j, i, u, v, du_dx, dv_dx;
  unsigned rndu, rndv;
  unsigned height = BITMAP_HEIGHT(pixmapbits - 4);
  for (i = minY; i <= maxY; i++)
    {
      offset = i * BytesPerScanline;
      if (cscan->left < cscan->right)
	{
	  offset += cscan->left;
	  j = (cscan->right - cscan->left) << 20;
	  u = cscan->texelsU_left;
	  v = cscan->texelsV_left;
	  du_dx = (((long long)(cscan->texelsU_right - u)) << 20) / j;
	  dv_dx = (((long long)(cscan->texelsV_right - v)) << 20) / j;
	  for (j >>= 20; j > 0; j--)
	    {
	      rndu = u >> 20;
	      rndv = v >> 20;
	      //                  if (rndu >= (unsigned)width)
	      //                        handleError(ErrorMessage("texels outside map referenced: rndu/width", "", rndu, width));
	      if (rndv >= (unsigned)height)
		handleError(ErrorMessage("texels outside map referenced: rndv/height", "", rndv, height));
	      offscreen[offset++] = pixmapbits[(rndv << width_shift) + rndu];
	      u += du_dx;
	      v += dv_dx;
	    }
	  offset -= cscan->left;
	}
      offset += BytesPerScanline;
      cscan++;
    }
};
#endif

void drawlineartexturepoly(Face* poly, Texture* tex)
{
  int minY, maxY, i;
  minY = 0x1000;
  maxY = 0;
  for (i = 0; i < poly->nIntroVerts; i++)
    {
      if (poly->verts[i].y < minY)
	minY = poly->verts[i].y;
      if (poly->verts[i].y > maxY)
	maxY = poly->verts[i].y;
    }
  clrhll(minY, maxY);
  lineart_scanside(poly->verts[0], poly->verts[poly->nIntroVerts - 1]);
  for (i = 0; i < poly->nIntroVerts - 1; i++)
    lineart_scanside(poly->verts[i], poly->verts[i + 1]);
  drawhll_lineartexture(minY, maxY, tex->width_shift, (unsigned char*)(tex->pic + 4));
};

void tg_scanside(Vertex p1, Vertex p2)
{
  int i;
  Vertex tempP;
  if (p1.y == p2.y)
    {
      if (p1.x > p2.x)
	{
	  scanline_list[p2.y].right = p1.x;
	  scanline_list[p2.y].g_shaderight = p1.shade;
	  scanline_list[p2.y].left = p2.x;
	  scanline_list[p2.y].g_shadeleft = p2.shade;
	  scanline_list[p2.y].texelsU_left = p2.u;
	  scanline_list[p2.y].texelsV_left = p2.v;
	  scanline_list[p2.y].texelsU_right = p1.u;
	  scanline_list[p2.y].texelsV_right = p1.v;
	}
      else
	{
	  scanline_list[p2.y].right = p2.x;
	  scanline_list[p2.y].g_shaderight = p2.shade;
	  scanline_list[p2.y].left = p1.x;
	  scanline_list[p2.y].g_shadeleft = p1.shade;
	  scanline_list[p2.y].texelsU_left = p1.u;
	  scanline_list[p2.y].texelsV_left = p1.v;
	  scanline_list[p2.y].texelsU_right = p2.u;
	  scanline_list[p2.y].texelsV_right = p2.v;
	}
      return;
    }
  if (p1.y > p2.y)
    {
      tempP = p2;
      p2 = p1;
      p1 = tempP;
    }
  int dx_dy, x, u, v, du_dy, dv_dy, intx, shade, dshade_dy;
  x = p1.x << 20;
  u = p1.u;
  v = p1.v;
  shade = p1.shade;
  intx = (p2.y - p1.y) << 20;
  dx_dy = (((long long)(p2.x - p1.x)) << 40) / intx;
  du_dy = (((long long)(p2.u - u)) << 20) / intx;
  dv_dy = (((long long)(p2.v - v)) << 20) / intx;
  dshade_dy = (((long long)(p2.shade - shade)) << 20) / intx;
  for (i = p1.y; i < p2.y; i++)
    {
      intx = x >> 20;
      if (intx < scanline_list[i].left)
	{
	  scanline_list[i].left = intx;
	  scanline_list[i].g_shadeleft = shade;
	  scanline_list[i].texelsU_left = u;
	  scanline_list[i].texelsV_left = v;
	}
      if (intx > scanline_list[i].right)
	{
	  scanline_list[i].right = intx;
	  scanline_list[i].g_shaderight = shade;
	  scanline_list[i].texelsU_right = u;
	  scanline_list[i].texelsV_right = v;
	}
      x += dx_dy;
      u += du_dy;
      v += dv_dy;
      shade += dshade_dy;
    }
};

#ifndef DJ_RELEASE
static void Cdrawhll_lineartexture_gouraud(int minY, int maxY, int width_shift, unsigned char* pixmapbits)
{
  Scanline* cscan = scanline_list + minY;
  int offset = minY * BytesPerScanline, j, i, u, v, du_dx, dv_dx, dshade_dx, shade;
  unsigned char pixmapcolor;
  unsigned rndu, rndv, rndshade;
  unsigned height = BITMAP_HEIGHT(pixmapbits - 4);
  for (i = minY; i <= maxY; i++)
    {
      offset = i * BytesPerScanline;
      if (cscan->left < cscan->right)
	{
	  offset += cscan->left;
	  j = (cscan->right - cscan->left) << 20;
	  u = cscan->texelsU_left;
	  v = cscan->texelsV_left;
	  shade = cscan->g_shadeleft;
	  du_dx = (((long long)(cscan->texelsU_right - u)) << 20) / j;
	  dv_dx = (((long long)(cscan->texelsV_right - v)) << 20) / j;
	  dshade_dx = (((long long)(cscan->g_shaderight - shade)) << 20) / j;
	  for (j >>= 20; j > 0; j--)
	    {
	      rndu = u >> 20;
	      rndv = v >> 20;
	      rndshade = shade >> 20;
	      //                  if (rndu >= (unsigned)width_shift)
	      //                        handleError(ErrorMessage("texels outside map referenced: rndu/width", "", rndu, width));
	      if (rndv >= (unsigned)height)
		handleError(ErrorMessage("texels outside map referenced: rndv/height", "", rndv, height));
	      if (rndshade >= (unsigned)SHADES_PER_COLOR)
		handleError(ErrorMessage("shade out of range: rndshade", "", rndshade));
	      pixmapcolor = pixmapbits[(rndv << width_shift) + rndu];
	      offscreen[offset++] = *(shadinglookie[pixmapcolor] + rndshade);
	      u += du_dx;
	      v += dv_dx;
	      shade += dshade_dx;
	    }
	  offset -= cscan->left;
	}
      offset += BytesPerScanline;
      cscan++;
    }
};
#endif

void drawtexturegouraudpoly(Face* poly, Texture* tex)
{
  int minY, maxY, i;
  minY = 0x1000;
  maxY = 0;
  for (i = 0; i < poly->nIntroVerts; i++)
    {
      if (poly->verts[i].y < minY)
	minY = poly->verts[i].y;
      if (poly->verts[i].y > maxY)
	maxY = poly->verts[i].y;
    }
  clrhll(minY, maxY);
  tg_scanside(poly->verts[0], poly->verts[poly->nIntroVerts - 1]);
  for (i = 0; i < poly->nIntroVerts - 1; i++)
    tg_scanside(poly->verts[i], poly->verts[i + 1]);
  /*unsigned char* pixmapbits = (unsigned char*)(tex->pic + 4);
    for (i = minY; i <= maxY; i++)
    {
    if (scanline_list[i].left < scanline_list[i].right)
    tg_hline(i, tex->width, pixmapbits);
    }*/
  drawhll_lineartexture_gouraud(minY, maxY, tex->width_shift, (unsigned char*)(tex->pic + 4));
};

void drawwireframe(Face* poly)
{
  int i; // clipline?
  line(poly->verts[poly->nIntroVerts - 1].x, poly->verts[poly->nIntroVerts - 1].y, poly->verts[0].x, poly->verts[0].y, poly->basecolor + poly->flatshade);
  for (i = 0; i < poly->nIntroVerts - 1; i++)
    line(poly->verts[i].x, poly->verts[i].y, poly->verts[i + 1].x, poly->verts[i + 1].y, poly->basecolor + poly->flatshade);
};

void brandmarkFace(Face* poly)
{
  int i; // clipline?
  line(poly->verts[poly->nIntroVerts - 1].x, poly->verts[poly->nIntroVerts - 1].y, poly->verts[0].x, poly->verts[0].y, NEAREST[CCWHITE]);
  for (i = 0; i < poly->nIntroVerts - 1; i++)
    line(poly->verts[i].x, poly->verts[i].y, poly->verts[i + 1].x, poly->verts[i + 1].y, NEAREST[CCWHITE]);
};


void scaleimage(char** image, unsigned newwidth, unsigned newheight)
{
  float sX, sY;
  unsigned ox, oy, width, height, x, y;
  width = BITMAP_WIDTH(*image);
  height = BITMAP_HEIGHT(*image);
  sX = width / (float)newwidth;
  sY = height / (float)newheight;
  char* result = Imageblock(newwidth, newheight);
  result[0] = LOW_BYTE(newwidth);
  result[1] = HIGH_BYTE(newwidth);
  result[2] = LOW_BYTE(newheight);
  result[3] = HIGH_BYTE(newheight);
  for (y = 0; y < newheight; y++)
    for (x = 0; x < newwidth; x++)
      {
	ox = (int)((x * sX) + 0.5F);
	oy = (int)((y * sY) + 0.5F);
	if (ox >= width)
	  ox = width - 1;
	if (oy >= height)
	  oy = height - 1;
	result[4 + (y * newwidth) + x] = (*image)[4 + (oy * width) + ox];
      }
  xfree(*image);
  *image = result;
};

#ifdef TEST_VBE

#include "fifo99.h"

int main(int noargs, char** args)
{
  char* pic, *pal;
  loadpalette("shadelin.pal", &pal);
  loadspr_256(args[1], &pic, TRANSLATE_PAL, pal);
  if (pic == NULL)
    return 1;
  if (!openVBE(640, 480))
    return 1;
  setpal(pal);
  installkeyhandler();
  updatecolors();
  int picx, picy;
  clearoffscreenmem();
  scaleimage(&pic, 640, 480);
  putimage(0, 0, pic);
  setblackpal();
  flipoffscreenmem();
  fadepalinto(pal, SLOW);
  while (!getkey(KEY_ESC)) ;
  fadepalout(SLOW);
  restorekeyhandler();
  closeVBE();
  xfree(pic);
  xfree(pal);
  return 0;
}
#endif
