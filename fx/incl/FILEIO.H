#ifndef __FILEIO_H
#define __FILEIO_H

#include <stdio.h>
#include <string.h>
#include "quitbug.h"

class SafeIO
{
  FILE* FObject;
  unsigned long offset;
  char szfilename[200];

 public:

  FILE* getFILEObject()
    {
      return FObject;
    };

  unsigned long get_offset()
    {
      if (fgetpos(FObject, &offset) == 0)
	return offset;
      else
	handleError(ErrorMessage("Error getting offset of file", szfilename));
      return 0; // dummy
    };

  void writeb(void* buffer, unsigned int bytes)
    {
      get_offset();
      //        _go32_dpmi_lock_data(buffer, bytes);
      if (fwrite(buffer, 1, bytes, FObject) != bytes)
	handleError(ErrorMessage("Error writing to file", szfilename, offset));
    };

  void write(void* buffer, unsigned int size, unsigned int count)
    {
      get_offset();
      //        _go32_dpmi_lock_data(buffer, size * count);
      if (fwrite(buffer, size, count, FObject) != count)
	handleError(ErrorMessage("Error writing to file", szfilename, offset));
    };

  void readb(void* buffer, unsigned int bytes)
    {
      get_offset();
      //        _go32_dpmi_lock_data(buffer, bytes);
      if (fread(buffer, 1, bytes, FObject) != bytes)
	handleError(ErrorMessage("Error reading from file", szfilename, offset));
    };

  void read(void* buffer, unsigned int size, unsigned int count)
    {
      //        _go32_dpmi_lock_data(buffer, size * count);
      get_offset();
      if (fread(buffer, size, count, FObject) != count)
	handleError(ErrorMessage("Error reading from file", szfilename, offset));
    };

  void goto_EOF(int reloffset)
    {
      if (fseek(FObject, reloffset, SEEK_END) != 0)
	handleError(ErrorMessage("Error trying to seek to EOF + [PARAM1]", "", reloffset));
    };

  void goto_BEG(int reloffset)
    {
      if (fseek(FObject, reloffset, SEEK_SET) != 0)
	handleError(ErrorMessage("Error trying to seek to BEGIN_FILE + [PARAM1]", "", reloffset));
    };

  void goto_CUR(int reloffset)
    {
      if (fseek(FObject, reloffset, SEEK_CUR) != 0)
	handleError(ErrorMessage("Error trying to seek to CURRENT + [PARAM1]", "", reloffset));
    };

  unsigned char operator[](unsigned int rdoffset)
    {
      get_offset();
      goto_BEG(rdoffset);
      int ret = fgetc(FObject);
      if (ret == EOF)
	handleError(ErrorMessage("Error reading at offset [PARAM1], EOF", szfilename, rdoffset));
      if (fsetpos(FObject, &offset) != 0)
	handleError(ErrorMessage("Error returning to offset [PARAM1]", szfilename, offset));
      return (unsigned char)ret;
    };

  unsigned long size()
    {
      get_offset();
      goto_EOF(0);
      unsigned long eofoffset;
      if (fgetpos(FObject, &eofoffset) != 0)
	handleError(ErrorMessage("Error getting offset of file", szfilename));
      if (fsetpos(FObject, &offset) != 0)
	handleError(ErrorMessage("Error returning to offset [PARAM1]", szfilename, offset));
      return eofoffset;
    };

  unsigned char getchar()
    {
      int ret = fgetc(FObject);
      if (ret == EOF)
	handleError(ErrorMessage("Reached EOF while trying to get next byte", "", offset));
      return (unsigned char)ret;
    };

  void close()
    {
      if (FObject != NULL)
	{
	  if (fclose(FObject) != 0)
	    handleError(ErrorMessage("Error closing file", szfilename));
	  FObject = NULL;
	}
    };

  SafeIO(const char* iszfilename, const char* mode)
    {
      strcpy(szfilename, iszfilename);
      FObject = fopen(iszfilename, mode);
      if (FObject == NULL)
	handleError(ErrorMessage("Error opening file", szfilename));
    };

  ~SafeIO() { close(); };
};

bool fexists(const char* szfilename);

#endif
