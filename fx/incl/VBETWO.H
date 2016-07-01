#ifndef __VBETWO_H
#define __VBETWO_H

typedef enum {
  vbeMemTEXT     = 0,           	/* Text mode memory model           */
  vbeMemCGA      = 1,            	/* CGA style mode                   */
  vbeMemHercules = 2,            	/* Hercules graphics style mode     */
  vbeMemPlanar   = 3,            	/* 16 color VGA style planar mode   */
  vbeMemPacked   = 4,            	/* Packed pixel mode                */
  vbeMemModeX    = 5,           	/* Non-chain 4, 256 color (ModeX)   */
  vbeMemRGB      = 6,            	/* Direct color RGB mode            */
  vbeMemYUV      = 7            	/* Direct color YUV mode            */
} VBE_memModels;

/* Flags for the mode attributes returned by VBE_getModeInfo. If
  vbeModeNonBanked is set to 1 and vbeModeLinear is also set to 1, then only
  the linear framebuffer mode is available. */
#define vbeModeAvailable	   0x0001		/* Video mode is available		*/
#define vbeModeTTYOutput	   0x0004		/* TTY BIOS output is supported		*/
#define vbeModeColorMode	   0x0008		/* Mode is a color video mode		*/
#define vbeModeGraphMode	   0x0010		/* Mode is a graphics mode		*/
#define vbeModeNonVGA	         0x0020	        /* Mode is not VGA compatible		*/
#define vbeModeNonBanked	   0x0040		/* Banked mode is not supported		*/
#define vbeModeLinear	         0x0080	        /* Linear mode supported		*/
#define vbeModeDoubleScan	   0x0100		/* Mode is double scanned		*/
#define vbeModeInterlaced	   0x0200		/* Supports interlaced timings		*/
#define vbeModeTripleBuffer 	   0x0400		/* Mode supports HW triple buffer	*/
#define vbeModeStereo	         0x0800	        /* Mode supports hardware stereo	*/
#define vbeModeDualDisplay       0x1000	        /* Mode supports dual display start	*/
#define vbeModeAvailablewithLFB  (vbeModeAvailable | vbeModeColorMode | vbeModeGraphMode | vbeModeNonBanked | vbeModeLinear)

#define PACKED __attribute__ ((packed))

#pragma pack(1) //this is done to fix a bug in DJGPP V2.00
struct VBE_MODEINFO
{
  unsigned short ModeAttributes PACKED;        /* Mode attributes                  */
  unsigned char WinAAttributes PACKED;         /* Window A attributes              */
  unsigned char WinBAttributes PACKED;         /* Window B attributes     	  */
  unsigned short WinGranularity PACKED;        /* Window granularity in k 	  */
  unsigned short WinSize PACKED;               /* Window size in k        	  */
  unsigned short WinASegment PACKED;    	     /* Window A segment                 */
  unsigned short WinBSegment PACKED;    	     /* Window B segment                 */
  unsigned short WinFuncPtrOffset PACKED;	     /* Pointer to window function(Offset)*/
  unsigned short WinFuncPtrSegment PACKED;     /* Pointer to window function(Segment)*/
  unsigned short BytesPerScanline PACKED;      /* Bytes per scanline      	  */
  // VBE 1.2+
  unsigned short XResolution PACKED;           /* Horizontal resolution            */
  unsigned short YResolution PACKED;           /* Vertical resolution              */
  unsigned char XCharSize PACKED;              /* Character cell width             */
  unsigned char YCharSize PACKED;              /* Character cell height            */
  unsigned char NumberOfPlanes PACKED;         /* Number of memory (bit)planes     */
  unsigned char BitsPerPixel PACKED;           /* Bits per pixel                   */
  unsigned char NumberOfBanks PACKED;          /* Number of CGA style banks        */
  unsigned char MemoryModel PACKED;            /* Memory model type                */
  unsigned char BankSize PACKED;               /* Size of CGA style banks          */
  unsigned char NumberOfImagePages PACKED;     /* Number of images pages (Max)     */
  unsigned char reserved1 PACKED;              /* Reserved for page function ???   */
  // required for direct color memory models (RGB, YUV)
  unsigned char RedMaskSize PACKED;            /* Size of direct color red mask    */
  unsigned char RedFieldPosition PACKED;       /* Bit posn of lsb of red mask      */
  unsigned char GreenMaskSize PACKED;          /* Size of direct color green mask  */
  unsigned char GreenFieldPosition PACKED;     /* Bit posn of lsb of green mask    */
  unsigned char BlueMaskSize PACKED;           /* Size of direct color blue mask   */
  unsigned char BlueFieldPosition PACKED;      /* Bit posn of lsb of blue mask     */
  unsigned char RsvdMaskSize PACKED;           /* Size of direct color res mask    */
  unsigned char RsvdFieldPosition PACKED;      /* Bit posn of lsb of res mask      */
  unsigned char DirectColorModeInfo PACKED;    /* Direct color mode attributes     */
  // VBE 2.0+
  unsigned long PhysicalBasePtr PACKED;        /* Physical address for LFB         */
  unsigned long OffScreenMemOffset PACKED;     /* Pointer to Offscreen-memory      */
  unsigned short OffScreenMemSize PACKED;       /* Size of Offscreen-memory in K    */
  unsigned char reserved2[206] PACKED;
};

struct VBEINFO
{
  char VESASignature[4] PACKED;               /* 'VESA' 4 byte signature          */
  short VBEVersion PACKED;                    /* VBE version number               */
  unsigned short OEMStringPtrOffset PACKED;   /* Pointer to OEM string (Offset)   */
  unsigned short OEMStringPtrSegment PACKED;  /* Pointer to OEM string (Segment)  */
  unsigned long Capabilities PACKED;          /* Capabilities of video card       */
  unsigned short VideoModePtrOffset PACKED;   /* Pointer to supported modes(Offset)*/
  unsigned short VideoModePtrSegment PACKED;  /* Pointer to supported modes(Segment)*/
  unsigned short TotalMemory PACKED;          /* Number of 64kb memory blocks     */
  // VBE 2.0+
  unsigned short OEMSoftwareRevision PACKED;
  char* OEMVendorNamePtr PACKED;
  char* OEMProductNamePtr PACKED;
  char* OEMProductRevPtr PACKED;
  unsigned char reserved[222] PACKED;
  unsigned char OEMdata[256] PACKED;
};

struct VBE_PM_Interface
{
  unsigned short setWindow PACKED;
  unsigned short setDisplayStart PACKED;
  unsigned short setPrimaryPalette PACKED;
  unsigned short IOPrivInfo PACKED;
};

struct rasterpoint
{
  int x PACKED;
  int y PACKED;
};

#pragma pack()

#include "xpos.h"
#include "lcstdlib.h" // xmalloc/xfree

#define VBE_MODEFLAG_LFB        0x4000
#define VBE_MODEFLAG_DONTCLEAR  0x8000

// globals
extern VBEINFO vbe_info;
extern VBE_MODEINFO vbe_modeinfo;
extern int xres, yres;

bool openVBE(int width, int height, unsigned char bpp = 8);
void closeVBE();
void init_shadinglookups(char* shadepal);
void try_free_shadinglookups();
void setClipRect(int left, int top, int right, int bottom);

inline char* Imageblock(int height, int width)
{
  return (char*)xmalloc((height * width) + 4);
};

extern void come_on_push_it(); // S3 speedup
extern int setLogicalScanlineWidthPixels(int widthPixels);
//int setLogicalScanlineWidthBytes(int widthBytes);
extern int getLogicalScanlineWidthPixels();
//int getLogicalScanlineWidthBytes();
extern int setDisplayStart(int x, int y);
//int setDisplayStartVR(int x, int y);
extern int getDisplayStart(int* x, int* y);

extern void putpixel(int x, int y, unsigned char color);
extern unsigned char getpixel(int x, int y);
extern void clearoffscreenmem();
extern void flipoffscreenmem();
extern void putbgimage(char* full_screen_img_bits);
extern void putimage(int left, int top, char* buffer);
extern void clipputimage(int left, int top, char* buffer);
extern void getimage(int left, int top, int width, int height, char* buffer);
extern void hline(int x1, int x2, int y, unsigned char color);
extern void clip_hline(int x1, int x2, int y, unsigned char color);
extern void vline(int y1, int y2, int x, unsigned char color);
extern void floatt_hline(int x1, int x2, int y, int fixed20_tX1, int fixed20_tY1, int fixed20_tX2, int fixed20_tY2, int width, unsigned char* pixmapbits);
extern void drawhll_flat(int minY, int maxY, unsigned char color);
extern void drawhll_gouraud(int minY, int maxY, unsigned char basecolor);
extern void drawhll_lineartexture(int minY, int maxY, int width_shift, unsigned char* pixmapbits);
extern void drawhll_lineartexture_gouraud(int minY, int maxY, int width_shift, unsigned char* pixmapbits);

void saveScreen(const char* szfilename, char* pal = 0);
void circle(int cx, int cy, int cr, unsigned char col); // round in 4x3 modes
void clipcircle(int cx, int	cy, int cr,	unsigned char col);
void thickcircle(int cx, int cy, int cr, unsigned char col); // round in 4x3 modes
void fillcircle(int cx, int cy, int cr, unsigned char col); // round in 4x3 modes
void clip_fillcircle(int cx, int cy, int cr, unsigned char col); // round in 4x3 modes
void line(int x1, int y1, int x2, int y2, unsigned char color);
void thickline(int x1, int y1, int x2, int y2, unsigned char color);
void clipline(int x1, int y1, int x2, int y2, unsigned char color);
void rectangle(int left, int top, int right, int bottom, unsigned char color);
void fillrect(int left, int top, int right, int bottom, unsigned char color);
void drawplasma(void);
void drawwireframe(Face* poly);
void drawflatpoly(Face* poly);
void drawgouraudpoly(Face* poly);
void drawlineartexturepoly(Face* poly, Texture* tex);
void drawtexturegouraudpoly(Face* poly, Texture* tex);
void brandmarkFace(Face* poly);
void scaleimage(char** image, unsigned newwidth, unsigned newheight);

#endif
