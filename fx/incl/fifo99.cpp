//#define TEST_FIFO99

#include "fifo99.h"
#include "quitbug.h"
#include <dpmi.h>
#include <go32.h>


char key_count[128];
extern void keyhandler();
extern int keyhandler__Size;
static __dpmi_paddr bios_handler;

void installkeyhandler()
{
  __dpmi_paddr my_handler;
  if (!KeyHandlerInstalled)
    {
      _go32_dpmi_lock_code(keyhandler, keyhandler__Size);
      _go32_dpmi_lock_data(key_count, 128);
      __dpmi_get_protected_mode_interrupt_vector(0x09, &bios_handler);
      my_handler.selector = _my_cs();
      my_handler.offset32 = (int)keyhandler;
      if (__dpmi_set_protected_mode_interrupt_vector(0x09, &my_handler) != 0)
	handleError(ErrorMessage("Error trying to install irq1 PM-handler (int 0x09)"));
      KeyHandlerInstalled = true;
      clearallkeys();
    }
};

void restorekeyhandler()
{
  if (KeyHandlerInstalled)
    {
      if (__dpmi_set_protected_mode_interrupt_vector(0x09, &bios_handler) != 0)
	handleError(ErrorMessage("Error trying to restore BIOS int 9 (IRQ1) handler."));
      KeyHandlerInstalled = false;
    }
};

int getkey(int scancode)
{
  int ret = key_count[scancode];
  key_count[scancode] = 0;
  return ret;
};

int peekkey(int scancode)
{
  return key_count[scancode];
};

void clearkey(int scancode)
{
  key_count[scancode] = 0;
};

void clearallkeys()
{
  int i;
  for (i = 0; i < 128; i++)
    key_count[i] = 0;
};

bool peekanykey()
{
  int ret = 0, i;
  for (i = 0; i < 128; i++)
    ret += key_count[i];
  return (ret > 0);
};

#ifdef TEST_FIFO99

#include <conio.h>

void printcodes()
{
  int i, j; // six scancodes in one row
  for (i = 0; i < 21; i++)
    {
      j = i * 6;
      printf("%i:%i  %i:%i  %i:%i  %i:%i  %i:%i  %i:%i\n", j + 0, key_count[j + 0], j + 1, key_count[j + 1], j + 2, key_count[j + 2], j + 3, key_count[j + 3], j + 4, key_count[j + 4], j + 5, key_count[j + 5]);
    }
  printf("%i:%i  %i:%i\n", 126, key_count[126], 127, key_count[127]);
};

int main()
{
  installkeyhandler();
  while (!getkey(KEY_ESC))
    {
      clrscr();
      printcodes();
      //      while (!peekanykey()) ;
    }
  restorekeyhandler();
}

#endif
