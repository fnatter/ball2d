#include <stdio.h>
#include <dir.h>

int main(int argc, char** args) {
  char* fname = args[1];
  struct ffblk f;
  FILE *infile, *outfile;
  char* buffer;
  int i;
  if (argc < 3) {
    printf("Syntax error: this should be called by make to change the format\nof the generated dependencies!\n");
    return 1;
  }
  if (strcmp(args[2], "dummy") != 0 || fname[strlen(fname) - 2] != '.'
      || fname[strlen(fname) - 1] != 'd') {
    printf("Syntax error: this should be called by make to change the format\nof the generated dependencies!\n");
    return 1;
  }
  
  if (findfirst(args[1], &f, 0) == 1) { /* find out if file exists and get its size */
    printf("File doesn't exist: %s.\n", args[0]);
    return 1;
  }
  if (f.ff_fsize == 0) { /* remove empty makefile */
    if (!remove(fname)) {
      printf("Couldn't remove %s!\n", fname);
      return 1;
    }
    return 0;
  }
  infile = fopen(args[1], "rb");
  if (infile == 0) {
    printf("Can't open %s for reading!\n", args[0]);
    return 1;
  }
  buffer = (char*)xmalloc(f.ff_fsize);
  fread(buffer, sizeof(char), f.ff_fsize, infile);
  fclose(infile);
  i = 0;
  while (buffer[i] != ':' && buffer[i] != '\0') {
    i++;
  }
  if (buffer[i] == '\0') { /* no ':' found! */
    printf("Syntax Error: no ':' found!\n");
    return 1;
  }
  
  outfile = fopen(args[1], "wb");
  fwrite(buffer, sizeof(char), i, outfile); /* write part before ':' ("something.o[ ]") */
  if (buffer[i - 1] != ' ')
    fputc(' ', outfile);
  fwrite(fname, sizeof(char), strlen(fname), outfile); /* insert "something.d" as a target */
  fwrite(buffer + i, sizeof(char), f.ff_fsize - i, outfile); /* write the rest (dependencies) */
  fclose(outfile);
  xfree(buffer);
  return 0;
}

