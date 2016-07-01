#include <pc.h>
#include <keys.h>
#include <stdio.h>

int main()
{
  int key;
  do
    {
      key = getkey();
      printf("getkey returned: %i\n", key);
    } while (key != K_Escape);
}

