#include "gformats.h"
#include "fifo99.h"
#include "palettes.h"
#include "fileio.h"
#include <dir.h>
#include <stdio.h>
#include <string.h>

char szdestfile[1000];
char* pal = NULL;

bool isSpritePalequaltoTHEPalette()
{
  SafeIO sprite(szdestfile, "rb");
  char spritepal[0x300];
  sprite.readb(spritepal, 0x300);
  sprite.close();
  bool equal = true;
  int i;
  for (i = 0; i < 0x300; i++)
    {
      if (spritepal[i] != pal[i])
	equal = false;
    }
  return equal;
};

long getFileSize(char* fname) {
  ffblk f;
  if (findfirst(fname, &f, FA_HIDDEN | FA_RDONLY) == 1) // if 'done'
    return -1;
  else
    return f.ff_fsize;
} 

int main(int argc, char** args)
{
  bool shutup = false, prompt = false; // shutup = overwrite all without asking
  int i, yes, no, all, escapism, bin_cmp;

#ifndef DJ_RELEASE
  for (i = 0; i < argc; i++) {
    printf("arg%i: %s\t", i, args[i]);
  }
  printf("\n\n");
#endif

  if (argc < 2)
    {
      printf("MAKEALL will convert all windows DIB's (searching for the wildcards you enter) to sprites with the same filename and the extension .SPR\n");
      printf("SYNTAX:\tMAKEALL [filename_with_wildcards] [-prompt (prompts you everytime when replacing files)] ['-usepal <6bit (vga)-palette>']\n");
      printf("If 'prompt' is not enabled, it checks whether to overwrite a sprite file depending upon a binary compare between its palette and the 'universal pal' (univ.pal)\n");
      return 0;
    }

  for (i = 1; i < argc; i++) { // look for options
      if (stricmp(args[i], "-prompt") == 0) {
	  prompt = true;
	  continue;
	}
      if (stricmp(args[i], "-usepal") == 0) {
	  i++;
	  if (!fexists(args[i])) {
	      printf("Can't find palette %s.\n", args[i]);
	      return 1;
	    }
	  loadpalette(args[i], &pal);
	  continue;
	}
      if (*(args[i] + 0) == '-') {
	printf("Invalid argument: %s.\n", args[i]);
	return 1;
      }
    }

  if (pal == NULL) // if no user-defined palette has been loaded ...
    {
      if (!fexists("univ.pal")) 
	{
	  printf("Universal palette (univ.pal) not found.\n");
	  return 1;
	} // .. load the universal palette
      loadpalette("univ.pal", &pal);
    }
  unsigned long totalSize = 0, doneSize = 0, temp;
  char* pic;

  // find total size
  int count = 1;
  while (count < argc) {
    if (stricmp(args[count], "-prompt") == 0) {
      count++;
      continue;
    } else if (stricmp(args[count], "-usepal") == 0) {
      count += 2; // skip the parameter as well
      continue;	
    } else {
      temp = getFileSize(args[count]);
      if (temp != -1)
	totalSize += temp;
    }
    count++;
  }

  count = 1;
  while (count < argc)
    {
      if (stricmp(args[count], "-prompt") == 0) {
	count++;
	continue;
      }
      if (stricmp(args[count], "-usepal") == 0) {
	count += 2; // skip the parameter as well
	continue;	
      }	
      check_DIB(args[count]);
      if (DIB_FILE_CANT_HANDLE) {
	  switch(GF_DIB_type)
	    {
	    case DIB_FILE_DOES_NOT_EXIST: // file doesn't exist
	      printf("No such file: %s, skipping to next argument..\n", args[count]);
	      count++;
	      continue;
	    case DIB_FILE_NO_DIB:
	      printf("%s is no Windows DIB (wrong ID).\n", args[count]);
	      break;
	    case DIB_FILE_COMPRESSED:
	      printf("%s is compressed (can't handle that).\n", args[count]);
	      break;
	    case DIB_FILE_MONO:
	      printf("%s is a monochrome bitmap (this program only converts 24- or 8bit DIB's).\n", args[count]);
	      break;
	    case DIB_FILE_4:
	      printf("%s is a 16-color bitmap (this program only converts 24- or 8bit DIB's).\n", args[count]);
	      break;
	    case DIB_FILE_16:
	      printf("%s is a 16-bit bitmap (this program only converts 24- or 8bit DIB's).\n", args[count]);
	      break;
	    case DIB_FILE_32:
	      printf("%s is a 32-bit bitmap (this program only converts 24- or 8bit DIB's).\n", args[count]);
	      break;
	    case DIB_FILE_UNKNOWN:
	      printf("%s is an unknown type of DIB.\n", args[count]);
	      break;
	    default:
	      printf("Unknown Error in program: %i!\n", (int)GF_DIB_type);
	      abort();
	    }
	  count++; // skip file because I can't handle it
	  doneSize += getFileSize(args[count]);
	  continue; 
	}
      strcpy(szdestfile, args[count]);
      i = 0;
      do
	{
	  i++;
	} while (szdestfile[i] != '.');
      szdestfile[i + 1] = 's';
      szdestfile[i + 2] = 'p';
      szdestfile[i + 3] = 'r';
      szdestfile[i + 4] = '\0';
      if (fexists(szdestfile) && prompt && !shutup)
	{
	  printf("%s already exists. Overwrite ? (Yes/No/All/replace only if Palettes are not equal) (Press any capitalized letter)\n", szdestfile);
	  installkeyhandler();
	  do
	    {
	      yes = getkey(KEY_Y);
	      no = getkey(KEY_N);
	      all = getkey(KEY_A);
	      bin_cmp = getkey(KEY_P);
	      escapism = getkey(KEY_ESC);
	    } while (!yes && !no && !all && !escapism && !bin_cmp);
	  restorekeyhandler();
	  if (no) {
	    printf("Skipping %s.\n", args[count]);
	    count++; // skip this file
	    doneSize += getFileSize(args[count]);
	    continue;
	  }
	  if (all)
	    shutup = true;
	  if (escapism) {
	      printf("Program canceled.\n");
	      return 1;
	    }
	  if (bin_cmp) {
	      if (isSpritePalequaltoTHEPalette()) {
		printf("Skipping %s (palettes are equal).\n", args[count]);
		doneSize += getFileSize(args[count]);
		count++; // skip this file (palettes are equal)
		continue; 
	      }
	    }
	}
      if (!shutup && fexists(szdestfile)) { // default behavior: if !prompt && !shutup
	  if (isSpritePalequaltoTHEPalette()) {
	    printf("Auto-Mode: Skipping %s (palettes are equal).\n", args[count]);
	    count++;
	    doneSize += getFileSize(args[count]);
	    continue;
	  }
	}
      if (GF_DIB_type == DIB_FILE_8)
	printf("Converting %s (%i x %i x 256) to %s...", args[count],
	       (int)GF_iheader.width, (int)GF_iheader.height, szdestfile);
      else
	printf("Converting %s (%i x %i x 16M) to %s...", args[count],
	       (int)GF_iheader.width, (int)GF_iheader.height, szdestfile);
      loadDIB_256(args[count], &pic, TRANSLATE_PAL, pal);
      savespr_256(szdestfile, pic, pal);
      xfree(pic);
      printf("done.\n");
      doneSize += getFileSize(args[count]);
      printf("%20.2f percent done.\n", 100.0F * doneSize / (float)totalSize);
      int blocks = (doneSize)/(float)totalSize * 80;
      for (i = 0; i < blocks; i++) {
	printf(".");
      }
      printf("\n");
      count++;
    }
  xfree(pal);
  printf("Finished converting.\n");
}


