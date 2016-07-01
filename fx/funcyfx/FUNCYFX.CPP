#include "vbetwo.h"
#include "fifo99.h"
#include "grtext.h"
#include "function.h"
#include "palettes.h"
#include "fileio.h"
#include <stdio.h>
#include <string.h>
#include "lcstdlib.h" // xmalloc/xfree
#include <math.h>
#include <pc.h>
#include <keys.h>

float MINX = -10.0F, MAXX = 10.0F, MINY = -10.0F, MAXY = 10.0F;
bool show_bounds = true;
int CONSOLE_FRAME_WIDTH_X, CONSOLE_FRAME_WIDTH_Y;

#define SCAN_STEPS                        100000
#define MAX_INFINITES                     2048
#define MAX_STORAGE                       100
#define OUTSIDE(x,y)                      (x < MINX || x > MAXX || y < MINY || y > MAXY)
// input storage for console
#define MAX_CONSOLE_LINES                 500
#define MAX_CONSOLE_LINE_LEN              256
#define CONSOLE_FONT_WIDTH                8
#define CONSOLE_FONT_HEIGHT               8
#define CONSOLE_CELL_HEIGHT               10
#define SCROLL_STEP                       0.05

struct console_Line
{
  char szline[MAX_CONSOLE_LINE_LEN];
  bool editable;
};

char* console_pic, *bgpic, *temppic;
console_Line console_Lines[MAX_CONSOLE_LINES];
int console_FirstLine = 0, CharsPerLine, VisibleConsoleLines, cursor_x = 0, cursor_y = 0, console_Height, console_Top;
#define WRAP(x)       ((x) % MAX_CONSOLE_LINES)


const unsigned char boundsidentifycolor = CCDARKGRAY, axescolor = CCGRAY,
  msgcol = CCWHITE, consolecolor = CCWHITE, cursorcolor = CCYELLOW, intersectioncolor = CCORANGE,
  zeroscolor = CCBLUE, tpcolor = CCRED, vertexcolor = CCBROWN, maxcolor = CCCYAN, mincolor = CCVIOLET, integralcolor = CCDARKRED;

struct Function2D
{
  Knot parseTreeRoot;
  Knot parseTreeRoot_slope;
  Knot parseTreeRoot_concavity;
  char szFunction[MAX_EXPR_LEN];
  char sz1stDerivative[MAX_EXPR_LEN];
  char sz2ndDerivative[MAX_EXPR_LEN];
  bool fVisible, f_slopeVisible, f_concavityVisible;
  bool isvalid, derivatives_valid;
  unsigned char fcolor, f_slopecolor, f_concavitycolor;
  DERIVE_ERROR_VAL derive_Error;
  bool is_infinite[MAX_INFINITES];
  bool is_infinite_slope[MAX_INFINITES];
  bool is_infinite_concavity[MAX_INFINITES];
};

Function2D fns[4];
Function2D* cur_func;
int maxdrawx, maxdrawy;

void analyze_fn(int idx, int* cmdline);
void find_zeros(int idx, int* cmdline);
void find_intersections(int idx1, int idx2, int* cmdline);
void reverse_map(int u, double* x);
void map_y_on_screen(double y, int* v);
void report_derive_error(char* expr, int* cmdline);
void respond_to_Console(const char* str, int* cmdline);
void report_record_error(char* expr, int* cmdline);
void recordFunc(char* szFunction, Function2D& func);
void map2screen(double x, double y, int* u, int* v);
void redrawfunc();
void console();
void handleInput(char* szcommand, int* cmdline);
void handleCommand(char* szcmd, char* szparamlist, int* cmdline);
void redrawfns();
void draw_infinite_line(int u, unsigned char color);
void find_infinites();
void definite_integral(double left, double right, int idx, int* cmdline);


inline void drawline(double x1, double y1, double x2, double y2, unsigned char color)
{
  int u1, v1, u2, v2;
  map2screen(x1, y1, &u1, &v1);
  map2screen(x2, y2, &u2, &v2);
  clipline(u1, v1, u2, v2, color);
};

inline double funcy(double x)
{
  VARIABLE_X = x;
  return evaluate_expression(&cur_func->parseTreeRoot);
};

inline double func_slope(double x)
{
  VARIABLE_X = x;
  return evaluate_expression(&cur_func->parseTreeRoot_slope);
};

inline double func_concavity(double x)
{
  VARIABLE_X = x;
  return evaluate_expression(&cur_func->parseTreeRoot_concavity);
};

void find_infinites()
{
  float x, dx, dummy;
  int u, i, v;
  x = MINX;
  dx = (MAXX - MINX) / (double)SCAN_STEPS;
  for (i = 0; i < xres; i++)
    {
      cur_func->is_infinite[i] = false;
      cur_func->is_infinite_slope[i] = false;
      cur_func->is_infinite_concavity[i] = false;
    }
  for (i = SCAN_STEPS; i > 0; i--)
    {
      evaluate_Error = EVALUATE_NO_ERROR;
      dummy = funcy(x);
      if (evaluate_Error == EVALUATE_IS_ASYMPTOTE)
	{
	  map2screen(x, dummy, &u, &v);
	  cur_func->is_infinite[u] = true;
	}
      //...
      x += dx;
    }
};

void draw_infinite_line(int u, unsigned char color)
{
  int v;
  for (v = 0; v <= maxdrawy; v++)
    {
      if (v % 3)
	putpixel(u, v, color);
    }
};

void redrawfns()
{
  clearoffscreenmem();
  drawline(-100000.0, 0.0, 100000.0, 0.0, NEAREST[axescolor]);
  drawline(0.0, -100000.0, 0.0, 100000.0, NEAREST[axescolor]);
  settextsize(8, 8);
  char str[30];
  if (show_bounds)
    {
      GRTEXT_BGColor = 0;
      floatstr(MINX, str, 2);
      showstrxy(8, maxdrawy / 2 - 4, str, NEAREST[boundsidentifycolor]);
      floatstr(MAXX, str, 2);
      showstrxy(maxdrawx - (strlen(str) + 1) * 8, maxdrawy / 2 - 4, str, NEAREST[boundsidentifycolor]);
      floatstr(MAXY, str, 2);
      showstrxy(maxdrawx / 2 - (strlen(str) * 8) / 2, 0, str, NEAREST[boundsidentifycolor]);
      floatstr(MINY, str, 2);
      showstrxy(maxdrawx / 2 - (strlen(str) * 8) / 2, maxdrawy - 10, str, NEAREST[boundsidentifycolor]);
      GRTEXT_BGColor = GRTEXT_BGTRANSPARENT;
    }
  if (fns[0].fVisible)
    {
      cur_func = &fns[0];
      redrawfunc();
    }
  if (fns[1].fVisible)
    {
      cur_func = &fns[1];
      redrawfunc();
    }
  if (fns[2].fVisible)
    {
      cur_func = &fns[2];
      redrawfunc();
    }
  if (fns[3].fVisible)
    {
      cur_func = &fns[3];
      redrawfunc();
    }
  settextsize(CONSOLE_FONT_WIDTH, CONSOLE_FONT_HEIGHT);
};

void recordFunc(Function2D& func)
{
  char szfn[MAX_EXPR_LEN];
  int i = 0;
  while (func.szFunction[i] != '\0')
    {
      if (func.szFunction[i] != ' ')
	szfn[i] = func.szFunction[i];
      i++;
    }
  szfn[i] = '\0';
  strlwr(szfn);
  func.fVisible = true;
  func.f_slopeVisible = false;
  func.f_concavityVisible = false;
  func.fcolor = CCWHITE;
  func.f_slopecolor = CCYELLOW;
  func.f_concavitycolor = CCORANGE;
  func.parseTreeRoot.Children[0] = 0;
  func.isvalid = true;
  record_expression(szfn, &func.parseTreeRoot);
};

void map2screen(double x, double y, int* u, int* v)
{
  //y = -y; // because +v extends down the screen
  x -= MINX;
  y -= MAXY;// x/y are in [0; MAXX/Y - MINX/Y]
  x /= (MAXX - MINX);
  y /= -(MAXY - MINY);// x/y are in [0; 1]
  x *= maxdrawx;
  y *= maxdrawy;
  *u = (int)(x + 0.5);
  *v = (int)(y + 0.5);
};

void map_y_on_screen(double y, int* v)
{
  y -= MAXY; // y is in [0; MAXY]
  y /= (MINY - MAXY); // y is in [0; 1]
  y *= maxdrawy;
  *v = (int)(y + 0.5);
};

void reverse_map(int u, double* x)
{// u is in [0; xres - 1]
  *x = u / (double)(xres - 1); // x is in [0; 1]
  *x = *x * (MAXX - MINX); // x is in [0; MAXX - MINX]
  *x = *x + MINX; // x is in [MINX; MAXX]
};

void redrawfunc()
{
  double x, prev_x, y, prev_y, slope, prev_slope, concav, prev_concav, dx;
  int u, v1, prev_v1, v2, prev_v2, v3, prev_v3;
  EVALUATE_ERROR_VAL eval1, eval2, eval3, prev_eval1, prev_eval2, prev_eval3;
  //find_infinites();
  prev_x = MINX;
  evaluate_Error = EVALUATE_NO_ERROR;
  prev_y = funcy(prev_x);
  prev_eval1 = evaluate_Error;
  map_y_on_screen(prev_y, &prev_v1);
  if (cur_func->f_slopeVisible)
    {
      evaluate_Error = EVALUATE_NO_ERROR;
      prev_slope = func_slope(prev_x);
      prev_eval2 = evaluate_Error;
      map_y_on_screen(prev_slope, &prev_v2);
    }
  if (cur_func->f_concavityVisible)
    {
      evaluate_Error = EVALUATE_NO_ERROR;
      prev_concav = func_concavity(prev_x);
      prev_eval3 = evaluate_Error;
      map_y_on_screen(prev_concav, &prev_v3);
    }
  dx = (MAXX - MINX) / (double)(xres - 1);
  x = prev_x + dx;
  for (u = 1; u < xres; u++)
    {
      evaluate_Error = EVALUATE_NO_ERROR;
      y = funcy(x);
      eval1 = evaluate_Error;
      map_y_on_screen(y, &v1);
      if (cur_func->is_infinite[u])
	draw_infinite_line(u, NEAREST[CCGREEN]);
      else if (eval1 == EVALUATE_NO_ERROR && prev_eval1 == EVALUATE_NO_ERROR)
	{
	  if (!cur_func->is_infinite[u - 1] && !(OUTSIDE(x,y) && OUTSIDE(prev_x, prev_y)))
	    clipline(u - 1, prev_v1, u, v1, NEAREST[cur_func->fcolor]);
	}
      prev_y = y;
      prev_v1 = v1;
      if (cur_func->f_slopeVisible)
	{
	  evaluate_Error = EVALUATE_NO_ERROR;
	  slope = func_slope(x);
	  eval2 = evaluate_Error;
	  map_y_on_screen(slope, &v2);
	  if (cur_func->is_infinite_slope[u])
	    draw_infinite_line(u, NEAREST[CCGREEN]);
	  else if (eval2 == EVALUATE_NO_ERROR && prev_eval2 == EVALUATE_NO_ERROR)
	    {
	      if (!cur_func->is_infinite_slope[u - 1] && !(OUTSIDE(x,slope) && OUTSIDE(prev_x, prev_slope)))
		clipline(u - 1, prev_v2, u, v2, NEAREST[cur_func->f_slopecolor]);
	    }
	  prev_slope = slope;
	  prev_eval2 = eval2;
	  prev_v2 = v2;
	}
      if (cur_func->f_concavityVisible)
	{
	  evaluate_Error = EVALUATE_NO_ERROR;
	  concav = func_concavity(x);
	  eval3 = evaluate_Error;
	  map_y_on_screen(concav, &v3);
	  if (cur_func->is_infinite_concavity[u])
	    draw_infinite_line(u, NEAREST[CCGREEN]);
	  else if (eval3 == EVALUATE_NO_ERROR && prev_eval3 == EVALUATE_NO_ERROR)
	    {
	      if (!cur_func->is_infinite_concavity[u - 1] && !(OUTSIDE(x,concav) && OUTSIDE(prev_x, prev_concav)))
		clipline(u - 1, prev_v3, u, v3, NEAREST[cur_func->f_concavitycolor]);
	    }
	  prev_concav = concav;
	  prev_v3 = v3;
	  prev_eval3 = eval3;
	}
      prev_x = x;
      prev_eval1 = eval1;
      x += dx;
    }
};

void console()
{
  int key, i, j;
  char temp[1024];
  do
    {
      putimage(0, console_Top, console_pic);
      // show previous input
      for (i = console_FirstLine, j = 0; j < VisibleConsoleLines; j++, i++)
	showstrxy(CONSOLE_FRAME_WIDTH_X, console_Top + CONSOLE_FRAME_WIDTH_Y + (j * CONSOLE_CELL_HEIGHT), console_Lines[i].szline, NEAREST[consolecolor]);
      // show cursor
      textcurx = CONSOLE_FRAME_WIDTH_X + (cursor_x * CONSOLE_FONT_WIDTH);
      textcury = console_Top + CONSOLE_FRAME_WIDTH_Y + (cursor_y * CONSOLE_CELL_HEIGHT);
      showchar('_', NEAREST[cursorcolor]);
      flipoffscreenmem();
      key = getkey();
      // handle input
      switch (key)
	{
	case K_Control_S:
	  saveScreen("screen.spr");
	  break;
	case K_Tab:
	  break;
	case K_Up:
	  if (console_FirstLine + cursor_y > 0)
	    {
	      if (cursor_y > 0)
		cursor_y--;
	      else
		{
		  if (console_FirstLine > 0)
		    console_FirstLine--;
		}
	      cursor_x = strlen(console_Lines[console_FirstLine + cursor_y].szline);
	      if (cursor_x >= CharsPerLine)
		cursor_x--;
	    }
	  break;
	case K_Down:
	  if (cursor_y < VisibleConsoleLines - 1)
	    cursor_y++;
	  else
	    {
	      if (console_FirstLine > MAX_CONSOLE_LINES - VisibleConsoleLines - 2)
		{
		  console_FirstLine = 0;
		  cursor_y = 0;
		}
	      else
		console_FirstLine++;
	    }
	  cursor_x = strlen(console_Lines[console_FirstLine + cursor_y].szline);
	  if (cursor_x >= CharsPerLine)
	    cursor_x--;
	  break;
	case K_Left:
	  if (cursor_x > 0)
	    cursor_x--;
	  break;
	case K_Right:
	  if (cursor_x < VisibleConsoleLines - 1)
	    cursor_x++;
	  break;
	case K_BackSpace:
	  if (cursor_x > 0 && console_Lines[console_FirstLine + cursor_y].editable == true)
	    {
	      cursor_x--;
	      strcpy(temp, console_Lines[console_FirstLine + cursor_y].szline);
	      console_Lines[console_FirstLine + cursor_y].szline[cursor_x] = '\0';
	      appendString(console_Lines[console_FirstLine + cursor_y].szline, temp + cursor_x + 1);
	    }
	  break;
	case K_Delete:
	  if (console_Lines[console_FirstLine + cursor_y].editable == true && cursor_x < (int)strlen(console_Lines[console_FirstLine + cursor_y].szline))
	    {
	      strcpy(temp, console_Lines[console_FirstLine + cursor_y].szline);
	      console_Lines[console_FirstLine + cursor_y].szline[cursor_x] = '\0';
	      appendString(console_Lines[console_FirstLine + cursor_y].szline, temp + cursor_x + 1);
	    }
	  break;
	case K_QuestionMark:
	  i = console_FirstLine + cursor_y;
	  respond_to_Console("? - Console commands:", &i);
	  respond_to_Console("set(<f/g/h/i>, <function>) - assign a function, use f/g/h/i[(x)]=<function> instead", &i);
	  respond_to_Console("i.e.: f = [-x^3 - 0.5*x^(-2) + 3*x] / (x^2 + sin(x^2) / ln(abs(x)))", &i);
	  respond_to_Console("hide/show (<f/g/h/i['/'']>) - enables/disables plotting of the function", &i);
	  respond_to_Console("hide   show   whatsup   clear   redraw   range   intersect   analyze   eval(uate)   integ(ral)   texit", &i);
	  respond_to_Console("just enter any command for usage and a list of arguments", &i);
	  respond_to_Console("functions available for function-definitions are: [arc]sin/cos/tan/cot, ln/log(x,y)/abs/sgn/sqrt", &i);
	  for (i -= console_FirstLine + cursor_y; i > 0; i--)
	    {
	      if (cursor_y < VisibleConsoleLines - 1)
		cursor_y++;
	      else
		{
		  if (console_FirstLine > MAX_CONSOLE_LINES - VisibleConsoleLines - 2)
		    {
		      console_FirstLine = 0;
		      cursor_y = 0;
		    }
		  else
		    console_FirstLine++;
		}
	    }
	  console_Lines[console_FirstLine + cursor_y].szline[0] = '\0';
	  console_Lines[console_FirstLine + cursor_y].editable = true;
	  cursor_x = 0;
	  break;
	case K_Return:
	  i = console_FirstLine + cursor_y;
	  if (console_Lines[i].editable == false)
	    break;
	  console_Lines[i].editable = true;
	  handleInput(console_Lines[i].szline, &i);
	  i = i - console_FirstLine - cursor_y;
	  for (; i > 0; i--)
	    {// skip the lines that are filled with responses
	      if (cursor_y < VisibleConsoleLines - 1)
		cursor_y++;
	      else
		{
		  if (console_FirstLine > MAX_CONSOLE_LINES - VisibleConsoleLines - 2)
		    {
		      console_FirstLine = 0;
		      cursor_y = 0;
		    }
		  else
		    console_FirstLine++;
		}
	    }
	  console_Lines[console_FirstLine + cursor_y].szline[0] = '\0';
	  console_Lines[console_FirstLine + cursor_y].editable = true;
	  cursor_x = 0;
	  break;
	default:// std ascii, just add to stream
	  if (cursor_x < CharsPerLine - 1 && console_Lines[console_FirstLine + cursor_y].editable == true)
	    {
	      i = strlen(console_Lines[console_FirstLine + cursor_y].szline);
	      console_Lines[console_FirstLine + cursor_y].szline[cursor_x] = (char)key;
	      cursor_x++;
	      if (i <= cursor_x)
		console_Lines[console_FirstLine + cursor_y].szline[cursor_x] = '\0';
	    }
	  break;
	}
    } while (key != K_Tab && key != K_Escape);
};

void handleInput(char* szcommand, int* cmdline)
{
  int i, j, openpos, closepos;
  i = 0;
  char szcmd[MAX_EXPR_LEN], szparams[MAX_EXPR_LEN];
  i = 0; j = 0;
  while (szcommand[i] != '\0')
    {
      if (szcommand[i] != ' ')
	{
	  szcmd[j] = szcommand[i];
	  j++;
	}
      i++;
    }
  szcmd[j] = '\0';
  strlwr(szcmd);
  i = 0;
  while (szcmd[i] != '=' && szcmd[i] != '\0')
    i++; // check for assignment
  if (szcmd[i] != '\0')
    {
      strcpy(szparams, szcmd + i + 1); // szparams = <function>
      szcmd[1] = '\0'; // szcmd = "f/g/h/i"
      appendString(szcmd, ",");
      appendString(szcmd, szparams);
      strcpy(szparams, szcmd);
      strcpy(szcmd, "set");
      goto pass_command;
    }
  openpos = 0;
  while (szcmd[openpos] != '\0' && szcmd[openpos] != '(' && szcmd[openpos] != '[' && szcmd[openpos] != '{')
    openpos++;
  if (szcmd[openpos] == '\0')
    {// command without parameter
    no_param:
      if (strcmp(szcmd, "clear") == 0)
	{
	  for(i = 0; i < 4; i++)
	    {
	      fns[i].fVisible = false;
	      fns[i].f_slopeVisible = false;
	      fns[i].f_concavityVisible = false;
	    }
	  respond_to_Console("cleared.", cmdline);
	  redrawfns();
	  return;
	}
      if (strcmp(szcmd, "fuck_you") == 0 || strcmp(szcmd, "fuckyou") == 0)
	{
	  respond_to_Console("Fuck yourself.", cmdline);
	  return;
	}
      if (strcmp(szcmd, "redraw") == 0)
	{
	  redrawfns();
	  respond_to_Console("Functions are redrawn.", cmdline);
	  return;
	}
      if (strcmp(szcmd, "range") == 0)
	{
	  strcpy(szparams, "left = ");
	  floatstr(MINX, szcmd);
	  appendString(szparams, szcmd);
	  floatstr(MAXX, szcmd);
	  appendString(szparams, "\tright = ");
	  appendString(szparams, szcmd);
	  respond_to_Console(szparams, cmdline);
	  strcpy(szparams, "bottom = ");
	  floatstr(MINY, szcmd);
	  appendString(szparams, szcmd);
	  floatstr(MAXY, szcmd);
	  appendString(szparams, "\ttop = ");
	  appendString(szparams, szcmd);
	  respond_to_Console(szparams, cmdline);
	  respond_to_Console("use range(<left>, <right>, <bottom>, <top>) to change", cmdline);
	  show_bounds = !show_bounds;
	  if (show_bounds)
	    respond_to_Console("Range is shown.", cmdline);
	  else
	    respond_to_Console("Range is hidden.", cmdline);
	  redrawfns();
	  return;
	}
      if (strcmp(szcmd, "show") == 0 || strcmp(szcmd, "hide") == 0)
	{
	  respond_to_Console("Use show/hide('f/g/h/i'['/'']) to show/hide a function", cmdline);
	  return;
	}
      if (strcmp(szcmd, "intersect") == 0)
	{
	  respond_to_Console("Use intersect(<f/g/h/i>, <f/g/h/i>) to find the intersections between two functions. ex.: intersect(f,g)", cmdline);
	  return;
	}
      if (strcmp(szcmd, "integral") == 0 || strcmp(szcmd, "integ") == 0)
	{
	  respond_to_Console("Use integ(f/g/h/i, <left>, <right>) to find the definite integral of f/g/h/i in the given interval. i.e.: integral(f, 1, 4)", cmdline);
	  return;
	}
      if (strcmp(szcmd, "analyze") == 0)
	{
	  respond_to_Console("Use analyze(<f/g/h/i>) to analyze f,g,h or i.", cmdline);
	  return;
	}
      if (strcmp(szcmd, "eval") == 0 || strcmp(szcmd, "evaluate") == 0)
	{
	  respond_to_Console("Use eval(<expression>) to evaluate an expression. ex.: eval(sin(3*pi/4))", cmdline);
	  return;
	}
      if (strcmp(szcmd, "texit") == 0)
	{
	  respond_to_Console("Use texit(<f/g/h/i>, <filename.tex>) to export a function (and its derivatives) to LaTeX2e", cmdline);
	  respond_to_Console("you need the tmf (metafont)-, the webc (main)- and the dvivga (previewer for .dvi's) packages to view this", cmdline);
	  respond_to_Console("As of 3/99, you could get the LaTeX-package from the DJGPP ftp sites (mirrors of ftp.simtel.net), in /djgpp/v2apps/tex/", cmdline);
	  return;
	}

      if (strcmp(szcmd, "whatsup") == 0)
	{
	  *cmdline = *cmdline + 1;
	  for (i = 0; i < 4; i++)
	    {
	      szparams[0] = i + 102;
	      szparams[1] = '\0';
	      if (!fns[i].isvalid)
		{
		  appendString(szparams, "(x) is not defined.");
		}
	      else
		{
		  appendString(szparams, "(x) = ");
		  appendString(szparams, fns[i].szFunction);
		}
	      respond_to_Console(szparams, cmdline);
	    }
	  return;
	}
      strcpy(szparams, "I didn't get it. (cmd = ");
      appendString(szparams, szcmd);
      appendString(szparams, ")");
      respond_to_Console(szparams, cmdline);
      return;
    }
  closepos = openpos;
  while (szcmd[closepos] != '\0')
    closepos++;
  closepos--;
  strcpy(szparams, szcmd + openpos + 1); // backup parameter list
  szparams[strlen(szparams) - 1] = '\0';
  szcmd[openpos] = '\0';
  if (szparams[0] == '\0')
    goto no_param;
 pass_command:
  *cmdline = *cmdline + 1;
  strcpy(console_Lines[*cmdline].szline, "cmd = ");
  appendString(console_Lines[*cmdline].szline, szcmd);
  appendString(console_Lines[*cmdline].szline, ", params = ");
  appendString(console_Lines[*cmdline].szline, szparams);
  handleCommand(szcmd, szparams, cmdline);
};

void respond_to_Console(const char* str, int* cmdline)
{
  char temp[MAX_EXPR_LEN];
  strcpy(temp, str);
  int charsleft = strlen(temp);
  while (charsleft > CharsPerLine)
    {
      temp[CharsPerLine] = '\0';
      strcpy(console_Lines[*cmdline].szline, temp);
      charsleft -= strlen(temp);
      console_Lines[*cmdline].editable = false;
      *cmdline = WRAP(*cmdline + 1);
      strcpy(temp, str + strlen(str) - charsleft);
    }
  if (strlen(temp) > 0)
    {
      strcpy(console_Lines[*cmdline].szline, temp);
      console_Lines[*cmdline].editable = false;
      *cmdline = WRAP(*cmdline + 1);
    }
};

void report_record_error(char* expr, int* cmdline)
{
  char temp[MAX_EXPR_LEN];
  switch (record_Error)
    {
    case EXPR_MISSING_OPEN:
      strcpy(temp, "Expression misses an open parenthesis - ");
      break;
    case EXPR_MISSING_CLOSED:
      strcpy(temp, "Expression misses a closed parenthesis - ");
      break;
    case EXPR_FUNCTION_WO_PARENTH:
      strcpy(temp, "Functions (multi-character operators, [arc]sin/cos/tan/cot, sgn/abs/log/ln) require parentheses - ");
      break;
    case EXPR_LOG_MISSING_2ND_ARG:
      strcpy(temp, "Logarithm call (log[base,exp]) is missing second argument - ");
      break;
    case EXPR_OPERATOR_RIGHT_HAND_EXPR_MISSING:
      strcpy(temp, "Operator (+,-,*,/,^) is missing right-hand expression - ");
      break;
    case EXPR_OPERATOR_LEFT_HAND_EXPR_MISSING:
      strcpy(temp, "Operator (+,-,*,/,^) is missing left-hand expression - ");
      break;
    case EXPR_EMPTY_EXPRESSION:
      strcpy(temp, "Empty Expression - ");
      break;
    case EXPR_UNKNOWN_FUNCTION:
      strcpy(temp, "Unknown function - ");
      break;
    case RECORD_NO_ERROR: // suppresses warning
      return;
    }
  appendString(temp, expr);
  respond_to_Console(temp, cmdline);
};

void report_derive_error(char* expr, int* cmdline)
{
  char temp[MAX_EXPR_LEN];
  switch (derive_Error)
    {
    case DERIVE_NO_ERROR: // suppresses warning
      return;
    case DERIVE_FUNCTION_NOT_IMPLEMENTED:
      strcpy(temp, "Couldn't find 1st/2nd derivative of ");
      appendString(temp, expr);
      appendString(temp, ": Function not yet implemented.");
      respond_to_Console(temp, cmdline);
      break;
    }
};

void handleCommand(char* szcmd, char* szparamlist, int* cmdline)
{
  char temp1[MAX_EXPR_LEN];
  char temp2[MAX_EXPR_LEN];
  int i, j;
  bool truefalse;
  Knot tempTree;
  double tempval, tempval2;
  if (strcmp(szcmd, "eval") == 0 || strcmp(szcmd, "evaluate") == 0)
    {
      record_Error = RECORD_NO_ERROR;
      tempTree.Parent = 0;
      tempTree.Children[0] = 0;
      record_expression(szparamlist, &tempTree);
      if (record_Error == RECORD_NO_ERROR)
	{
	  knot2expr(&tempTree, temp2);
	  if (depends_on_x(&tempTree))
	    {
	      strcpy(temp1, "What is x ? - ");
	      appendString(temp1, temp2);
	      respond_to_Console(temp1, cmdline);
	      return;
	    }
	  evaluate_Error = EVALUATE_NO_ERROR;
	  tempval = evaluate_expression(&tempTree);
	  freeKnot(&tempTree);
	  if (evaluate_Error != EVALUATE_NO_ERROR)
	    strcpy(temp1, " is not defined.");
	  else
	    {
	      sprintf(temp1, " = %lG", tempval);
	      make_exponent_explicit(temp1);
	    }
	  appendString(temp2, temp1);
	  respond_to_Console(temp2, cmdline);
	}
      else
	report_record_error(temp1, cmdline);
      return;
    }
  if (strcmp(szcmd, "integral") == 0 || strcmp(szcmd, "integ") == 0)
    {// integral(f/g/h/i, <left>, <right>)
      i = 0; j = 0;
      while (szparamlist[i] != '\0')
	{
	  if (szparamlist[i] == ',')
	    j++;
	  i++;
	}
      if (j < 2)
	{
	  respond_to_Console("Too few parameters passed to integral(f/g/h/i, <left>, <right>)", cmdline);
	  return;
	}
      if (j > 2)
	{
	  respond_to_Console("Too many parameters passed to integral(f/g/h/i, <left>, <right>)", cmdline);
	  return;
	}
      i = 0;
      while (szparamlist[i] != ',')
	i++;
      strcpy(temp1, szparamlist);
      strcpy(temp2, szparamlist);
      temp1[i] = '\0';
      strcpy(szparamlist, temp2 + i + 1);
      if (strlen(temp1) != 1 || temp1[0] < 102 || temp1[0] > 105)
	{
	  strcpy(temp2, "invalid function - ");
	  appendString(temp2, temp1);
	  respond_to_Console(temp2, cmdline);
	  return;
	}
      j = temp1[0] - 102;
      if (!fns[j].isvalid)
	{
	  temp1[1] = '\0';
	  appendString(temp1, "(x) is not valid.");
	  respond_to_Console(temp1, cmdline);
	  return;
	}
      i = 0;
      while (szparamlist[i] != ',')
	i++;
      strcpy(temp2, szparamlist + i + 1);
      strcpy(temp1, szparamlist);
      temp1[i] = '\0';
      record_Error = RECORD_NO_ERROR;
      record_expression(temp1, &tempTree);
      report_record_error(temp1, cmdline);
      evaluate_Error = EVALUATE_NO_ERROR;
      tempval = evaluate_expression(&tempTree);
      freeKnot(&tempTree);
      record_Error = RECORD_NO_ERROR;
      record_expression(temp2, &tempTree);
      report_record_error(temp2, cmdline);
      tempval2 = evaluate_expression(&tempTree);
      freeKnot(&tempTree);
      if (evaluate_Error != EVALUATE_NO_ERROR)
	{
	  respond_to_Console("Expression is not defined.", cmdline);
	  return;
	}
      redrawfns();
      definite_integral(tempval, tempval2, j, cmdline);
    }
  if (strcmp(szcmd, "texit") == 0)
    {
      i = 0; j = 0;
      while (szparamlist[i] != '\0')
	{
	  if (szparamlist[i] == ',')
	    j++;
	  i++;
	}
      if (j > 1)
	{
	  respond_to_Console("Too many parameters passed to texit(<f/g/h/i>, <filename.tex>)", cmdline);
	  return;
	}
      if (j < 1)
	{
	  respond_to_Console("Too few parameters passed to texit(<f/g/h/i>, <filename.tex>)", cmdline);
	  return;
	}
      i = 0;
      while (szparamlist[i] != ',')
	i++;
      strcpy(temp2, szparamlist + i + 1);
      strcpy(temp1, szparamlist);
      temp1[i] = '\0';
      if (strlen(temp1) != 1 || temp1[0] < 102 || temp1[0] > 105)
	{
	  strcpy(temp2, "invalid function - ");
	  appendString(temp2, temp1);
	  respond_to_Console(temp2, cmdline);
	  return;
	}
      j = temp1[0] - 102;
      if (!fns[j].isvalid)
	{
	  temp1[1] = '\0';
	  strcpy(temp2, temp1);
	  appendString(temp2, "(x) is not valid.");
	  respond_to_Console(temp2, cmdline);
	  return;
	}
      FILE* texf = fopen(temp2, "wt");
      if (texf == 0)
	{
	  sprintf(temp1, "Couldn't open %s for writing!", temp2);
	  respond_to_Console(temp1, cmdline);
	  return;
	}
      fprintf(texf, "\\documentclass[fleqn,landscape,a4paper]{article}\n");
      fprintf(texf, "\\begin{document}\n");
      fprintf(texf, "\\setlength{\\mathindent}{1mm}\n");
      fprintf(texf, "\\[f(x)=");
      export_expression_latex(&fns[j].parseTreeRoot, texf);
      fprintf(texf, "\\]\n");
      fprintf(texf, "\\[f'(x)=");
      export_expression_latex(&fns[j].parseTreeRoot_slope, texf);
      fprintf(texf, "\\]\n");
      fprintf(texf, "\\[f''(x)=");
      export_expression_latex(&fns[j].parseTreeRoot_concavity, texf);
      fprintf(texf, "\\]\n");
      fprintf(texf, "\\LaTeX\\ representation created with FuncyFX \\\\");
      fprintf(texf, "\\copyright\\ Felix Natter, check out my homepage @ www.ndh.net/home/natter/");
      fprintf(texf, "\\end{document}\n");
      fclose(texf);
      sprintf(temp1, "LaTeX Representation of %c(x) written to %s.", szparamlist[0], temp2);
      respond_to_Console(temp1, cmdline);
    }
  if (strcmp(szcmd, "range") == 0)
    {
      i = 0; j = 0;
      while (szparamlist[i] != '\0')
	{
	  if (szparamlist[i] == ',')
	    j++;
	  i++;
	}
      if (j < 3)
	{
	  respond_to_Console("Too few parameters passed to range(<left>, <right>, <bottom>, <top>)", cmdline);
	  return;
	}
      if (j > 3)
	{
	  respond_to_Console("Too many parameters passed to range(<left>, <right>, <bottom>, <top>)", cmdline);
	  return;
	}
      strcpy(temp2, szparamlist);
      strcpy(temp1, temp2);
      tempTree.Parent = 0;
      i = 0;
      while (temp2[i] != ',')
	i++;
      strcpy(temp1, temp2 + i + 1); // remove 1st parameter
      temp2[i] = '\0';
      record_expression(temp2, &tempTree);
      VARIABLE_X = MINX;
      MINX = evaluate_expression(&tempTree);
      freeKnot(&tempTree);
      strcpy(temp2, temp1); // temp2 = "right,bottom,top"
      i = 0;
      while (temp2[i] != ',')
	i++;
      strcpy(temp1, temp2 + i + 1); // remove 2nd parameter
      temp2[i] = '\0';
      record_expression(temp2, &tempTree);
      VARIABLE_X = MAXX;
      MAXX = evaluate_expression(&tempTree);
      freeKnot(&tempTree);
      strcpy(temp2, temp1); // temp2 = "bottom,top"
      i = 0;
      while (temp2[i] != ',')
	i++;
      strcpy(temp1, temp2 + i + 1); // remove 3rd parameter
      temp2[i] = '\0';
      record_expression(temp2, &tempTree);
      VARIABLE_X = MINY;
      MINY = evaluate_expression(&tempTree);
      freeKnot(&tempTree);
      record_expression(temp1, &tempTree);
      VARIABLE_X = MAXY;
      MAXY = evaluate_expression(&tempTree);
      freeKnot(&tempTree);
      redrawfns();
      respond_to_Console("Range is changed.", cmdline);
      return;
    }
  if (strcmp(szcmd, "set") == 0)
    {
      i = 0;
      while (szparamlist[i] != ',' && szparamlist[i] != '\0')
	i++;
      if (szparamlist[i] == '\0')
	{
	  strcpy(temp1, "Too few parameters in set command - ");
	  appendString(temp1, szparamlist);
	  respond_to_Console(temp1, cmdline);
	  return;
	}
      strcpy(temp2, szparamlist + i + 1);
      strcpy(temp1, szparamlist);
      temp1[i] = '\0';
      if (strcmp(temp1, "f") == 0)
	i = 0;
      else
	if (strcmp(temp1, "g") == 0)
	  i = 1;
	else
	  if (strcmp(temp1, "h") == 0)
            i = 2;
	  else
	    if (strcmp(temp1, "i") == 0)
	      i = 3;
	    else
	      {
		strcpy(temp2, "Unknown function - ");
		appendString(temp2, temp1);
		respond_to_Console(temp2, cmdline);
		return;
	      }
      strcpy(fns[i].szFunction, temp2);
      record_Error = RECORD_NO_ERROR;
      freeKnot(&fns[i].parseTreeRoot);
      freeKnot(&fns[i].parseTreeRoot_slope);
      freeKnot(&fns[i].parseTreeRoot_concavity);
      recordFunc(fns[i]);
      if (record_Error != RECORD_NO_ERROR)
	{
	  fns[i].isvalid = false;
	  fns[i].fVisible = false; // recordFunc has set it to true
	  freeKnot(&fns[i].parseTreeRoot);
	  report_record_error(fns[i].szFunction, cmdline);
	  return;
	}
      else
	{
	  simplify_expression(&fns[i].parseTreeRoot);
	  redrawfns();
	  knot2expr(&fns[i].parseTreeRoot, fns[i].szFunction);
	  temp1[1] = '\0';
	  appendString(temp1, "(x) = ");
	  appendString(temp1, fns[i].szFunction);
	  respond_to_Console(temp1, cmdline);
	  derive_Error = DERIVE_NO_ERROR;
	  derive_expression(&fns[i].parseTreeRoot, &fns[i].parseTreeRoot_slope);
	  if (derive_Error == DERIVE_NO_ERROR)
	    {
	      set_parse_tree_parents(&fns[i].parseTreeRoot_slope);
	      knot2expr(&fns[i].parseTreeRoot_slope, fns[i].sz1stDerivative);
	      simplify_expression(&fns[i].parseTreeRoot_slope);
	      derive_expression(&fns[i].parseTreeRoot_slope, &fns[i].parseTreeRoot_concavity);
	      if (derive_Error == DERIVE_NO_ERROR)
		{
		  fns[i].derivatives_valid = true;
		  set_parse_tree_parents(&fns[i].parseTreeRoot_concavity);
		  temp1[1] = '\0';
		  appendString(temp1, "'(x) = ");
		  appendString(temp1, fns[i].sz1stDerivative);
		  knot2expr(&fns[i].parseTreeRoot_slope, fns[i].sz1stDerivative);
		  appendString(temp1, " = ");
		  appendString(temp1, fns[i].sz1stDerivative);
		  respond_to_Console(temp1, cmdline);
		  temp1[1] = '\0';
		  appendString(temp1, "''(x) = ");
		  knot2expr(&fns[i].parseTreeRoot_concavity, fns[i].sz2ndDerivative);
		  appendString(temp1, fns[i].sz2ndDerivative);
		  simplify_expression(&fns[i].parseTreeRoot_concavity);
		  knot2expr(&fns[i].parseTreeRoot_concavity, fns[i].sz2ndDerivative);
		  appendString(temp1, " = ");
		  appendString(temp1, fns[i].sz2ndDerivative);
		  respond_to_Console(temp1, cmdline);
		}
	      else
		{
		  freeKnot(&fns[i].parseTreeRoot_concavity);
		  goto derive_error;
		}
	    }
	  else
	    {
	    derive_error:
	      fns[i].derive_Error = derive_Error;
	      freeKnot(&fns[i].parseTreeRoot_slope);
	      fns[i].derivatives_valid = false;
	      report_derive_error(fns[i].szFunction, cmdline);
	    }
	  return;
	}
    }
  if (strcmp(szcmd, "show") == 0 || strcmp(szcmd, "hide") == 0)
    {
      if (strcmp(szcmd, "show") == 0)
	truefalse = true;
      else
	truefalse = false;
      if (szparamlist[0] < 102 || szparamlist[0] > 105)
	{
	  strcpy(temp1, "invalid_function - ");
	  appendString(temp1, szparamlist);
	  respond_to_Console(temp1, cmdline);
	  return;
	}
      i = szparamlist[0] - 102;
      if (!fns[i].isvalid)
	{
	  console_Lines[*cmdline].szline[0] = szparamlist[0];
	  strcpy(temp1, szparamlist);
	  temp1[1] = '\0';
	  appendString(temp1, "(x) is invalid.");
	  respond_to_Console(temp1, cmdline);
	  return;
	}
      if (!fns[i].derivatives_valid && szparamlist[1] != '\0')
	{// a derivative is wanted, but couldn't be created
	  strcpy(temp2, szparamlist);
	  temp2[1] = '\0';
	  strcpy(temp1, "Derivatives of ");
	  appendString(temp1, temp2);
	  appendString(temp1, "(x) couldn't be created.");
	  respond_to_Console(temp1, cmdline);
	  return;
	}
      strcpy(temp1, szparamlist);
      if (szparamlist[1] == '\0')
	{// f/g/h/i(x)
	  fns[i].fVisible = truefalse;
	  appendString(temp1, "(x) is ");
	  if (truefalse)
	    appendString(temp1, "visible.");
	  else
	    appendString(temp1, "invisible.");
	  respond_to_Console(temp1, cmdline);
	  redrawfns();
	  return;
	}
      if (szparamlist[1] == 39 && szparamlist[2] == '\0')
	{// f'/g'/h'/i'(x)
	  fns[i].f_slopeVisible = truefalse;
	  appendString(temp1, "(x) is ");
	  if (truefalse)
	    appendString(temp1, "visible.");
	  else
	    appendString(temp1, "invisible.");
	  respond_to_Console(temp1, cmdline);
	  redrawfns();
	  return;
	}
      if (szparamlist[1] == 39 && szparamlist[2] == 39 && szparamlist[3] == '\0')
	{// f''/g''/h''/i''(x)
	  fns[i].f_concavityVisible = truefalse;
	  appendString(temp1, "(x) is ");
	  if (truefalse)
	    appendString(temp1, "visible.");
	  else
	    appendString(temp1, "invisible.");
	  respond_to_Console(temp1, cmdline);
	  redrawfns();
	  return;
	}
    }
  if (strcmp(szcmd, "analyze") == 0)
    {
      if (strlen(szparamlist) != 1 || szparamlist[0] < 102 || szparamlist[0] > 105)
	{
	  strcpy(temp2, "invalid function - ");
	  appendString(temp2, szparamlist);
	  respond_to_Console(temp2, cmdline);
	  return;
	}
      i = szparamlist[0] - 102;
      if (!fns[i].isvalid)
	{
	  temp1[0] = szparamlist[0];
	  temp1[1] = '\0';
	  appendString(temp1, "(x) is not valid.");
	  respond_to_Console(temp1, cmdline);
	  return;
	}
      if (!fns[i].derivatives_valid)
	{
	  derive_Error = fns[i].derive_Error;
	  report_derive_error(fns[i].szFunction, cmdline);
	  return;
	}
      analyze_fn(i, cmdline);
    }
  if (strcmp(szcmd, "intersect") == 0)
    {
      i = 0;
      while (szparamlist[i] != '\0' && szparamlist[i] != ',')
	i++;
      if (szparamlist[i] != ',')
	{
	  respond_to_Console("Enter 2nd function; example: intersect(f,g)", cmdline);
	  return;
	}
      strcpy(temp1, szparamlist);
      temp1[i] = '\0';
      strcpy(temp2, szparamlist + i + 1);
      if (temp1[0] < 102 || temp1[0] > 105)
	{
	  strcpy(temp2, "invalid_function - ");
	  appendString(temp2, temp1);
	  respond_to_Console(temp2, cmdline);
	  return;
	}
      if (temp2[0] < 102 || temp2[0] > 105)
	{
	  strcpy(temp1, "invalid_function - ");
	  appendString(temp1, temp2);
	  respond_to_Console(temp1, cmdline);
	  return;
	}
      i = temp1[0] - 102;
      j = temp2[0] - 102;
      if (!fns[i].isvalid || !fns[j].isvalid)
	{
	  if (!fns[i].isvalid)
	    {
	      temp1[1] = '\0';
	      appendString(temp1, "(x) is not valid.");
	      respond_to_Console(temp1, cmdline);
	    }
	  if (!fns[j].isvalid)
	    {
	      temp2[1] = '\0';
	      appendString(temp2, "(x) is not valid.");
	      respond_to_Console(temp2, cmdline);
	    }
	  return;
	}
      find_intersections(i, j, cmdline);
    }
};

void analyze_fn(int idx, int* cmdline)
{
  double x, dx, y, slope, concavity;
  int u, v, i, nZeros = 0, nTPs = 0, nExtrs = 0;
  char zeros[MAX_EXPR_LEN], tps[MAX_EXPR_LEN], extrs[MAX_EXPR_LEN], str[MAX_EXPR_LEN], temp[30];
  strcpy(str, "analysis of ");
  appendString(str, fns[idx].szFunction);
  appendString(str, " in [");
  floatstr((float)MINX, temp);
  appendString(str, temp);
  appendString(str, ";");
  floatstr((float)MAXX, temp);
  appendString(str, temp);
  appendString(str, "]:");
  respond_to_Console(str, cmdline);
  strcpy(zeros, "zeros {");
  strcpy(extrs, "extreme values {");
  strcpy(tps, "turning points {");
  dx = (MAXX - MINX) / (double)(SCAN_STEPS);
  x = MINX;
  int was_above_XAxis, is_above_XAxis, was_positive_Slope, is_positive_Slope, was_positive_Concavity, is_positive_Concavity;
  cur_func = &fns[idx];
  evaluate_Error = EVALUATE_NO_ERROR;
  y = funcy(x - dx);
  if (y > 0.0)
    was_above_XAxis = 1;
  else
    was_above_XAxis = 0;
  if (evaluate_Error != EVALUATE_NO_ERROR)
    was_above_XAxis = 0xFFFF;
  evaluate_Error = EVALUATE_NO_ERROR;
  slope = func_slope(x - dx);
  if (slope > 0.0)
    was_positive_Slope = 1;
  else
    was_positive_Slope = 0;
  if (evaluate_Error != EVALUATE_NO_ERROR)
    was_positive_Slope = 0xFFFF;
  evaluate_Error = EVALUATE_NO_ERROR;
  concavity = func_concavity(x - dx);
  if (concavity > 0.0)
    was_positive_Concavity = 1;
  else
    was_positive_Concavity = 0;
  if (evaluate_Error != EVALUATE_NO_ERROR)
    was_positive_Concavity = 0xFFFF;
  for (i = SCAN_STEPS; i > 0; i--)
    {
      evaluate_Error = EVALUATE_NO_ERROR;
      y = funcy(x);
      if (y > 0.0)
	is_above_XAxis = 1;
      else
	is_above_XAxis = 0;
      if (evaluate_Error != EVALUATE_NO_ERROR)
	was_above_XAxis = 0xFFFF;
      evaluate_Error = EVALUATE_NO_ERROR;
      slope = func_slope(x);
      if (slope > 0.0)
	is_positive_Slope = 1;
      else
	is_positive_Slope = 0;
      if (evaluate_Error != EVALUATE_NO_ERROR)
	was_positive_Slope = 0xFFFF;
      evaluate_Error = EVALUATE_NO_ERROR;
      concavity = func_concavity(x);
      if (concavity > 0.0)
	is_positive_Concavity = 1;
      else
	is_positive_Concavity = 0;
      if (evaluate_Error != EVALUATE_NO_ERROR)
	was_positive_Concavity = 0xFFFF;
      if ((is_above_XAxis != was_above_XAxis || fabs(y) < SMALL_NUMBER) && is_above_XAxis != 0xFFFF && was_above_XAxis != 0xFFFF && nZeros < MAX_STORAGE)
	{
	  nZeros++;// we know there's a zero in [x - dx; x]
	  appendString(zeros, " (");
	  y = funcy(x - (dx * 0.5));
	  map2screen(x - (dx * 0.5), y, &u, &v);
	  clipcircle(u, v, 3, NEAREST[zeroscolor]);
	  floatstr((float)(x - (dx * 0.5)), temp, 2);
	  appendString(zeros, temp);
	  appendString(zeros, "|");
	  floatstr((float)y, temp, 2);
	  appendString(zeros, temp);
	  appendString(zeros, "),");
	}
      if ((is_positive_Slope != was_positive_Slope || fabs(slope) < SMALL_NUMBER) && was_positive_Slope != 0xFFFF && is_positive_Slope != 0xFFFF && nExtrs < MAX_STORAGE)
	{
	  nExtrs++;// we know there's an local Extreme value in [x - dx; x]
	  y = funcy(x - (dx * 0.5));
	  map2screen(x - (dx * 0.5), y, &u, &v);
	  if (was_positive_Concavity != is_positive_Concavity || fabs(concavity) < SMALL_NUMBER)
	    {
	      appendString(extrs, " Vertex");
	      clipcircle(u, v, 3, NEAREST[vertexcolor]);
	    }
	  else
	    {
	      if (is_positive_Concavity)
		{
		  appendString(extrs, " Min");
		  clipcircle(u, v, 3, NEAREST[mincolor]);
		}
	      else
		{
		  appendString(extrs, " Max");
		  clipcircle(u, v, 3, NEAREST[maxcolor]);
		}
	    }
	  appendString(extrs, "(");
	  floatstr((float)(x - (dx * 0.5)), temp, 2);
	  appendString(extrs, temp);
	  appendString(extrs, "|");
	  floatstr((float)y, temp, 2);
	  appendString(extrs, temp);
	  appendString(extrs, "),");
	}
      if ((is_positive_Concavity != was_positive_Concavity || fabs(concavity) < SMALL_NUMBER) && was_positive_Concavity != 0xFFFF && is_positive_Concavity != 0xFFFF && nTPs < MAX_STORAGE)
	{
	  if (((func_concavity(x - dx) - concavity) / 1.0E-3) != 0.0)
	    {
	      nTPs++;// we know there's a turning point in [x - dx; x]
	      appendString(tps, " (");
	      y = funcy(x - (dx * 0.5));
	      map2screen(x - (dx * 0.5), y, &u, &v);
	      clipcircle(u, v, 3, NEAREST[tpcolor]);
	      floatstr((float)(x - (dx * 0.5)), temp, 2);
	      appendString(tps, temp);
	      appendString(tps, "|");
	      floatstr((float)y, temp, 2);
	      appendString(tps, temp);
	      appendString(tps, "),");
	    }
	}
      x += dx;
      was_above_XAxis = is_above_XAxis;
      was_positive_Slope = is_positive_Slope;
      was_positive_Concavity = is_positive_Concavity;
    }
  if (nZeros != 0)
    zeros[strlen(zeros) - 1] = '\0';
  if (nZeros == 0)
    appendString(zeros, " none");
  if (nZeros == MAX_STORAGE)
    appendString(zeros, " ...");
  if (nExtrs != 0)
    extrs[strlen(extrs) - 1] = '\0';
  if (nExtrs == 0)
    appendString(extrs, " none");
  if (nExtrs == MAX_STORAGE)
    appendString(extrs, " ...");
  if (nTPs != 0)
    tps[strlen(tps) - 1] = '\0';
  if (nTPs == 0)
    appendString(tps, " none");
  if (nTPs == MAX_STORAGE)
    appendString(tps, " ...");
  appendString(zeros, " } (");
  itoa(nZeros, temp, 10);
  appendString(zeros, temp);
  appendString(zeros, ")");
  appendString(extrs, " } (");
  itoa(nExtrs, temp, 10);
  appendString(extrs, temp);
  appendString(extrs, ")");
  appendString(tps, " } (");
  itoa(nTPs, temp, 10);
  appendString(tps, temp);
  appendString(tps, ")");
  respond_to_Console(zeros, cmdline);
  respond_to_Console(extrs, cmdline);
  respond_to_Console(tps, cmdline);
};

void find_intersections(int idx1, int idx2, int* cmdline)
{
  double x, dx, y1, y2;
  int su, sv, u, nIntersections = 0;
  char str[MAX_EXPR_LEN], temp[30];
  strcpy(str, "Intersections between ");
  u = strlen(str);
  str[u] = idx1 + 102;
  str[u + 1] = ',';
  str[u + 2] = idx2 + 102;
  str[u + 3] = '\0';
  appendString(str, " in [");
  floatstr((float)MINX, temp, 2);
  appendString(str, temp);
  appendString(str, ";");
  floatstr((float)MAXX, temp, 2);
  appendString(str, temp);
  appendString(str, "]: {");
  dx = (MAXX - MINX) / (double)(SCAN_STEPS);
  x = MINX;
  int f1_was_above_f2, f1_is_above_f2;
  cur_func = &fns[idx1];
  evaluate_Error = EVALUATE_NO_ERROR;
  y1 = funcy(x - dx);
  if (evaluate_Error != EVALUATE_NO_ERROR)
    {
      f1_was_above_f2 = 0xFFFF;
      goto start;
    }
  cur_func = &fns[idx2];
  evaluate_Error = EVALUATE_NO_ERROR;
  y2 = funcy(x - dx);
  if (evaluate_Error != EVALUATE_NO_ERROR)
    {
      f1_was_above_f2 = 0xFFFF;
      goto start;
    }
  if (y1 > y2)
    f1_was_above_f2 = 1;
  else
    f1_was_above_f2 = 0;
 start:
  for (u = SCAN_STEPS; u > 0; u--)
    {
      cur_func = &fns[idx1];
      evaluate_Error = EVALUATE_NO_ERROR;
      y1 = funcy(x);
      if (evaluate_Error != EVALUATE_NO_ERROR)
	goto next;
      cur_func = &fns[idx2];
      evaluate_Error = EVALUATE_NO_ERROR;
      y2 = funcy(x);
      if (evaluate_Error != EVALUATE_NO_ERROR)
	goto next;
      if (y1 > y2)
	f1_is_above_f2 = 1;
      else
	f1_is_above_f2 = 0;
      if ((f1_is_above_f2 != f1_was_above_f2 && f1_was_above_f2 != 0xFFFF && nIntersections < MAX_STORAGE) || y1 == y2)
	{
	  nIntersections++;
	  y1 = funcy(x - (dx * 0.5));
	  map2screen(x - (dx * 0.5), y1, &su, &sv);
	  clipcircle(su, sv, 3, NEAREST[intersectioncolor]);
	  appendString(str, " (");
	  floatstr((float)(x - (dx * 0.5)), temp, 2);
	  appendString(str, temp);
	  appendString(str, "|");
	  floatstr((float)y1, temp, 2);
	  appendString(str, temp);
	  appendString(str, "),");
	}
      f1_was_above_f2 = f1_is_above_f2;
      goto normal;
    next:
      f1_was_above_f2 = 0xFFFF;
    normal:
      x += dx;
    }
  if (nIntersections != 0)
    str[strlen(str) - 1] = '\0'; // cut off comma
  if (nIntersections == MAX_STORAGE)
    appendString(str, " ...");
  appendString(str, " } (");
  itoa(nIntersections, temp, 10);
  appendString(str, temp);
  appendString(str, ")");
  respond_to_Console(str, cmdline);
};

void definite_integral(double left, double right, int idx, int* cmdline)
{
  cur_func = &fns[idx];
  int i, u, v, y_axis_v;
  char temp[100];
  double integ = 0.0, dx, x, y, prev_y, trapezoid;
  dx = (right - left) / (double)SCAN_STEPS;
  evaluate_Error = EVALUATE_NO_ERROR;
  map_y_on_screen(0.0, &y_axis_v);
  x = left;
  prev_y = funcy(x);
  map2screen(x, prev_y, &u, &v);
  if (prev_y > 0.0)
    v++;
  else
    v--;
  if (evaluate_Error == EVALUATE_NO_ERROR)
    clipline(u, y_axis_v, u, v, NEAREST[integralcolor]);
  x += dx;
  for (i = SCAN_STEPS; i > 0; i--)
    { // using trapezoidal rule
      y = funcy(x);
      map2screen(x, y, &u, &v);
      if (y > 0.0)
	v++;
      else
	v--;
      if (evaluate_Error == EVALUATE_NO_ERROR)
	clipline(u, y_axis_v, u, v, NEAREST[integralcolor]);
      trapezoid = (prev_y + y) * dx / 2.0;
      integ += trapezoid;
      x += dx;
      prev_y = y;
    }
  if (evaluate_Error != EVALUATE_NO_ERROR)
    {
      sprintf(temp, "Function is not defined somewhere in [%lG;%lG].", left, right);
      respond_to_Console(temp, cmdline);
      return;
    }
  strcpy(temp, "The definite integral of ");
  i = strlen(temp);
  temp[i] = idx + 102;
  sprintf(temp + i + 1, " in [%lG;%lG] = %lG", left, right, integ);
  respond_to_Console(temp, cmdline);
};

int main(int nargs, char** args)
{
  FILE* config = fopen("fx.cfg", "rb");
  if (config == 0)
    {
      printf("fx.cfg not found.\n");
      return 1;
    }
  int i, j, k;
  fscanf(config, "res: %d %d\n", &i, &j);
  fclose(config);
  char* pal;
  FILE* cpic = fopen("console.spr", "rb");
  if (cpic == 0)
    {
      printf("Couldn't find console.spr.");
      return 1;
    }
  pal = (char*)xmalloc(0x300);
  fread(pal, 1, 0x300, cpic);
  unsigned long lastoffs, size;
  fgetpos(cpic, &lastoffs);
  fseek(cpic, 0, SEEK_END);
  fgetpos(cpic, &size);
  size -= 0x300;
  console_pic = (char*)xmalloc(size);
  fsetpos(cpic, &lastoffs);
  fread(console_pic, 1, size, cpic);
  fclose(cpic);
  pal[0] = pal[1] = pal[2] = 0x00;
  if (!openVBE(i, j))
    {
      printf("Couldn't enter VBE mode (%i x %i).\n", i, j);
      return 1;
    }
  maxdrawx = xres - 1;
  maxdrawy = yres - 1;
  console_Height = yres / 3;
  console_Top = yres - console_Height - 1;
  bgpic = Imageblock(xres, yres);
  temppic = Imageblock(xres, console_Height);
  char* temppic1 = Imageblock(xres, yres - console_Height - 1);
  setpal(pal);
  updatecolors();
  xfree(pal);
  scaleimage(&console_pic, xres, console_Height);
  // find out the frame width (x and y)
  putimage(0, console_Top, console_pic);
  i = 10; //x
  CONSOLE_FRAME_WIDTH_X = 0;
  while (!CONSOLE_FRAME_WIDTH_X)
    {
      k = 1;
      for (j = console_Top + console_Height / 4; j < yres - console_Height / 4; j++)//y
	{ // scan one half of the height to find the place where the frame is finished
	  if (getpixel(i, j) != 0)
	    k = 0;
	}
      if (k == 1)
	CONSOLE_FRAME_WIDTH_X = i;
      i++;
    }
  i = 10; //y
  CONSOLE_FRAME_WIDTH_Y = 0;
  while (!CONSOLE_FRAME_WIDTH_Y)
    {
      k = 1;
      for (j = xres / 4; j < (3 * xres) / 4; j++)//x
	{ // scan one half of the width to find the place where the frame is finished
	  if (getpixel(j, i + console_Top) != 0)
	    k = 0;
	}
      if (k == 1)
	CONSOLE_FRAME_WIDTH_Y = i;
      i++;
    }
  CONSOLE_FRAME_WIDTH_Y += 5;
  settextsize(CONSOLE_FONT_WIDTH, CONSOLE_FONT_HEIGHT);
  CharsPerLine = (xres - 2*CONSOLE_FRAME_WIDTH_X) / CONSOLE_FONT_WIDTH;
  VisibleConsoleLines = (console_Height - 2*CONSOLE_FRAME_WIDTH_Y) / CONSOLE_CELL_HEIGHT;
  for (i = 0; i < 4; i++)
    {
      fns[i].isvalid = false;
      fns[i].parseTreeRoot.Children[0] = 0;
      fns[i].parseTreeRoot.Parent = 0;
      fns[i].parseTreeRoot_slope.Children[0] = 0;
      fns[i].parseTreeRoot_slope.Parent = 0;
      fns[i].parseTreeRoot_concavity.Children[0] = 0;
      fns[i].parseTreeRoot_concavity.Parent = 0;
      for (j = 0; j < xres; j++)
	{
	  fns[i].is_infinite[j] = false;
	  fns[i].is_infinite_slope[j] = false;
	  fns[i].is_infinite_concavity[j] = false;
	}
    }
  for (i = 0; i < MAX_CONSOLE_LINES; i++)
    {
      console_Lines[i].szline[0] = '\0';
      for (j = 1; j < MAX_CONSOLE_LINE_LEN; j++)
	console_Lines[i].szline[j] = ' ';
      console_Lines[i].editable = true;
    }
  installkeyhandler();
  memcpy(temppic, console_pic, (xres * console_Height) + 4);
  changeCharacter(42, 0, 0, 0,  8 | 16, 8 | 16, 0, 0, 0);// change '*'
  //changeCharacter(47, 2 | 1, 64 | 32 | 4 | 2 | 1, 64 | 32 | 8 | 4 | 2, 16 | 8 | 4, 32 | 16 | 8, 64 | 32 | 16 | 4 | 2, 128 | 64 | 32 | 4 | 2, 128 | 64);// change '/'
  //changeCharacter(47, 0, 64 | 32 | 2, 64 | 32 | 4, 8, 16, 32 | 4 | 2, 64 | 4 | 2, 0);// change '/'
  while (!getkey(KEY_ESC))
    {
      if (peekkey(KEY_X))
	{
	  if (peekkey(KEY_PLUS))
	    {
	      MINX -= 0.25;
	      MAXX += 0.25;
	    }
	  if (peekkey(KEY_MINUS))
	    {
	      MINX += 0.25;
	      MAXX -= 0.25;
	    }
	}
      if (peekkey(KEY_Y))
	{
	  if (peekkey(KEY_PLUS))
	    {
	      MINY -= 0.25;
	      MAXY += 0.25;
	    }
	  if (peekkey(KEY_MINUS))
	    {
	      MINY += 0.25;
	      MAXY -= 0.25;
	    }
	}
      if (peekkey(KEY_PAD6))
	{
	  MAXX += 0.25;
	  MINX += 0.25;
	}
      if (peekkey(KEY_PAD4))
	{
	  MAXX -= 0.25;
	  MINX -= 0.25;
	}
      if (peekkey(KEY_PAD2))
	{
	  MAXY -= 0.25;
	  MINY -= 0.25;
	}
      if (peekkey(KEY_PAD8))
	{
	  MAXY += 0.25;
	  MINY += 0.25;
	}
      if (getkey(KEY_TAB))
	{
	  restorekeyhandler();
	  maxdrawy = yres - 1 - console_Height;
	  setClipRect(0, 0, maxdrawx, maxdrawy);
	  redrawfns();
	  getimage(0, 0, xres, yres - console_Height - 1, temppic1);
	  clearoffscreenmem();
	  j = yres - 2 - console_Top;
	  for (VARIABLE_X = 0.0; VARIABLE_X <= 1.0; VARIABLE_X += SCROLL_STEP)
	    {
	      i = xres - 2 - (int)((xres - 2) * VARIABLE_X);
	      clipputimage(i, 0, temppic1);
	      i = yres - 2 - (int)(j * VARIABLE_X);
	      clipputimage(0, i, temppic);
	      flipoffscreenmem();
	    }
	  putimage(0, 0, temppic1);
	  console();
	  maxdrawy = yres - 1; // reset
	  setClipRect(0, 0, maxdrawx, maxdrawy);
	  getimage(0, 0, xres, yres - console_Height - 1, temppic1);
	  getimage(0, console_Top, xres, console_Height, temppic);
	  clearoffscreenmem();
	  j = yres - 2 - console_Top;
	  for (VARIABLE_X = 0.0; VARIABLE_X <= 1.0; VARIABLE_X += SCROLL_STEP)
	    {
	      clearoffscreenmem();
	      i = console_Top + (int)(j * VARIABLE_X);
	      clipputimage(0, i, temppic);
	      i = (int)((xres - 1) * VARIABLE_X);
	      clipputimage(i, 0, temppic1);
	      flipoffscreenmem();
	    }
	  redrawfns();
	  getimage(0, 0, xres, yres, bgpic);
	  clearoffscreenmem();
	  for (VARIABLE_X = 0.0; VARIABLE_X <= 1.0; VARIABLE_X += SCROLL_STEP)
	    {
	      i = yres - 2 - (int)((yres - 2) * VARIABLE_X);
	      clipputimage(0, i, bgpic);
	      flipoffscreenmem();
	    }
	  clearallkeys();
	  installkeyhandler();
	}
      redrawfns();
      flipoffscreenmem();
    }
  restorekeyhandler();
  closeVBE();
  xfree(console_pic);
  xfree(bgpic);
  xfree(temppic);
  xfree(temppic1);
  freeKnot(&fns[0].parseTreeRoot);
  freeKnot(&fns[0].parseTreeRoot_slope);
  freeKnot(&fns[0].parseTreeRoot_concavity);
  freeKnot(&fns[1].parseTreeRoot);
  freeKnot(&fns[1].parseTreeRoot_slope);
  freeKnot(&fns[1].parseTreeRoot_concavity);
  freeKnot(&fns[2].parseTreeRoot);
  freeKnot(&fns[2].parseTreeRoot_slope);
  freeKnot(&fns[2].parseTreeRoot_concavity);
  freeKnot(&fns[3].parseTreeRoot);
  freeKnot(&fns[3].parseTreeRoot_slope);
  freeKnot(&fns[3].parseTreeRoot_concavity);
}
