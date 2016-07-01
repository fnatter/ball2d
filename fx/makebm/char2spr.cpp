#include <stdio.h>
#include "vbetwo.hpp"
#include "keyinp.hpp"

char charpattern[128][8];

void getcharpattern()
{
unsigned char character;
//get 128 * 8 bytes (1024) @ 0xF000:0xFA6E, physical_address = 0xF0000 + 0x0FA6E = 0xFFA6E
int physical = 0xFFA6E;
for (character = 0; character < 128; character++)
	{
	dosmemget(physical, 8, charpattern[character]);
	physical += 8;
	}
};

int main(int nargs, char** args)
{
if (nargs != 3)
   {
   printf("SYNTAX:\tCHAR2SPR [decimal ASCII keycode [0;128[] [filename.spr]\n");
   return 0;
   }
int ASCII = atoi(args[1]);
if (ASCII >= 128)
   {
   printf("ASCII code must be <= 128.\n");
   return 1;
   }
getcharpattern();
char* pic = Imageblock(8, 8);
pic[0] = pic[2] = 0x08;
pic[1] = pic[3] = 0x00;
int x, y;
openVBE(320, 240);
updatecolors();
for (y = 0; y < 8; y++)
    for (x = 7; x >= 0; x--)
        {
        if (charpattern[ASCII][y] & (1 << x))
            pic[4 + (y * 8) + (7 - x)] = NEAREST[CCWHITE];
        else
            pic[4 + (y * 8) + (7 - x)] = 0x00;
        }
savespr_256(args[2], pic);
clearoffscreenmem();
rectangle(49, 9, 101, 101, NEAREST[CCYELLOW]);
scaleimage(&pic, 100, 100);
putimage(50, 10, pic);
flipoffscreenmem();
installkeyhandler();
while (!peekanykey()) ;
restorekeyhandler();
closeVBE();
xfree(pic);
return 0;
}
