#include "function.h"
#include <iostream.h>

int main()
{
  Knot top;
  top.Children[0] = 0;
  record_expression("(x^3-2*x^2-3*x+6)/(x^3+2*x^2-4*x-8)", &top);
  char expr_rep[MAX_EXPR_LEN];
  knot2expr(&top, expr_rep);
  cout << "and that's what I thought it is ;-):\n" << expr_rep << endl;
  return 0;
}
