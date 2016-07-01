#ifndef __QUITBUGGING
#define __QUITBUGGING

class ErrorMessage
{
  char message[1000];
  char strparam[1000];
  unsigned long int param1, param2;
 public:
  void killmesoftly() const;
  ErrorMessage(const char* Message, const char* paramstr = "", int param_1 = 0xFFFFFFFF, int param_2 = 0xFFFFFFFF);
};

void handleError(const ErrorMessage& msg);

#endif
