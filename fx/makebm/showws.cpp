#include "vbetwo.hpp"
#include "keyinp.hpp"

int main(int nargs, char** args)
{
if (nargs != 2)
   {
   printf("SYNTAX: showws\t[filename.spr] \n");
   return 0;
   }
if (!fexists(args[1]))
   {
   printf("Can't find %s.\n", args[1]);
   return 1;
   }
char *pic, *pal;
pal = loadspr_256(args[1], &pic, USE_PAL);
if (!openVBE(1024, 768))
   return 1;
setpal(pal);
xfree(pal);
scaleimage(&pic, 1024, 768);
clearoffscreenmem();
putimage(0, 0, pic);
flipoffscreenmem();
installkeyhandler();
while (!peekanykey()) ;
restorekeyhandler();
xfree(pic);
closeVBE();
}
