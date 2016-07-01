#include "fileio.h"
#include <string.h>

bool fexists(const char* szfilename)
{
  FILE* test = fopen(szfilename, "rb");
  if (test == NULL)
    return false;
  else
    {
      fclose(test);
      return true;
    }
};
