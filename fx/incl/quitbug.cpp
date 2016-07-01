#include "quitbug.h"
#include <dpmi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

ErrorMessage::ErrorMessage(const char* Message, const char* paramstr, int param_1, int param_2)
{
  strcpy(message, Message);
  strcpy(strparam, paramstr);
  param1 = param_1;
  param2 = param_2;
};

void ErrorMessage::killmesoftly() const
{
  // switch to text mode here
  __dpmi_regs regs;
  regs.x.ax = 0x4F02;
  regs.x.bx = 0x0003;
  __dpmi_int(0x10, &regs);
  fprintf(stderr, "killing me softly with his software...:-(\n>hush..did you hear that ? (Aborted with Error):\n'%s'\nSTRPARAM: %s\nPARAM: %li\n", message, strparam, param1);
  if (param2 != 0xFFFFFFFF)
    fprintf(stderr, "PARAM2: %li\n", param2);
  // get DOS extended error info here ?
  //abort(); this causes another exception to occur (so the actual output will not be readable)
  exit(1);
};

void handleError(const ErrorMessage& msg)
{
  msg.killmesoftly();
};
