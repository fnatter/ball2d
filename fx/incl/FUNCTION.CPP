#include "function.h"
#include "quitbug.h"
#include "grtext.h" // appendString
#include <math.h>
#include <stdio.h> // sscanf
#include <ctype.h> // isdigit
#include "lcstdlib.h" // xmalloc/xfree

void export_expression_latex(Knot* base, FILE* texf)
{
  char temp1[MAX_EXPR_LEN], temp2[MAX_EXPR_LEN];
  int i, j;
  if (base->OPtype == CONSTANT_VALUE)
    {
      if (base->value == M_PI)
	{
	  fprintf(texf, "\\pi");
	  return;
	}
      else if (base->value == M_E)
	{
	  fprintf(texf, "e");
	  return;
	}
      else
	{
	  sprintf(temp1, "%G", base->value);
	  i = 0;
	  while (temp1[i] != '\0' && temp1[i] != 'E')
	    i++;
	  if (temp1[i] != '\0')
	    {
	      strcpy(temp2, temp1 + i + 1);
	      temp1[i] = '\0';
	      if (temp2[0] == '+')
		{ // remove obsolete + in "1.0E+2"
		  i = strlen(temp2);
		  j = 0;
		  while (i)
		    {
		      temp2[j] = temp2[j + 1];
		      j++;
		      i--;
		    }
		}
	      fprintf(texf, "%s\\cdot 10^{%s}", temp1, temp2);
	      return;
	    }
	  else  // temp1 = "xx.xx"
	    {
	      fprintf(texf, "%G", base->value);
	      return;
	    }
	}
    }
  if (base->OPtype == TOKEN_VARIABLE_X)
    {
      fprintf(texf, "x");
      return;
    }
  if (base->OPtype == TOKEN_VARIABLE_Y)
    {
      fprintf(texf, "y");
      return;
    }
  // operator isn't "pseudo"
  int thisorder = getOPPrecedence(base->OPtype), suborder1, suborder2;
  suborder1 = getOPPrecedence(base->Children[0]->OPtype);
  if (!isBinary(base->OPtype))
    {
      switch (base->OPtype)
	{
	case OP_SINE:
	  fprintf(texf, "\\sin (");
	  export_expression_latex(base->Children[0], texf);
	  fprintf(texf, ")");
	  break;
	case OP_COSINE:
	  fprintf(texf, "\\cos (");
	  export_expression_latex(base->Children[0], texf);
	  fprintf(texf, ")");
	  break;
	case OP_TANGENT:
	  fprintf(texf, "\\tan (");
	  export_expression_latex(base->Children[0], texf);
	  fprintf(texf, ")");
	  break;
	case OP_COTANGENT:
	  fprintf(texf, "\\cot (");
	  export_expression_latex(base->Children[0], texf);
	  fprintf(texf, ")");
	  break;
	case OP_ARCUS_SINE:
	  fprintf(texf, "\\arcsin (");
	  export_expression_latex(base->Children[0], texf);
	  fprintf(texf, ")");
	  break;
	case OP_ARCUS_COSINE:
	  fprintf(texf, "\\arccos (");
	  export_expression_latex(base->Children[0], texf);
	  fprintf(texf, ")");
	  break;
	case OP_ARCUS_TANGENT:
	  fprintf(texf, "\\arctan (");
	  export_expression_latex(base->Children[0], texf);
	  fprintf(texf, ")");
	  break;
	case OP_ARCUS_COTANGENT:
	  fprintf(texf, "\\arccot (");
	  export_expression_latex(base->Children[0], texf);
	  fprintf(texf, ")");
	  break;
	case OP_NATURAL_LOG:
	  fprintf(texf, "\\ln (");
	  export_expression_latex(base->Children[0], texf);
	  fprintf(texf, ")");
	  break;
	case OP_ABSOLUTE_VALUE:
	  fprintf(texf, "|");
	  export_expression_latex(base->Children[0], texf);
	  fprintf(texf, "|");
	  break;
	case OP_SQUARE_ROOT:
	  fprintf(texf, "\\sqrt{");
	  export_expression_latex(base->Children[0], texf);
	  fprintf(texf, "}");
	  break;
	case OP_SIGN:
	  fprintf(texf, " sgn (");
	  export_expression_latex(base->Children[0], texf);
	  fprintf(texf, ")");
	  break;
	case OP_UNARY_MINUS:
	  fprintf(texf, "-");
	  if (suborder1 <= thisorder)
	    {
	      fprintf(texf, "(");
	      export_expression_latex(base->Children[0], texf);
	      fprintf(texf, ")");
	    }
	  else
	    export_expression_latex(base->Children[0], texf);
	  break;
	default:
	  handleError(ErrorMessage("OPtype not handled!!"));
	}
      return;
    }
  else
    {
      suborder2 = getOPPrecedence(base->Children[1]->OPtype);
      switch (base->OPtype)
	{
	case OP_PLUS:
	  export_expression_latex(base->Children[0], texf);
	  fprintf(texf, "+");
	  export_expression_latex(base->Children[1], texf);
	  break;
	case OP_MINUS:
	  // if sub-level-operator has lower precedence, (or equal and operator is not commutative) set brackets
	  // also set brackets if left argument is negative immediate (like x+(-3)*x)
	  // also set brackets if left argument is a unary minus (like x+(-x)*x)
	  if (suborder1 < thisorder || (suborder1 == thisorder && !isCommutative(base->OPtype))
	      || (base->Children[0]->OPtype == CONSTANT_VALUE && base->Children[0]->value < 0.0) || base->Children[0]->OPtype == OP_UNARY_MINUS)
	    {
	      fprintf(texf, "(");
	      export_expression_latex(base->Children[0], texf);
	      fprintf(texf, ")");
	    }
	  else
	    export_expression_latex(base->Children[0], texf);
	  fprintf(texf, "-");
	  // if sub-level-operator has lower precedence, (or equal and operator is not commutative) set brackets
	  // set brackets if right argument is a negative immediate as well
	  // set brackets if right argument is a (unary) minus as well
	  if (suborder2 < thisorder || (suborder2 == thisorder && !isCommutative(base->OPtype))
	      || (base->Children[1]->OPtype == CONSTANT_VALUE && base->Children[1]->value < 0.0)
	      || base->Children[1]->OPtype == OP_UNARY_MINUS || base->Children[1]->OPtype == OP_MINUS)
	    {
	      fprintf(texf, "(");
	      export_expression_latex(base->Children[1], texf);
	      fprintf(texf, ")");
	    }
	  else
	    export_expression_latex(base->Children[1], texf);
	  break;
	case OP_TIMES:
	  // if sub-level-operator has lower precedence, (or equal and operator is not commutative) set brackets
	  // also set brackets if left argument is negative immediate (like x+(-3)*x)
	  // also set brackets if left argument is a unary minus (like x+(-x)*x)
	  if (suborder1 < thisorder || (suborder1 == thisorder && !isCommutative(base->OPtype))
	      || (base->Children[0]->OPtype == CONSTANT_VALUE && base->Children[0]->value < 0.0) || base->Children[0]->OPtype == OP_UNARY_MINUS)
	    {
	      fprintf(texf, "(");
	      export_expression_latex(base->Children[0], texf);
	      fprintf(texf, ")");
	    }
	  else
	    export_expression_latex(base->Children[0], texf);
	  fprintf(texf, "\\cdot "); //!!!!!!!!
	  // if sub-level-operator has lower precedence, (or equal and operator is not commutative) set brackets
	  // set brackets if right argument is a negative immediate as well
	  // set brackets if right argument is a (unary) minus as well
	  if (suborder2 < thisorder || (suborder2 == thisorder && !isCommutative(base->OPtype))
	      || (base->Children[1]->OPtype == CONSTANT_VALUE && base->Children[1]->value < 0.0)
	      || base->Children[1]->OPtype == OP_UNARY_MINUS || base->Children[1]->OPtype == OP_MINUS)
	    {
	      fprintf(texf, "(");
	      export_expression_latex(base->Children[1], texf);
	      fprintf(texf, ")");
	    }
	  else
	    export_expression_latex(base->Children[1], texf);
	  break;
	case OP_DIVIDE:
	  fprintf(texf, "\\frac{");
	  export_expression_latex(base->Children[0], texf);
	  fprintf(texf, "}{");
	  export_expression_latex(base->Children[1], texf);
	  fprintf(texf, "}");
	  break;
	case OP_LOGARITHM:
	  fprintf(texf, "\\log_{");
	  export_expression_latex(base->Children[0], texf);
	  fprintf(texf, "}");
	  export_expression_latex(base->Children[1], texf);
	  break;
	case OP_POWER:
	  // if sub-level-operator has lower precedence, (or equal and operator is not commutative) set brackets
	  // also set brackets if left argument is negative immediate (like x+(-3)*x)
	  // also set brackets if left argument is a unary minus (like x+(-x)*x)
	  if (suborder1 < thisorder || (suborder1 == thisorder && !isCommutative(base->OPtype))
	      || (base->Children[0]->OPtype == CONSTANT_VALUE && base->Children[0]->value < 0.0) || base->Children[0]->OPtype == OP_UNARY_MINUS)
	    {
	      fprintf(texf, "(");
	      export_expression_latex(base->Children[0], texf);
	      fprintf(texf, ")");
	    }
	  else
	    export_expression_latex(base->Children[0], texf);
	  fprintf(texf, "^{");
	  export_expression_latex(base->Children[1], texf);
	  fprintf(texf, "}");
	  break;
	default:
	  handleError(ErrorMessage("OPtype not handled!!"));
	}
      return;
    }
};


void make_exponent_explicit(char* expr)
{ // converts "xEa" to "x*10^(a)" or "x*10^a"
  char temp[MAX_EXPR_LEN];
  int i;
  i = 0;
  while (expr[i] != '\0' && tolower(expr[i]) != 'e')
    i++;
  if (expr[i] == '\0')
    return;
  if (expr[i + 1] == '+') // + is obsolete, so leave it out
    strcpy(temp, expr + i + 2);
  else
    strcpy(temp, expr + i + 1);
  expr[i] = '\0';
  appendString(expr, "*10^");
  if (temp[0] == '-')
    appendString(expr, "(");
  appendString(expr, temp);
  if (temp[0] == '-')
    appendString(expr, ")");
};

int getOPPrecedence(OperatorType type)
{
  switch (type)
    {
    case OP_POWER: return 5;
    case OP_DIVIDE: return 4;
    case OP_TIMES: return 3;
    case OP_UNARY_MINUS: return 2;
    case OP_MINUS: return 1;
    case OP_PLUS: return 0;
    default: return 100;
    }
};

char getOPchar(OperatorType type)
{
  switch (type)
    {
    case OP_POWER: return '^';
    case OP_UNARY_MINUS: return '-';
    case OP_TIMES: return '*';
    case OP_DIVIDE: return '/';
    case OP_MINUS: return '-';
    case OP_PLUS: return '+';
    default: return '\0';
    }
};

OperatorType getOPtype(char Char)
{
  switch (Char)
    {
    case '^': return OP_POWER;
    case '*': return OP_TIMES;
    case '/': return OP_DIVIDE;
    case '-': return OP_MINUS;
    case '+': return OP_PLUS;
    default: return OP_DONTKNOW;
    }
};

void getmcOPString(OperatorType op, char* str)
{
  switch(op)
    {
    case OP_SINE:
      strcpy(str, "sin");
      return;
    case OP_COSINE:
      strcpy(str, "cos");
      return;
    case OP_TANGENT:
      strcpy(str, "tan");
      return;
    case OP_COTANGENT:
      strcpy(str, "cot");
      return;
    case OP_ARCUS_SINE:
      strcpy(str, "arcsin");
      return;
    case OP_ARCUS_COSINE:
      strcpy(str, "arccos");
      return;
    case OP_ARCUS_TANGENT:
      strcpy(str, "arctan");
      return;
    case OP_ARCUS_COTANGENT:
      strcpy(str, "arccot");
      return;
    case OP_NATURAL_LOG:
      strcpy(str, "ln");
      return;
    case OP_LOGARITHM:
      strcpy(str, "log");
      return;
    case OP_ABSOLUTE_VALUE:
      strcpy(str, "abs");
      return;
    case OP_SIGN:
      strcpy(str, "sgn");
      return;
    case OP_SQUARE_ROOT:
      strcpy(str, "sqrt");
      return;
    default:
      strcpy(str, "NO_MC_OP");
      return;
    }
};

OperatorType getmcOPtype(char* expr, int* len) // mc = multi-character
{
  if (expr[0] == 'l' && expr[1] == 'n')
    {
      *len = 2;
      return OP_NATURAL_LOG;
    }
  if (expr[0] == 'l' && expr[1] == 'o' && expr[2] == 'g')
    {
      *len = 3;
      return OP_LOGARITHM;
    }
  if (expr[0] == 's' && expr[1] == 'i' && expr[2] == 'n')
    {
      *len = 3;
      return OP_SINE;
    }
  if (expr[0] == 'c' && expr[1] == 'o' && expr[2] == 's')
    {
      *len = 3;
      return OP_COSINE;
    }
  if (expr[0] == 't' && expr[1] == 'a' && expr[2] == 'n')
    {
      *len = 3;
      return OP_TANGENT;
    }
  if (expr[0] == 'c' && expr[1] == 'o' && expr[2] == 't')
    {
      *len = 3;
      return OP_COTANGENT;
    }
  if (expr[0] == 'a' && expr[1] == 'r' && expr[2] == 'c' && expr[3] == 's' && expr[4] == 'i' && expr[5] == 'n')
    {
      *len = 6;
      return OP_ARCUS_SINE;
    }
  if (expr[0] == 'a' && expr[1] == 'r' && expr[2] == 'c' && expr[3] == 'c' && expr[4] == 'o' && expr[5] == 's')
    {
      *len = 6;
      return OP_ARCUS_COSINE;
    }
  if (expr[0] == 'a' && expr[1] == 'r' && expr[2] == 'c' && expr[3] == 't' && expr[4] == 'a' && expr[5] == 'n')
    {
      *len = 6;
      return OP_ARCUS_TANGENT;
    }
  if (expr[0] == 'a' && expr[1] == 'r' && expr[2] == 'c' && expr[3] == 'c' && expr[4] == 'o' && expr[5] == 't')
    {
      *len = 6;
      return OP_ARCUS_COTANGENT;
    }
  if (expr[0] == 'a' && expr[1] == 'b' && expr[2] == 's')
    {
      *len = 3;
      return OP_ABSOLUTE_VALUE;
    }
  if (expr[0] == 's' && expr[1] == 'g' && expr[2] == 'n')
    {
      *len = 3;
      return OP_SIGN;
    }
  if (expr[0] == 's' && expr[1] == 'q' && expr[2] == 'r' && expr[3] == 't')
    {
      *len = 4;
      return OP_SQUARE_ROOT;
    }
  *len = 0;
  return OP_DONTKNOW;
};

bool isCommutative(OperatorType type)
{
  switch(type)
    {
    case OP_LOGARITHM:
      return false;
    case OP_PLUS:
      return true;
    case OP_MINUS:
      return false;
    case OP_TIMES:
      return true;
    case OP_POWER:
      return false;
    case OP_DIVIDE:
      return false;
    default:
      return false;
    }
};

bool isBinary(OperatorType type)
{
  if (type == OP_LOGARITHM || (getOPchar(type) != '\0' && type != OP_UNARY_MINUS))
    return true;
  else
    return false;
};

void set_parse_tree_parents(Knot* base)
{
  int i = 0;
  while (base->Children[i] != 0)
    {
      base->Children[i]->Parent = base;
      set_parse_tree_parents(base->Children[i]);
      i++;
    }
};

void copy_expression(Knot* source, Knot* dest)
{
  int i = 0;
  dest->OPtype = source->OPtype;
  dest->value = source->value;
  dest->Parent = source->Parent;
  while (source->Children[i] != 0)
    {
      dest->Children[i] = (Knot*)xmalloc(sizeof(Knot));
      i++;
    }
  dest->Children[i] = 0;
  i = 0;
  while (source->Children[i] != 0)
    {
      copy_expression(source->Children[i], dest->Children[i]);
      i++;
    }
};

bool depends_on_x(Knot* cknot)
{
  int i = 0;
  if (cknot->OPtype == TOKEN_VARIABLE_X)
    return true;
  while (cknot->Children[i] != 0)
    {
      if (depends_on_x(cknot->Children[i]))
	return true;
      i++;
    }
  return false;
};

bool depends_on_x_or_y(Knot* cknot)
{
  int i = 0;
  if (cknot->OPtype == TOKEN_VARIABLE_X || cknot->OPtype == TOKEN_VARIABLE_Y)
    return true;
  while (cknot->Children[i] != 0)
    {
      if (depends_on_x_or_y(cknot->Children[i]))
	return true;
      i++;
    }
  return false;
};

void derive_expression(Knot* base, Knot* derivative_knot)
{
  Knot temp;
  if (base->OPtype == CONSTANT_VALUE || base->OPtype == TOKEN_VARIABLE_Y)
    {
      derivative_knot->Children[0] = 0;
      derivative_knot->OPtype = CONSTANT_VALUE;
      derivative_knot->value = 0.0;
      return;
    }
  if (base->OPtype == TOKEN_VARIABLE_X)
    {
      derivative_knot->Children[0] = 0;
      derivative_knot->OPtype = CONSTANT_VALUE;
      derivative_knot->value = 1.0;
      return;
    }
  if (isBinary(base->OPtype))
    { // binary operator
      derivative_knot->Children[0] = (Knot*)xmalloc(sizeof(Knot));
      derivative_knot->Children[1] = (Knot*)xmalloc(sizeof(Knot));
      derivative_knot->Children[2] = 0;
      switch(base->OPtype)
	{
	case OP_PLUS:
	case OP_MINUS:
	  derivative_knot->OPtype = base->OPtype;
	  derive_expression(base->Children[0], derivative_knot->Children[0]);
	  derive_expression(base->Children[1], derivative_knot->Children[1]);
	  return;
	case OP_TIMES:
	  if (depends_on_x(base->Children[0]))
	    {
	      if (depends_on_x(base->Children[1]))
		{// (u(x)*v(x))' = u'(x)*v(x) + u(x)*v'(x)
		  derivative_knot->OPtype = OP_PLUS;
		  derivative_knot->Children[0]->OPtype = OP_TIMES;
		  derivative_knot->Children[0]->Children[0] = (Knot*)xmalloc(sizeof(Knot));
		  derivative_knot->Children[0]->Children[1] = (Knot*)xmalloc(sizeof(Knot));
		  derivative_knot->Children[0]->Children[2] = 0;
		  derive_expression(base->Children[0], derivative_knot->Children[0]->Children[0]);
		  copy_expression(base->Children[1], derivative_knot->Children[0]->Children[1]);
		  derivative_knot->Children[1]->OPtype = OP_TIMES;
		  derivative_knot->Children[1]->Children[0] = (Knot*)xmalloc(sizeof(Knot));
		  derivative_knot->Children[1]->Children[1] = (Knot*)xmalloc(sizeof(Knot));
		  derivative_knot->Children[1]->Children[2] = 0;
		  copy_expression(base->Children[0], derivative_knot->Children[1]->Children[0]);
		  derive_expression(base->Children[1], derivative_knot->Children[1]->Children[1]);
		}
	      else// [u(x)*v]' = v*u'(x)
		{
		  derivative_knot->OPtype = OP_TIMES;
		  derivative_knot->Children[0]->OPtype = CONSTANT_VALUE;
		  derivative_knot->Children[0]->value = evaluate_expression(base->Children[1]);
		  derivative_knot->Children[0]->Children[0] = 0;
		  derive_expression(base->Children[0], derivative_knot->Children[1]);
		}
	    }
	  else
	    {
	      if (!depends_on_x(base->Children[1]))
		{//[u*v]' = u*v
		  xfree(derivative_knot->Children[0]);
		  xfree(derivative_knot->Children[1]);
		  derivative_knot->Children[0] = 0;
		  derivative_knot->OPtype = CONSTANT_VALUE;
		  derivative_knot->value = 0.0;
		}
	      else //[u*v(x)]'=u*v'(x)
		{
		  derivative_knot->OPtype = OP_TIMES;
		  derivative_knot->Children[0]->OPtype = CONSTANT_VALUE;
		  derivative_knot->Children[0]->value = evaluate_expression(base->Children[0]);
		  derivative_knot->Children[0]->Children[0] = 0;
		  derive_expression(base->Children[1], derivative_knot->Children[1]);
		}
	    }
	  return;
	case OP_DIVIDE:
	  if (depends_on_x(base->Children[0]))
	    {
	      if (!depends_on_x(base->Children[1]))
		{// [u(x)/v]' = 1/v*u'(x)
		  derivative_knot->OPtype = OP_TIMES;
		  derivative_knot->Children[0]->OPtype = CONSTANT_VALUE;
		  derivative_knot->Children[0]->value = 1.0 / evaluate_expression(base->Children[1]);
		  derive_expression(base->Children[0], derivative_knot->Children[1]);
		}
	      // (u(x)/v(x))' = (u'(x)*v(x) - u(x)*v'(x)) / v(x)^2
	      derivative_knot->OPtype = OP_DIVIDE;
	      derivative_knot->Children[0]->OPtype = OP_MINUS;
	      derivative_knot->Children[0]->Children[0] = (Knot*)xmalloc(sizeof(Knot));
	      derivative_knot->Children[0]->Children[1] = (Knot*)xmalloc(sizeof(Knot));
	      derivative_knot->Children[0]->Children[2] = 0;
	      derivative_knot->Children[0]->Children[0]->OPtype = OP_TIMES;
	      derivative_knot->Children[0]->Children[0]->Children[0] = (Knot*)xmalloc(sizeof(Knot));
	      derivative_knot->Children[0]->Children[0]->Children[1] = (Knot*)xmalloc(sizeof(Knot));
	      derivative_knot->Children[0]->Children[0]->Children[2] = 0;
	      derive_expression(base->Children[0], derivative_knot->Children[0]->Children[0]->Children[0]);
	      copy_expression(base->Children[1], derivative_knot->Children[0]->Children[0]->Children[1]);
	      derivative_knot->Children[0]->Children[1]->OPtype = OP_TIMES;
	      derivative_knot->Children[0]->Children[1]->Children[0] = (Knot*)xmalloc(sizeof(Knot));
	      derivative_knot->Children[0]->Children[1]->Children[1] = (Knot*)xmalloc(sizeof(Knot));
	      derivative_knot->Children[0]->Children[1]->Children[2] = 0;
	      copy_expression(base->Children[0], derivative_knot->Children[0]->Children[1]->Children[0]);
	      derive_expression(base->Children[1], derivative_knot->Children[0]->Children[1]->Children[1]);
	      derivative_knot->Children[1]->OPtype = OP_POWER;
	      derivative_knot->Children[1]->Children[0] = (Knot*)xmalloc(sizeof(Knot));
	      derivative_knot->Children[1]->Children[1] = (Knot*)xmalloc(sizeof(Knot));
	      derivative_knot->Children[1]->Children[2] = 0;
	      copy_expression(base->Children[1], derivative_knot->Children[1]->Children[0]);
	      derivative_knot->Children[1]->Children[1]->OPtype = CONSTANT_VALUE;
	      derivative_knot->Children[1]->Children[1]->value = 2.0;
	      derivative_knot->Children[1]->Children[1]->Children[0] = 0;
	    }
	  else // base->Children[0] does not depend on x
	    {
	      if (!depends_on_x(base->Children[1]))
		{//(u/v)' = u*v
		  xfree(derivative_knot->Children[0]);
		  xfree(derivative_knot->Children[1]);
		  derivative_knot->Children[0] = 0;
		  derivative_knot->OPtype = CONSTANT_VALUE;
		  derivative_knot->value = 0.0;
		}
	      else // [u/v(x)]' = -u * v'(x)/v(x)^2
		{
		  derivative_knot->OPtype = OP_TIMES;
		  derivative_knot->Children[0]->Children[0] = 0;
		  derivative_knot->Children[0]->OPtype = CONSTANT_VALUE;
		  derivative_knot->Children[0]->value = -evaluate_expression(base->Children[0]);
		  derivative_knot->Children[1]->OPtype = OP_DIVIDE;
		  derivative_knot->Children[1]->Children[0] = (Knot*)xmalloc(sizeof(Knot));
		  derivative_knot->Children[1]->Children[1] = (Knot*)xmalloc(sizeof(Knot));
		  derivative_knot->Children[1]->Children[2] = 0;
		  derive_expression(base->Children[1], derivative_knot->Children[1]->Children[0]);
		  derivative_knot->Children[1]->Children[1]->OPtype = OP_POWER;
		  derivative_knot->Children[1]->Children[1]->Children[0] = (Knot*)xmalloc(sizeof(Knot));
		  derivative_knot->Children[1]->Children[1]->Children[1] = (Knot*)xmalloc(sizeof(Knot));
		  derivative_knot->Children[1]->Children[1]->Children[2] = 0;
		  copy_expression(base->Children[1], derivative_knot->Children[1]->Children[1]->Children[0]);
		  derivative_knot->Children[1]->Children[1]->Children[1]->OPtype = CONSTANT_VALUE;
		  derivative_knot->Children[1]->Children[1]->Children[1]->value = 2.0;
		  derivative_knot->Children[1]->Children[1]->Children[1]->Children[0] = 0;
		}
	    }
	  return;
	case OP_POWER:
	  if (depends_on_x(base->Children[0]))
	    {
	      if (depends_on_x(base->Children[1]))
		{// [u(x)^v(x)]' = u(x)^v(x)*[v'(x)*ln(u(x))+v(x)/u(x)] (logarithmic derivative)
		  derivative_knot->OPtype = OP_TIMES;
		  derivative_knot->Children[1] = (Knot*)xmalloc(sizeof(Knot));
		  derivative_knot->Children[2] = 0;
		  copy_expression(base, derivative_knot->Children[0]);
		  derivative_knot->Children[1]->OPtype = OP_PLUS;
		  derivative_knot->Children[1]->Children[0] = (Knot*)xmalloc(sizeof(Knot));
		  derivative_knot->Children[1]->Children[1] = (Knot*)xmalloc(sizeof(Knot));
		  derivative_knot->Children[1]->Children[2] = 0;
		  derivative_knot->Children[1]->Children[0]->OPtype = OP_TIMES;
		  derivative_knot->Children[1]->Children[0]->Children[0] = (Knot*)xmalloc(sizeof(Knot));
		  derivative_knot->Children[1]->Children[0]->Children[1] = (Knot*)xmalloc(sizeof(Knot));
		  derivative_knot->Children[1]->Children[0]->Children[2] = 0;
		  derive_expression(base->Children[1], derivative_knot->Children[1]->Children[0]->Children[0]);
		  derivative_knot->Children[1]->Children[0]->Children[1]->OPtype = OP_NATURAL_LOG;
		  derivative_knot->Children[1]->Children[0]->Children[1]->Children[0] = (Knot*)xmalloc(sizeof(Knot));
		  derivative_knot->Children[1]->Children[0]->Children[1]->Children[1] = 0;
		  copy_expression(base->Children[0], derivative_knot->Children[1]->Children[0]->Children[1]->Children[0]);
		  derivative_knot->Children[1]->Children[1]->OPtype = OP_DIVIDE;
		  derivative_knot->Children[1]->Children[1]->Children[0] = (Knot*)xmalloc(sizeof(Knot));
		  derivative_knot->Children[1]->Children[1]->Children[1] = (Knot*)xmalloc(sizeof(Knot));
		  derivative_knot->Children[1]->Children[1]->Children[2] = 0;
		  copy_expression(base->Children[1], derivative_knot->Children[1]->Children[1]->Children[0]);
		  copy_expression(base->Children[0], derivative_knot->Children[1]->Children[1]->Children[1]);
		  return;
		}// (u(x)^a)' = a*u(x)^(a-1) * u'(x)
	      derivative_knot->OPtype = OP_TIMES;
	      derivative_knot->Children[0]->OPtype = CONSTANT_VALUE;
	      derivative_knot->Children[0]->value = evaluate_expression(base->Children[1]);
	      derivative_knot->Children[0]->Children[0] = 0;
	      derivative_knot->Children[1]->OPtype = OP_TIMES;
	      derivative_knot->Children[1]->Children[0] = (Knot*)xmalloc(sizeof(Knot));
	      derivative_knot->Children[1]->Children[1] = (Knot*)xmalloc(sizeof(Knot));
	      derivative_knot->Children[1]->Children[2] = 0;
	      derivative_knot->Children[1]->Children[0]->OPtype = OP_POWER;
	      derivative_knot->Children[1]->Children[0]->Children[0] = (Knot*)xmalloc(sizeof(Knot));
	      derivative_knot->Children[1]->Children[0]->Children[1] = (Knot*)xmalloc(sizeof(Knot));
	      derivative_knot->Children[1]->Children[0]->Children[2] = 0;
	      copy_expression(base->Children[0], derivative_knot->Children[1]->Children[0]->Children[0]);
	      derivative_knot->Children[1]->Children[0]->Children[1]->OPtype = CONSTANT_VALUE;
	      derivative_knot->Children[1]->Children[0]->Children[1]->value = derivative_knot->Children[0]->value - 1.0;
	      derivative_knot->Children[1]->Children[0]->Children[1]->Children[0] = 0;
	      derive_expression(base->Children[0], derivative_knot->Children[1]->Children[1]);
	    }
	  else  // (a^u(x))' = ln(a)*a^u(x)*u'(x)
	    {
	      if (!depends_on_x(base->Children[1]))
		{
		}
	      derivative_knot->OPtype = OP_TIMES;
	      derivative_knot->Children[0]->OPtype = OP_TIMES;
	      derive_expression(base->Children[1], derivative_knot->Children[1]);
	      derivative_knot->Children[0]->Children[0] = (Knot*)xmalloc(sizeof(Knot));
	      derivative_knot->Children[0]->Children[1] = (Knot*)xmalloc(sizeof(Knot));
	      derivative_knot->Children[0]->Children[2] = 0;
	      derivative_knot->Children[0]->Children[0]->OPtype = CONSTANT_VALUE;
	      derivative_knot->Children[0]->Children[0]->value = log(evaluate_expression(base->Children[0]));
	      derivative_knot->Children[0]->Children[0]->Children[0] = 0;
	      derivative_knot->Children[0]->Children[1]->OPtype = OP_POWER;
	      derivative_knot->Children[0]->Children[1]->Children[0] = (Knot*)xmalloc(sizeof(Knot));
	      derivative_knot->Children[0]->Children[1]->Children[1] = (Knot*)xmalloc(sizeof(Knot));
	      derivative_knot->Children[0]->Children[1]->Children[2] = 0;
	      derivative_knot->Children[0]->Children[1]->Children[0]->OPtype = CONSTANT_VALUE;
	      derivative_knot->Children[0]->Children[1]->Children[0]->value = evaluate_expression(base->Children[0]);
	      derivative_knot->Children[0]->Children[1]->Children[0]->Children[0] = 0;
	      copy_expression(base->Children[1], derivative_knot->Children[0]->Children[1]->Children[1]);
	    }
	  return;
	case OP_LOGARITHM:
	  temp.OPtype = OP_DIVIDE;
	  temp.Children[0] = (Knot*)xmalloc(sizeof(Knot));
	  temp.Children[1] = (Knot*)xmalloc(sizeof(Knot));
	  temp.Children[2] = 0;
	  temp.Children[0]->OPtype = OP_NATURAL_LOG;
	  temp.Children[0]->Children[0] = (Knot*)xmalloc(sizeof(Knot));
	  temp.Children[0]->Children[1] = 0;
	  copy_expression(base->Children[1], temp.Children[0]->Children[0]);
	  temp.Children[1]->OPtype = OP_NATURAL_LOG;
	  temp.Children[1]->Children[0] = (Knot*)xmalloc(sizeof(Knot));
	  temp.Children[1]->Children[1] = 0;
	  copy_expression(base->Children[0], temp.Children[1]->Children[0]);
	  derive_expression(&temp, derivative_knot);
	  freeKnot(&temp);
	  return;
	default:
	  derive_Error = DERIVE_FUNCTION_NOT_IMPLEMENTED;
	  derivative_knot->Children[0] = 0;
	  return;
	}
    }
  else // unary operator
    {
      derivative_knot->Children[0] = (Knot*)xmalloc(sizeof(Knot));
      switch (base->OPtype)
	{
	case OP_UNARY_MINUS:// [-u(x)]' =-u'(x)
	  derivative_knot->Children[1] = 0;
	  derive_expression(base->Children[0], derivative_knot->Children[0]);
	  derivative_knot->OPtype = OP_UNARY_MINUS;
	  return;
	case OP_NATURAL_LOG:// [ln(u(x))]' = u'(x) / u(x)
	  derivative_knot->OPtype = OP_DIVIDE;
	  derivative_knot->Children[1] = (Knot*)xmalloc(sizeof(Knot));
	  derivative_knot->Children[2] = 0;
	  derive_expression(base->Children[0], derivative_knot->Children[0]);
	  copy_expression(base->Children[0], derivative_knot->Children[1]);
	  return;
	case OP_SINE:// [sin(u(x))]' = cos(u(x))*u'(x)
	  derivative_knot->OPtype = OP_TIMES;
	  derivative_knot->Children[1] = (Knot*)xmalloc(sizeof(Knot));
	  derivative_knot->Children[2] = 0;
	  derivative_knot->Children[0]->OPtype = OP_COSINE;
	  derivative_knot->Children[0]->Children[0] = (Knot*)xmalloc(sizeof(Knot));
	  derivative_knot->Children[0]->Children[1] = 0;
	  copy_expression(base->Children[0], derivative_knot->Children[0]->Children[0]);
	  derive_expression(base->Children[0], derivative_knot->Children[1]);
	  return;
	case OP_COSINE://[cos(u(x))]' = -sin(x)*u'(x)
	  derivative_knot->OPtype = OP_TIMES;
	  derivative_knot->Children[1] = (Knot*)xmalloc(sizeof(Knot));
	  derivative_knot->Children[2] = 0;
	  derivative_knot->Children[1]->OPtype = OP_UNARY_MINUS;
	  derivative_knot->Children[1]->Children[0] = (Knot*)xmalloc(sizeof(Knot));
	  derivative_knot->Children[1]->Children[1] = 0;
	  derivative_knot->Children[1]->Children[0]->OPtype = OP_SINE;
	  derivative_knot->Children[1]->Children[0]->Children[0] = (Knot*)xmalloc(sizeof(Knot));
	  derivative_knot->Children[1]->Children[0]->Children[1] = 0;
	  copy_expression(base->Children[0], derivative_knot->Children[1]->Children[0]->Children[0]);
	  derive_expression(base->Children[0], derivative_knot->Children[0]);
	  return;
	case OP_TANGENT:// [tan(u(x))]' = u'(x)*[cos(u(x))]^(-2)
	  derivative_knot->OPtype = OP_TIMES;
	  derivative_knot->Children[1] = (Knot*)xmalloc(sizeof(Knot));
	  derivative_knot->Children[2] = 0;
	  derive_expression(base->Children[0], derivative_knot->Children[0]);
	  derivative_knot->Children[1]->OPtype = OP_POWER;
	  derivative_knot->Children[1]->Children[0] = (Knot*)xmalloc(sizeof(Knot));
	  derivative_knot->Children[1]->Children[1] = (Knot*)xmalloc(sizeof(Knot));
	  derivative_knot->Children[1]->Children[2] = 0;
	  derivative_knot->Children[1]->Children[0]->OPtype = OP_COSINE;
	  derivative_knot->Children[1]->Children[0]->Children[0] = (Knot*)xmalloc(sizeof(Knot));
	  derivative_knot->Children[1]->Children[0]->Children[1] = 0;
	  copy_expression(base->Children[0], derivative_knot->Children[1]->Children[0]->Children[0]);
	  derivative_knot->Children[1]->Children[1]->OPtype = CONSTANT_VALUE;
	  derivative_knot->Children[1]->Children[1]->value = -2.0;
	  derivative_knot->Children[1]->Children[1]->Children[0] = 0;
	  return;
	case OP_COTANGENT:// [cot(u(x))]' = -u'(x)*[sin(u(x))]^(-2)
	  derivative_knot->OPtype = OP_TIMES;
	  derivative_knot->Children[1] = (Knot*)xmalloc(sizeof(Knot));
	  derivative_knot->Children[2] = 0;
	  derivative_knot->Children[0]->OPtype = OP_UNARY_MINUS;
	  derivative_knot->Children[0]->Children[0] = (Knot*)xmalloc(sizeof(Knot));
	  derivative_knot->Children[0]->Children[1] = 0;
	  derive_expression(base->Children[0], derivative_knot->Children[0]->Children[0]);
	  derivative_knot->Children[1]->OPtype = OP_POWER;
	  derivative_knot->Children[1]->Children[0] = (Knot*)xmalloc(sizeof(Knot));
	  derivative_knot->Children[1]->Children[1] = (Knot*)xmalloc(sizeof(Knot));
	  derivative_knot->Children[1]->Children[2] = 0;
	  derivative_knot->Children[1]->Children[0]->OPtype = OP_SINE;
	  derivative_knot->Children[1]->Children[0]->Children[0] = (Knot*)xmalloc(sizeof(Knot));
	  derivative_knot->Children[1]->Children[0]->Children[1] = 0;
	  copy_expression(base->Children[0], derivative_knot->Children[1]->Children[0]->Children[0]);
	  derivative_knot->Children[1]->Children[1]->OPtype = CONSTANT_VALUE;
	  derivative_knot->Children[1]->Children[1]->value = -2.0;
	  derivative_knot->Children[1]->Children[1]->Children[0] = 0;
	  return;
	case OP_ABSOLUTE_VALUE: // [abs(u(x))]' = sgn(u(x))*u'(x)
	  derivative_knot->OPtype = OP_TIMES;
	  derivative_knot->Children[1] = (Knot*)xmalloc(sizeof(Knot));
	  derivative_knot->Children[2] = 0;
	  derive_expression(base->Children[0], derivative_knot->Children[0]);
	  derivative_knot->Children[1]->OPtype = OP_SIGN;
	  derivative_knot->Children[1]->Children[0] = (Knot*)xmalloc(sizeof(Knot));
	  derivative_knot->Children[1]->Children[1] = 0;
	  copy_expression(base->Children[0], derivative_knot->Children[1]->Children[0]);
	  return;
	case OP_SIGN://[sgn(u(x))]' = 0
	  xfree(derivative_knot->Children[0]);
	  derivative_knot->Children[0] = 0;
	  derivative_knot->OPtype = CONSTANT_VALUE;
	  derivative_knot->value = 0.0;
	  return;
	case OP_SQUARE_ROOT:// [sqrt(u(x))]' = u'(x)*0.5*u(x)^(-0.5)
	  derivative_knot->OPtype = OP_TIMES;
	  derivative_knot->Children[1] = (Knot*)xmalloc(sizeof(Knot));
	  derivative_knot->Children[2] = 0;
	  derive_expression(base->Children[0], derivative_knot->Children[1]);
	  derivative_knot->Children[0]->OPtype = OP_TIMES;
	  derivative_knot->Children[0]->Children[0] = (Knot*)xmalloc(sizeof(Knot));
	  derivative_knot->Children[0]->Children[1] = (Knot*)xmalloc(sizeof(Knot));
	  derivative_knot->Children[0]->Children[2] = 0;
	  derivative_knot->Children[0]->Children[0]->OPtype = CONSTANT_VALUE;
	  derivative_knot->Children[0]->Children[0]->value = 0.5;
	  derivative_knot->Children[0]->Children[0]->Children[0] = 0;
	  derivative_knot->Children[0]->Children[1]->OPtype = OP_POWER;
	  derivative_knot->Children[0]->Children[1]->Children[0] = (Knot*)xmalloc(sizeof(Knot));
	  derivative_knot->Children[0]->Children[1]->Children[1] = (Knot*)xmalloc(sizeof(Knot));
	  derivative_knot->Children[0]->Children[1]->Children[2] = 0;
	  copy_expression(base->Children[0], derivative_knot->Children[0]->Children[1]->Children[0]);
	  derivative_knot->Children[0]->Children[1]->Children[1]->OPtype = CONSTANT_VALUE;
	  derivative_knot->Children[0]->Children[1]->Children[1]->value = -0.5;
	  derivative_knot->Children[0]->Children[1]->Children[1]->Children[0] = 0;
	  return;
	case OP_ARCUS_SINE:// [arcsin(u(x))]' = u'(x)/sqrt(1 - u(x)^2)
	  derivative_knot->OPtype = OP_DIVIDE;
	  derivative_knot->Children[1] = (Knot*)xmalloc(sizeof(Knot));
	  derivative_knot->Children[2] = 0;
	  derive_expression(base->Children[0], derivative_knot->Children[0]);
	  derivative_knot->Children[1]->OPtype = OP_SQUARE_ROOT;
	  derivative_knot->Children[1]->Children[0] = (Knot*)xmalloc(sizeof(Knot));
	  derivative_knot->Children[1]->Children[1] = 0;
	  derivative_knot->Children[1]->Children[0]->OPtype = OP_MINUS;
	  derivative_knot->Children[1]->Children[0]->Children[0] = (Knot*)xmalloc(sizeof(Knot));
	  derivative_knot->Children[1]->Children[0]->Children[1] = (Knot*)xmalloc(sizeof(Knot));
	  derivative_knot->Children[1]->Children[0]->Children[2] = 0;
	  derivative_knot->Children[1]->Children[0]->Children[0]->OPtype = CONSTANT_VALUE;
	  derivative_knot->Children[1]->Children[0]->Children[0]->value = 1.0;
	  derivative_knot->Children[1]->Children[0]->Children[0]->Children[0] = 0;
	  derivative_knot->Children[1]->Children[0]->Children[1]->OPtype = OP_POWER;
	  derivative_knot->Children[1]->Children[0]->Children[1]->Children[0] = (Knot*)xmalloc(sizeof(Knot));
	  derivative_knot->Children[1]->Children[0]->Children[1]->Children[1] = (Knot*)xmalloc(sizeof(Knot));
	  derivative_knot->Children[1]->Children[0]->Children[1]->Children[2] = 0;
	  copy_expression(base->Children[0], derivative_knot->Children[1]->Children[0]->Children[1]->Children[0]);
	  derivative_knot->Children[1]->Children[0]->Children[1]->Children[1]->OPtype = CONSTANT_VALUE;
	  derivative_knot->Children[1]->Children[0]->Children[1]->Children[1]->value = 2.0;
	  derivative_knot->Children[1]->Children[0]->Children[1]->Children[1]->Children[0] = 0;
	  return;
	case OP_ARCUS_COSINE:// [arccos(u(x))]' = -u'(x)/sqrt(1 - u(x)^2)
	  derivative_knot->OPtype = OP_DIVIDE;
	  derivative_knot->Children[1] = (Knot*)xmalloc(sizeof(Knot));
	  derivative_knot->Children[2] = 0;
	  derivative_knot->Children[0]->OPtype = OP_UNARY_MINUS;
	  derivative_knot->Children[0]->Children[0] = (Knot*)xmalloc(sizeof(Knot));
	  derivative_knot->Children[0]->Children[1] = 0;
	  derive_expression(base->Children[0], derivative_knot->Children[0]->Children[0]);
	  derivative_knot->Children[1]->OPtype = OP_SQUARE_ROOT;
	  derivative_knot->Children[1]->Children[0] = (Knot*)xmalloc(sizeof(Knot));
	  derivative_knot->Children[1]->Children[1] = 0;
	  derivative_knot->Children[1]->Children[0]->OPtype = OP_MINUS;
	  derivative_knot->Children[1]->Children[0]->Children[0] = (Knot*)xmalloc(sizeof(Knot));
	  derivative_knot->Children[1]->Children[0]->Children[1] = (Knot*)xmalloc(sizeof(Knot));
	  derivative_knot->Children[1]->Children[0]->Children[2] = 0;
	  derivative_knot->Children[1]->Children[0]->Children[0]->OPtype = CONSTANT_VALUE;
	  derivative_knot->Children[1]->Children[0]->Children[0]->value = 1.0;
	  derivative_knot->Children[1]->Children[0]->Children[0]->Children[0] = 0;
	  derivative_knot->Children[1]->Children[0]->Children[1]->OPtype = OP_POWER;
	  derivative_knot->Children[1]->Children[0]->Children[1]->Children[0] = (Knot*)xmalloc(sizeof(Knot));
	  derivative_knot->Children[1]->Children[0]->Children[1]->Children[1] = (Knot*)xmalloc(sizeof(Knot));
	  derivative_knot->Children[1]->Children[0]->Children[1]->Children[2] = 0;
	  copy_expression(base->Children[0], derivative_knot->Children[1]->Children[0]->Children[1]->Children[0]);
	  derivative_knot->Children[1]->Children[0]->Children[1]->Children[1]->OPtype = CONSTANT_VALUE;
	  derivative_knot->Children[1]->Children[0]->Children[1]->Children[1]->value = 2.0;
	  derivative_knot->Children[1]->Children[0]->Children[1]->Children[1]->Children[0] = 0;
	  return;
	case OP_ARCUS_TANGENT:// [arctan(u(x))]' = u'(x)/(1+u(x)^2)
	  derivative_knot->OPtype = OP_DIVIDE;
	  derivative_knot->Children[1] = (Knot*)xmalloc(sizeof(Knot));
	  derivative_knot->Children[2] = 0;
	  derive_expression(base->Children[0], derivative_knot->Children[0]);
	  derivative_knot->Children[1]->OPtype = OP_PLUS;
	  derivative_knot->Children[1]->Children[0] = (Knot*)xmalloc(sizeof(Knot));
	  derivative_knot->Children[1]->Children[1] = (Knot*)xmalloc(sizeof(Knot));
	  derivative_knot->Children[1]->Children[2] = 0;
	  derivative_knot->Children[1]->Children[0]->OPtype = CONSTANT_VALUE;
	  derivative_knot->Children[1]->Children[0]->value = 1.0;
	  derivative_knot->Children[1]->Children[0]->Children[0] = 0;
	  derivative_knot->Children[1]->Children[1]->OPtype = OP_POWER;
	  derivative_knot->Children[1]->Children[1]->Children[0] = (Knot*)xmalloc(sizeof(Knot));
	  derivative_knot->Children[1]->Children[1]->Children[1] = (Knot*)xmalloc(sizeof(Knot));
	  derivative_knot->Children[1]->Children[1]->Children[2] = 0;
	  copy_expression(base->Children[0], derivative_knot->Children[1]->Children[1]->Children[0]);
	  derivative_knot->Children[1]->Children[1]->Children[1]->OPtype = CONSTANT_VALUE;
	  derivative_knot->Children[1]->Children[1]->Children[1]->value = 2.0;
	  derivative_knot->Children[1]->Children[1]->Children[1]->Children[0] = 0;
	  return;
	case OP_ARCUS_COTANGENT:// [arccot(u(x))]' = -u'(x)/(1+u(x)^2)
	  derivative_knot->OPtype = OP_DIVIDE;
	  derivative_knot->Children[1] = (Knot*)xmalloc(sizeof(Knot));
	  derivative_knot->Children[2] = 0;
	  derivative_knot->Children[0]->OPtype = OP_UNARY_MINUS;
	  derivative_knot->Children[0]->Children[0] = (Knot*)xmalloc(sizeof(Knot));
	  derivative_knot->Children[0]->Children[1] = 0;
	  derive_expression(base->Children[0], derivative_knot->Children[0]->Children[0]);
	  derivative_knot->Children[1]->OPtype = OP_PLUS;
	  derivative_knot->Children[1]->Children[0] = (Knot*)xmalloc(sizeof(Knot));
	  derivative_knot->Children[1]->Children[1] = (Knot*)xmalloc(sizeof(Knot));
	  derivative_knot->Children[1]->Children[2] = 0;
	  derivative_knot->Children[1]->Children[0]->OPtype = CONSTANT_VALUE;
	  derivative_knot->Children[1]->Children[0]->value = 1.0;
	  derivative_knot->Children[1]->Children[0]->Children[0] = 0;
	  derivative_knot->Children[1]->Children[1]->OPtype = OP_POWER;
	  derivative_knot->Children[1]->Children[1]->Children[0] = (Knot*)xmalloc(sizeof(Knot));
	  derivative_knot->Children[1]->Children[1]->Children[1] = (Knot*)xmalloc(sizeof(Knot));
	  derivative_knot->Children[1]->Children[1]->Children[2] = 0;
	  copy_expression(base->Children[0], derivative_knot->Children[1]->Children[1]->Children[0]);
	  derivative_knot->Children[1]->Children[1]->Children[1]->OPtype = CONSTANT_VALUE;
	  derivative_knot->Children[1]->Children[1]->Children[1]->value = 2.0;
	  derivative_knot->Children[1]->Children[1]->Children[1]->Children[0] = 0;
	  return;
	default:
	  derive_Error = DERIVE_FUNCTION_NOT_IMPLEMENTED;
	  derivative_knot->Children[0] = 0;
	  return;
	}
    }
};

void simplify_expression(Knot* base)
{
  while (simplify_expr(base))
    ;
};

bool simplify_expr(Knot* cknot)
{// note that functions like "ln(0.0)" could bring this to crash!
  Knot temp;
  double value;
  if (cknot->OPtype == CONSTANT_VALUE || cknot->OPtype == TOKEN_VARIABLE_X || cknot->OPtype == TOKEN_VARIABLE_Y)
    return false;
  if (!depends_on_x_or_y(cknot))
    {
      value = evaluate_expression(cknot);
      freeKnot(cknot);
      cknot->OPtype = CONSTANT_VALUE;
      cknot->value = value;
      cknot->Children[0] = 0;
      return true;
    }
  if (isBinary(cknot->OPtype))
    {
      if (cknot->Children[0]->OPtype == CONSTANT_VALUE &&
          cknot->Children[0]->value == 0.0)
	{// left-hand argument is 0.0
	  switch(cknot->OPtype)
	    {
	    case OP_TIMES: // E = 0.0*u(x) = 0.0
	    case OP_DIVIDE:// E = 0.0/u(x) = 0.0
	      //                  case OP_POWER: // E = 0.0^u(x) = 0.0 (ONLY if u(x) >= 0)
	      freeKnot(cknot);
	      cknot->Children[0] = 0;
	      cknot->OPtype = CONSTANT_VALUE;
	      cknot->value = 0.0;
	      break;
	    case OP_PLUS:  // E = 0.0 + u(x) = u(x)
	      temp = *cknot;
	      copy_expression(temp.Children[1], cknot);
	      cknot->Parent = temp.Parent;
	      freeKnot(&temp);
	      break;
	    case OP_MINUS: // E = 0.0 - u(x) = -u(x)
	      temp = *cknot;
	      cknot->OPtype = OP_UNARY_MINUS;
	      cknot->Children[0] = (Knot*)xmalloc(sizeof(Knot));
	      cknot->Children[1] = 0;
	      cknot->Parent = temp.Parent;
	      copy_expression(temp.Children[1], cknot->Children[0]);
	      freeKnot(&temp);
	      break;
	    default:
	      goto next1;
	    }
	  return true;
	}
    next1:
      if (cknot->Children[1]->OPtype == CONSTANT_VALUE &&
          cknot->Children[1]->value == 0.0)
	{// right-hand argument is 0.0
	  switch(cknot->OPtype)
	    {// Error with OP_DIVIDE should be handled
	    case OP_TIMES: // E = u(x) * 0.0 = 0.0
	      freeKnot(cknot);
	      cknot->Children[0] = 0;
	      cknot->OPtype = CONSTANT_VALUE;
	      cknot->value = 0.0;
	      break;
	    case OP_POWER: // E = u(x)^0.0 = 1.0
	      freeKnot(cknot); // throw away remaining expression
	      cknot->Children[0] = 0;
	      cknot->OPtype = CONSTANT_VALUE;
	      cknot->value = 1.0;
	      break;
	    case OP_PLUS: // E = u(x) + 0.0 = u(x)
	    case OP_MINUS:// E = u(x) - 0.0 = u(x)
	      temp = *cknot;
	      copy_expression(temp.Children[0], cknot);
	      cknot->Parent = temp.Parent;
	      freeKnot(&temp);
	      break;
	    default:
	      goto next2;
	    }
	  return true;
	}
    next2:
      if (cknot->Children[1]->OPtype == CONSTANT_VALUE &&
          cknot->Children[1]->value == 1.0)
	{// right-hand argument is 1.0
	  switch(cknot->OPtype)
	    {
	    case OP_TIMES: // E = u(x) * 1.0 = u(x)
	    case OP_DIVIDE: // E = u(x) / 1.0 = u(x)
	      //                  case OP_POWER: // E = u(x)^1.0 = u(x) (ONLY IF u(x) != 0)
	      temp = *cknot;
	      copy_expression(temp.Children[0], cknot);
	      cknot->Parent = temp.Parent;
	      freeKnot(&temp);
	      break;
	    default:
	      goto next3;
	    }
	  return true;
	}
      /*      if (cknot->OPtype == OP_POWER && cknot->Children[0]->OPtype == OP_POWER)
	      { // (x^b)^c = x^(b*c)
	      copy_expression(cknot, &temp);
	      freeKnot(cknot);
	      cknot->OPtype = OP_POWER;
	      cknot->Parent = temp.Parent;
	      cknot->Children[0] = (Knot*)xmalloc(sizeof(Knot));
	      cknot->Children[1] = (Knot*)xmalloc(sizeof(Knot));
	      cknot->Children[2] = 0;
	      copy_expression(temp.Children[0]->Children[0], cknot->Children[0]);
	      cknot->Children[1]->Parent = cknot;
	      cknot->Children[1]->OPtype = OP_TIMES;
	      cknot->Children[1]->Children[0] = (Knot*)xmalloc(sizeof(Knot));
	      cknot->Children[1]->Children[1] = (Knot*)xmalloc(sizeof(Knot));
	      cknot->Children[1]->Children[2] = 0;
	      copy_expression(temp.Children[0]->Children[1], cknot->Children[1]->Children[0]);
	      copy_expression(temp.Children[1], cknot->Children[1]->Children[1]);
	      cknot->Children[1]->Children[0]->Parent = cknot->Children[1];
	      cknot->Children[1]->Children[1]->Parent = cknot->Children[1];
	      freeKnot(&temp);
	      return true;
	      }*/
    }
 next3:
  int i = 0;
  while (cknot->Children[i] != 0)
    {
      if (simplify_expr(cknot->Children[i]))
	return true;
      i++;
    }
  return false;
};

void knot2expr(Knot* cknot, char* expr, int nesting_level = 0)
{
  char expr1[MAX_EXPR_LEN], expr2[MAX_EXPR_LEN], expr3[MAX_EXPR_LEN], open_parenth, closed_parenth;
  int i;
  if ((nesting_level % 2) == 0)
    {
      open_parenth = '[';
      closed_parenth = ']';
    }
  else
    {
      open_parenth = '(';
      closed_parenth = ')';
    }
  if (cknot->OPtype == CONSTANT_VALUE)
    {
      if (cknot->value == M_PI)
	strcpy(expr, "pi");
      else
	{
	  if (cknot->value == M_E)
	    strcpy(expr, "e");
	  else
	    {
	      sprintf(expr, "%G", cknot->value);
	      make_exponent_explicit(expr); // "1.0E-6" -> "1.0*10^(-6)"
	    }
	}
      return;
    }
  if (cknot->OPtype == TOKEN_VARIABLE_X)
    {
      strcpy(expr, "x");
      return;
    }
  if (cknot->OPtype == TOKEN_VARIABLE_Y)
    {
      strcpy(expr, "y");
      return;
    }
  char x = getOPchar(cknot->OPtype);
  int this_precedence, lowest_sub_precedence;
  if (x != '\0') // cknot->OPtype is a one-character operator
    {
      this_precedence = getOPPrecedence(cknot->OPtype);
      if (cknot->OPtype == OP_UNARY_MINUS)
	{
	  lowest_sub_precedence = getOPPrecedence(cknot->Children[0]->OPtype);
	  knot2expr(cknot->Children[0], expr2, nesting_level + 1);
	  if (lowest_sub_precedence <= this_precedence)
	    {// if sub-level-operator has lower (or equal!) precedence, set brackets
	      expr3[0] = open_parenth;
	      strcpy(expr3 + 1, expr2);
	      strcpy(expr2, expr3);
	      i = strlen(expr2);
	      expr2[i] = closed_parenth;
	      expr2[i + 1] = '\0';
	    }
	  strcpy(expr1, "");
	}
      else  // binary, one-character operator (*,+,-,/,^)
	{
	  // handle left-hand-argument
	  lowest_sub_precedence = getOPPrecedence(cknot->Children[0]->OPtype);
	  knot2expr(cknot->Children[0], expr1, nesting_level + 1);
	  if (lowest_sub_precedence < this_precedence || (cknot->OPtype == cknot->Children[0]->OPtype && !isCommutative(cknot->OPtype))
	      || (cknot->Children[0]->OPtype == CONSTANT_VALUE
		  && cknot->Children[0]->value < 0.0) || cknot->Children[0]->OPtype == OP_UNARY_MINUS)
	    {// if sub-level-operator has lower precedence, (or equal and operator is not commutative) set brackets
	      // also set brackets if left argument is negative immediate (like x+(-3)*x)
	      // also set brackets if left argument is a unary minus (like x+(-x)*x)
	      expr3[0] = open_parenth;
	      strcpy(expr3 + 1, expr1);
	      strcpy(expr1, expr3);
	      i = strlen(expr1);
	      expr1[i] = closed_parenth;
	      expr1[i + 1] = '\0';
	    }
	  // handle right-hand-argument
	  lowest_sub_precedence = getOPPrecedence(cknot->Children[1]->OPtype);
	  knot2expr(cknot->Children[1], expr2, nesting_level + 1);
	  if (lowest_sub_precedence < this_precedence || (cknot->OPtype == cknot->Children[1]->OPtype && !isCommutative(cknot->OPtype))
	      || (cknot->Children[1]->OPtype == CONSTANT_VALUE && cknot->Children[1]->value < 0.0) || cknot->Children[1]->OPtype == OP_UNARY_MINUS)
	    {// if sub-level-operator has lower precedence, (or equal and operator is not commutative) set brackets
	      // set brackets if right argument is a negative immediate as well
	      // set brackets if right argument is a (unary) minus as well
	      expr3[0] = open_parenth;
	      strcpy(expr3 + 1, expr2);
	      strcpy(expr2, expr3);
	      i = strlen(expr2);
	      expr2[i] = closed_parenth;
	      expr2[i + 1] = '\0';
	    }
	}
      strcpy(expr, expr1);
      i = strlen(expr);
      expr[i] = x;
      strcpy(expr + i + 1, expr2);
      return;
    }
  else  // cknot->OPtype is a multi-character operator
    { // binary operator
      if (cknot->OPtype == OP_LOGARITHM)
	{
	  knot2expr(cknot->Children[0], expr1, nesting_level + 1);
	  knot2expr(cknot->Children[1], expr2, nesting_level + 1);
	  getmcOPString(cknot->OPtype, expr);
	  appendString(expr, "(");
	  appendString(expr, expr1);
	  appendString(expr, ", ");
	  appendString(expr, expr2);
	  appendString(expr, ")");
	  return;
	}
      else // unary operator
	{
	  knot2expr(cknot->Children[0], expr1, nesting_level + 1);
	  getmcOPString(cknot->OPtype, expr);
	  appendString(expr, "(");
	  appendString(expr, expr1);
	  appendString(expr, ")");
	  return;
	}
    }
};

void freeKnot(Knot* knot)
{// it is important that this leaves the parent field of the first passed structure alone
  // look if any of the children are empty...
  Knot* cknot;
  int i = 0;
  cknot = knot->Children[0];
  while (cknot != 0)
    {
      if (cknot->Children[0] == 0)
	{ // if our child has no children, free it
	  xfree(cknot); // free knot->Children[i]
	  knot->Children[i] = 0;
	}
      else // otherwise dig deeper
	freeKnot(cknot);
      i++;
      cknot = knot->Children[i];
    }
};

void record_expression(char* expr, Knot* current_knot)
{
  int i, j, OPpos, precedence, nOPs, nOpenParenth;
  OperatorType lasttype, type;
  char expr1[MAX_EXPR_LEN], expr2[MAX_EXPR_LEN];
  strcpy(expr1, expr);
 begin:
  precedence = 10000;
  i = 0;
  nOpenParenth = 0;
  nOPs = 0;
  while (expr1[i] != '\0')
    {
      if (IS_OPEN_PARENTH(expr1[i]))
	nOpenParenth++;
      if (IS_CLOSED_PARENTH(expr1[i]))
	nOpenParenth--;
      if (nOpenParenth == 0)
	{
	  lasttype = getOPtype(expr1[i]); // check for simple OP's: +,-,*,/,^
	  if (lasttype != OP_DONTKNOW)
	    {
	      //check for unary operator
	      if (lasttype == OP_MINUS)
		{
		  if (i == 0)
		    lasttype = OP_UNARY_MINUS;
		  else // another check for unary minus
		    {
		      if (IS_OPEN_PARENTH(expr1[i - 1]))
			lasttype = OP_UNARY_MINUS;
		    }
		}
	      nOPs++;
	      j = getOPPrecedence(lasttype);
	      if (j < precedence)
		{
		  precedence = j;
		  type = lasttype;
		  OPpos = i;
		}
	    }
	}
      i++;
    }
  if (nOpenParenth != 0)
    {
      if (nOpenParenth < 0)
	record_Error = EXPR_MISSING_OPEN;
      else
	record_Error = EXPR_MISSING_CLOSED;
      current_knot->Children[0] = 0;
      return;
    }
  if (nOPs != 0)
    {
      strcpy(expr2, expr1 + OPpos + 1);
      expr1[OPpos] = '\0';
      if (expr2[0] == '\0')
	{// if right-hand expression is missing: error
	  record_Error = EXPR_OPERATOR_RIGHT_HAND_EXPR_MISSING;
	  current_knot->Children[0] = 0;
	  return;
	}
      if (expr1[0] == '\0' && type != OP_UNARY_MINUS) // left-hand-expression is missing
	{
	  if (type == OP_PLUS)
	    {// unary +-operator is obsolete, so expression must be a token (or another complex expression bracketed)
	      record_expression(expr2, current_knot);
	      return;
	    }
	  record_Error = EXPR_OPERATOR_LEFT_HAND_EXPR_MISSING;
	  current_knot->Children[0] = 0;
	  return;
	}
      current_knot->OPtype = type;
      current_knot->Children[0] = (Knot*)xmalloc(sizeof(Knot));
      current_knot->Children[0]->Parent = current_knot;
      if (type == OP_UNARY_MINUS)
	{
	  current_knot->Children[1] = 0;
	  record_expression(expr2, current_knot->Children[0]);
	  return;
	}
      current_knot->Children[1] = (Knot*)xmalloc(sizeof(Knot));
      current_knot->Children[1]->Parent = current_knot;
      current_knot->Children[2] = 0;
      record_expression(expr1, current_knot->Children[0]);
      record_expression(expr2, current_knot->Children[1]);
      return;
    }
  else
    { // no "one-character" operators in expression
      if (IS_OPEN_PARENTH(expr1[0]))
	{ // parentheses are obsolete
	  strcpy(expr2, expr1 + 1);
	  strcpy(expr1, expr2);
	  expr1[strlen(expr1) - 1] = '\0';
	  goto begin;
	}
      type = getmcOPtype(expr1, &i); // i = length of operator
      if (type != OP_DONTKNOW) // multi-character operator found
	{
	  if ((!IS_OPEN_PARENTH(expr1[i])) || (!IS_CLOSED_PARENTH(expr[strlen(expr) - 1])))
	    {
	      current_knot->Children[0] = 0;
	      record_Error = EXPR_FUNCTION_WO_PARENTH;
	      return;
	    }
	  current_knot->OPtype = type;
	  strcpy(expr2, expr1 + i + 1);
	  expr2[strlen(expr2) - 1] = '\0';
	  if (type != OP_LOGARITHM)
	    current_knot->Children[1] = 0;
	  else
	    {// operator is log(base,exp) -> expr2 = "base,exp"
	      i = 0;
	      while (expr2[i] != '\0' && expr2[i] != ',')
		i++;
	      if (expr2[i] == '\0')
		{
		  record_Error = EXPR_LOG_MISSING_2ND_ARG;
		  current_knot->Children[0] = 0;
		  return;
		}
	      strcpy(expr1, expr2 + i + 1);
	      expr2[i] = '\0';
	      current_knot->Children[0] = (Knot*)xmalloc(sizeof(Knot));
	      current_knot->Children[0]->Parent = current_knot;
	      current_knot->Children[1] = (Knot*)xmalloc(sizeof(Knot));
	      current_knot->Children[1]->Parent = current_knot;
	      current_knot->Children[2] = 0;
	      record_expression(expr2, current_knot->Children[0]);
	      record_expression(expr1, current_knot->Children[1]);
	      return;
	    }
	  current_knot->Children[0] = (Knot*)xmalloc(sizeof(Knot));
	  current_knot->Children[0]->Parent = current_knot;
	  record_expression(expr2, current_knot->Children[0]);
	  return;
	}
      // operator is no multi-character-OP and no one-character-OP
      current_knot->Children[0] = 0;
      if (expr1[0] == 'x' && expr1[1] == '\0')
	current_knot->OPtype = TOKEN_VARIABLE_X;
      else if (expr1[0] == 'y' && expr1[1] == '\0')
	current_knot->OPtype = TOKEN_VARIABLE_Y;
      else
	{
	  current_knot->OPtype = CONSTANT_VALUE;
	  if (strcmp(expr1, "pi") == 0)
	    current_knot->value = M_PI;
	  else
            if (expr1[0] == 'e' && expr1[1] == '\0')
	      current_knot->value = M_E;
            else
	      {
		if (expr1[0] == '\0')
		  {
		    record_Error = EXPR_EMPTY_EXPRESSION;
		    return;
		  }
		i = 0;
		while (expr1[i] != '\0')
		  {
		    if (!isdigit(expr1[i]) && expr1[i] != '.' && expr1[i] != 'e' && expr1[i] != '-')
		      {
			record_Error = EXPR_UNKNOWN_FUNCTION;
			return;
		      }
		    i++;
		  }
		sscanf(expr1, "%lf", &current_knot->value);
	      }
	}
    }
};

double evaluate_expression(Knot* current_knot)
{
  double temp1, temp2;
  switch (current_knot->OPtype)
    {
    case OP_PLUS:
      temp1 = evaluate_expression(current_knot->Children[0]);
      temp2 = evaluate_expression(current_knot->Children[1]);
      return temp1 + temp2;
    case OP_MINUS:
      temp1 = evaluate_expression(current_knot->Children[0]);
      temp2 = evaluate_expression(current_knot->Children[1]);
      return temp1 - temp2;
    case OP_TIMES:
      temp1 = evaluate_expression(current_knot->Children[0]);
      temp2 = evaluate_expression(current_knot->Children[1]);
      return temp1 * temp2;
    case OP_DIVIDE:
      temp1 = evaluate_expression(current_knot->Children[0]);
      temp2 = evaluate_expression(current_knot->Children[1]);
      if (fabs(temp2) < SMALL_NUMBER)
	{
	  evaluate_Error = EVALUATE_IS_ASYMPTOTE;
	  return 0.0;
	}
      else
	return temp1 / temp2;
    case OP_POWER:
      temp1 = evaluate_expression(current_knot->Children[0]);
      temp2 = evaluate_expression(current_knot->Children[1]);
      if ((double)((int)temp2) != temp2 && temp1 < 0.0)
	{// exponent is irrational and base is negative
	  evaluate_Error = EVALUATE_NOT_IN_D;
	  return 0.0;
	}
      else if (temp1 == 0.0 && temp2 <= 0.0) // 0^0 defined???
	{
	  evaluate_Error = EVALUATE_IS_ASYMPTOTE;
	  return 0.0;
	}
      else
	return pow(temp1, temp2);
    case OP_UNARY_MINUS:
      temp1 = evaluate_expression(current_knot->Children[0]);
      return -temp1;
    case OP_SINE:
      temp1 = evaluate_expression(current_knot->Children[0]);
      return sin(temp1);
    case OP_COSINE:
      temp1 = evaluate_expression(current_knot->Children[0]);
      return cos(temp1);
    case OP_TANGENT:
      temp1 = evaluate_expression(current_knot->Children[0]);
      temp2 = M_PI_2 / temp1; // find out if temp1 is a number such that n*temp1 = pi/2 (n is in N) => n = (pi/2)/temp1 (if n is in N, tan(temp1) is not defined)
      if ((double)((int)temp2) == temp2)
	{
	  evaluate_Error = EVALUATE_IS_ASYMPTOTE;
	  return 0.0;
	}
      else
	return tan(temp1);
    case OP_COTANGENT:
      temp1 = evaluate_expression(current_knot->Children[0]);
      temp2 = M_PI / temp1; // find out if temp1 is a number such that n*temp1 = pi (n is in N) => n = pi/temp1 (if n is in N, cot(temp1) is not defined)
      if ((double)((int)temp2) == temp2 || fabs(temp1) < SMALL_NUMBER)
	{
	  evaluate_Error = EVALUATE_IS_ASYMPTOTE;
	  return 0.0;
	}
      else
	return 1.0 / tan(temp1);
    case OP_ARCUS_SINE:
      temp1 = evaluate_expression(current_knot->Children[0]);
      if (temp1 < -1.0 || temp1 > 1.0)
	{
	  evaluate_Error = EVALUATE_NOT_IN_D;
	  return 0.0;
	}
      else
	return asin(temp1);
    case OP_ARCUS_COSINE:
      temp1 = evaluate_expression(current_knot->Children[0]);
      if (temp1 < -1.0 || temp1 > 1.0)
	{
	  evaluate_Error = EVALUATE_NOT_IN_D;
	  return 0.0;
	}
      else
	return acos(temp1);
    case OP_ARCUS_TANGENT:
      temp1 = evaluate_expression(current_knot->Children[0]);
      return atan(temp1);
    case OP_ARCUS_COTANGENT:
      temp1 = evaluate_expression(current_knot->Children[0]);
      return atan(-temp1) + M_PI_2; // the graph of arccot(x) is arctan(x) mirrored on the y-axis and translated along y by pi/2, so arccot(x) = arctan(-x)+pi/2
    case OP_NATURAL_LOG:
      temp1 = evaluate_expression(current_knot->Children[0]);
      if (temp1 < 0.0)
	{
	  evaluate_Error = EVALUATE_NOT_IN_D;
	  return 0.0;
	}
      if (fabs(temp1) < SMALL_NUMBER)
	{
	  evaluate_Error = EVALUATE_IS_ASYMPTOTE;
	  return 0.0;
	}
      return log(temp1);
    case OP_LOGARITHM:
      temp1 = evaluate_expression(current_knot->Children[0]);
      temp2 = evaluate_expression(current_knot->Children[1]);
      if (temp1 < 0.0 || temp2 < 0.0)
	{
	  evaluate_Error = EVALUATE_NOT_IN_D;
	  return 0.0;
	}
      else if (fabs(temp1) < SMALL_NUMBER || fabs(temp2) < SMALL_NUMBER)
	{
	  evaluate_Error = EVALUATE_IS_ASYMPTOTE;
	  return 0.0;
	}
      else
	return log(temp2) / log(temp1);
    case OP_ABSOLUTE_VALUE:
      temp1 = evaluate_expression(current_knot->Children[0]);
      return fabs(temp1);
    case OP_SIGN:
      temp1 = evaluate_expression(current_knot->Children[0]);
      if (temp1 == 0.0)
	return 0.0;
      if (temp1 < 0.0)
	return -1.0;
      else
	return 1.0;
    case OP_SQUARE_ROOT:
      temp1 = evaluate_expression(current_knot->Children[0]);
      if (temp1 < 0.0)
	{
	  evaluate_Error = EVALUATE_NOT_IN_D;
	  return 0.0;
	}
      else
	return sqrt(temp1);
    case TOKEN_VARIABLE_X:
      return VARIABLE_X;
    case TOKEN_VARIABLE_Y:
      return VARIABLE_Y;
    case CONSTANT_VALUE:
      return current_knot->value;
    case OP_DONTKNOW:
      handleError("OP_DONTKNOW switched in evaluate...");
    }
  return 0.0; //suppresses warning
};

