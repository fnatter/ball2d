#ifndef __PALETTES_H
#define __PALETTES_H

enum NEARESTCC { CCRED, CCGREEN, CCBLUE, CCDARKRED, CCDARKGREEN, CCDARKBLUE,
		 CCYELLOW, CCORANGE, CCVIOLET, CCCYAN, CCBROWN, CCWHITE, CCGRAY, CCDARKGRAY,
		 CCBLACK };

enum PAL_FADE_SPEED { SLOW = 1, NORMAL = 2, FAST = 4, VERY_FAST = 8 };
struct RGBcolor { unsigned char red, green, blue; };
// 6bitColor = (255.0 * 8bitColor / 63.0) [at least it's more accurate than 8bitColor / 4.0]
#define COLOR_8to6(x) ( (char)(0.2470588F * (float)(x)) )
// 8bitColor = ((6bitColor + 1) * 4) - 1 [gives a little better results than 6bitColor * 4]
#define COLOR_6to8(x) ( (unsigned char)((((x) + 1) * 4) - 1) )
// remember that if you change this, also change _findnearestcolor_8

#define PAL_RED(x)  (((x) * 3) + 0)
#define PAL_GREEN(x)  (((x) * 3) + 1)
#define PAL_BLUE(x)  (((x) * 3) + 2)

#define SHADES_PER_COLOR        16

extern unsigned char NEAREST[15];

unsigned char* createshadinglookup(char* pal, int noShades, unsigned char index);
void createlinearshadepal(char* shade); // needs 256 * 3 bytes
void createshadepal(char* shade); // needs 256 * 3 bytes
void createsmoothpal(char* pal); // needs 512 * 3 bytes
void savepalette(const char* szfilename, char* pal = 0L, int colors = 256);
int loadpalette(const char* szfilename, char** nullPpal); // returns colors in pal, pass &(NULLPointer), allocates mem according to palsize
void rotatepalup(char* pal, int first, int last);// needs (last + 1) * 3 bytes
void rotatepaldown(char* pal, int first, int last);// needs (last + 1) * 3 bytes
void fadepalinto(char* pal, PAL_FADE_SPEED speed);
void fadepalout(PAL_FADE_SPEED speed);
unsigned char findnearestbasecolor(char* currentpal, char r, char g, char b);
unsigned char findnearestbasecolor_8(char* currentpal, unsigned char r, unsigned char g, unsigned char b);
unsigned char findnearestcolor(char* currentpal, RGBcolor rgb);
char* findtransitiontable(char* destpal, char* sourcepal);
void createContrastTable(char* pal, unsigned char* table);
unsigned char findcontrastingindex(char* currentpal, unsigned char index);
void updatecolors();

// defined in vesalow.asm
extern void waitVR(void);
extern void setpal(char* pal); // needs 256 * 3 bytes
extern void getpal(char* pal); // needs 256 * 3 bytes
extern void setblackpal();
extern unsigned char findnearestcolor(char* currentpal, char r, char g, char b);
extern unsigned char findnearestcolor_8(char* currentpal, unsigned char r, unsigned char g, unsigned char b);

#endif
