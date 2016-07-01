#ifndef __GFORMATS_H
#define __GFORMATS_H

#include <stdlib.h> // for declaration of NULL

#define PACKED __attribute__ ((packed))

#pragma pack(1) //this is done to fix a bug in DJGPP V2.00
struct DIB_fileheader
{
  unsigned short type PACKED;
  unsigned long filesize PACKED;
  unsigned long reserved PACKED;// set to 0
  unsigned long offsetbits PACKED;
};

struct DIB_infoheader
{
  unsigned long structsize PACKED;
  unsigned long width PACKED;
  unsigned long height PACKED;
  unsigned short planes PACKED;// set to 1
  unsigned short bpp PACKED;
  unsigned long compression PACKED;// 0 for none
  unsigned long bmsize PACKED;// required for compression
  unsigned long Xpixelspermeter PACKED;
  unsigned long Ypixelspermeter PACKED;
  unsigned long colorsused PACKED;
  unsigned long colorsimportant PACKED;
};

struct rgbQuad
{
  unsigned char blue PACKED;
  unsigned char green PACKED;
  unsigned char red PACKED;
  unsigned char reserved PACKED;
};
#pragma pack()

enum BGDIB_DISPLAY_MODE { BGDIB_CENTER, BGDIB_STRETCH };
enum PAL_MODE { USE_PAL, TRANSLATE_PAL };
enum DIB_FILE_TYPE
{ DIB_FILE_DOES_NOT_EXIST = 0x00, DIB_FILE_NO_DIB, DIB_FILE_COMPRESSED,
  DIB_FILE_MONO, DIB_FILE_4, DIB_FILE_8, DIB_FILE_16, DIB_FILE_24,
  DIB_FILE_32, DIB_FILE_UNKNOWN };

//macros to make life easier
#define BITMAP_WIDTH(x)   ( ((x)[0] & 0x00FF) + ((((x)[1] & 0x00FF) << 8) & 0xFF00) )
#define BITMAP_HEIGHT(x)  ( ((x)[2] & 0x00FF) + ((((x)[3] & 0x00FF) << 8) & 0xFF00) )
#define LOW_BYTE(x)       ( (x) & 0x000000FF )
#define HIGH_BYTE(x)       ( (((x) & 0x0000FF00) >> 8) & 0x000000FF )
#define DIB_FILE_CANT_HANDLE ( (GF_DIB_type == DIB_FILE_DOES_NOT_EXIST) || (GF_DIB_type == DIB_FILE_NO_DIB) || (GF_DIB_type == DIB_FILE_COMPRESSED) || (GF_DIB_type == DIB_FILE_MONO) || (GF_DIB_type == DIB_FILE_4) || (GF_DIB_type == DIB_FILE_16) || (GF_DIB_type == DIB_FILE_32) || (GF_DIB_type == DIB_FILE_UNKNOWN) )

extern DIB_FILE_TYPE GF_DIB_type;
extern DIB_fileheader GF_fheader;
extern DIB_infoheader GF_iheader;


char* loadspr_256(const char* szfilename, char** bitmap, PAL_MODE palmode, char* destPAL = NULL);
void savespr_256(const char* szfilename, char* bitmap, char* associatedPAL = NULL);
char* loadDIB_256(const char* szfilename, char** bitmap, PAL_MODE palmode, char* destPAL = NULL);
void saveDIB_256(const char* szfilename, char* bitmap, char* associatedPAL = NULL);
int createcolorlist(char* bitmap, char* colorlist);
void check_DIB(const char* szfilename);
// this also fills in GF_fheader & GF_iheader (needs to be called before loadDIB_256!!)

#endif
