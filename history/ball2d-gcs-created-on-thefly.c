/* xscreensaver, Copyright (c) 1992, 1995, 1996, 1997, 1998
 *  Jamie Zawinski <jwz@jwz.org>
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation.  No representations are made about the suitability of this
 * software for any purpose.  It is provided "as is" without express or 
 * implied warranty.
 */

/*
  If you want to understand the physics for ball collisions, just write to
  fnatter@gmx.net and I'll send you a pdf explaining this.
 */

#include <math.h>
#include "screenhack.h"
#include "alpha.h"
#ifdef HAVE_DOUBLE_BUFFER_EXTENSION
# include "xdbe.h"
#endif /* HAVE_DOUBLE_BUFFER_EXTENSION */

#define LOGGING 1

/*
  TODO:
  - dbuf + reshape (deluxe.c!)
  - better way of placing the balls
    - lattice (restrict masses to avoid huge balls!)
  - bind ncolors to count
  - honor aspect ratio of monitor
  - use delay parameter!
*/

/* typedefs / structs */
typedef double number;

typedef struct _Vector2D {
  number x;
  number y;
} Vector2D;

typedef struct _Ball {
  number x,y;
  number dx,dy;
  number mass;
  number radius;
  unsigned long color;
  double velocity;
  double way_to_go;
  int EOFiteration;
} Ball;

struct state {
  Display *dpy;
  Window window;

  Ball* balls;

  Bool dbuf;
  int delay;
  int count;
  int ncolors;
  Bool onedee; /* 1D */
  Bool showVelocityVectors;
  int* startAngles;
  int numStartAngles;
  Bool tiny;
  Bool startGrid;
  Bool slow;

  XColor *colors;
  GC erase_gc;
  XWindowAttributes xgwa;
  Pixmap b, ba, bb;	/* double-buffer to reduce flicker */

# ifdef HAVE_DOUBLE_BUFFER_EXTENSION
  Bool dbeclear_p;
  XdbeBackBuffer backb;
# endif /* HAVE_DOUBLE_BUFFER_EXTENSION */
};


/* constants */
const number MINX = -100.0;
const number MAXX = 100.0;
const number MINY = -100.0;
const number MAXY = 100.0;
#define ANGLES_ALL (NULL)

/* was: MAX_RADIUS = 6.0 */ /* was: SINGLE_STEP = 0.25, SEPARATE_STEP=0.01, MAX_MASS=2.0, MIN_MASS=0.1 */
const double MIN_RADIUS = 1.0, MAX_RADIUS = 10.0; 
double MIN_VELOCITY = 0.0, MAX_VELOCITY = 2.0, MIN_MASS = 0.5, MAX_MASS = 2.0, SINGLE_STEP = 0.1, SEPARATE_STEP = 0.01;
#define MAX_MOMENTUM (MAX_VELOCITY * MAX_MASS)
const double PERCENT_AFTER_MOVE = 1.0, PERCENT_AFTER_WALL_HIT = 1.0; /* was: 0.9999 */

static number
new_number_random (number min, number max) {
  return ( (max-min)*((number)random()/(number)RAND_MAX) ) + min;
}

static int
new_number_random_int (int min, int max) {
  return (int)( (max-min+1)*((number)random()/(number)RAND_MAX) ) + min;
}

/* Vector class */
static Vector2D
new_vector_random (number min_x, number max_x,
                   number min_y, number max_y) {
  Vector2D ret;
  ret.x = new_number_random (min_x, max_x);
  ret.y = new_number_random (min_y, max_y);
  return ret;
}

static number
Vector2D_length(Vector2D* vec)
{
  return sqrt(vec->x*vec->x + vec->y*vec->y);
}

static void
Vector2D_normalize(Vector2D* vec)
{
  number len = Vector2D_length(vec);
  vec->x /= len;
  vec->y /= len;
}

/*
static void
Vector2D_negate(Vector2D* vec)
{
  vec->x = -vec->x;
  vec->y = -vec->y;
}

static Vector2D
Vector2D_add(Vector2D* vec1, Vector2D* vec2)
{
  Vector2D sum;
  sum.x = vec1->x + vec2->x;
  sum.y = vec1->y + vec2->y;
  return sum;
}
*/

static Vector2D
Vector2D_sub(Vector2D* vec1, Vector2D* vec2)
{
  Vector2D diff;
  diff.x = vec1->x - vec2->x;
  diff.y = vec1->y - vec2->y;
  return diff;
}

static Vector2D
Vector2D_multScalar(Vector2D* vec, number scalar)
{
  Vector2D ret;
  ret.x = vec->x * scalar;
  ret.y = vec->y * scalar;
  return ret;
}

static number
Vector2D_scalarProduct(Vector2D* vec1, Vector2D* vec2)
{
  return vec1->x * vec2->x + vec1->y * vec2->y;
}

static number
Vector2D_scalarProjectionOfVec1OntoVec2(Vector2D* vec1, Vector2D* vec2)
{
  return Vector2D_scalarProduct(vec1, vec2) / Vector2D_length(vec2);
}

static void
map_length(struct state* st, double wld, int* screen)
{
  wld /= (MAXY-MINY);
  /* TODO: scale with mean(width,height)? */
  wld *= (st->xgwa.height-1);
  *screen = (int)(wld + 0.5);
}

static void
map2screen(struct state* st, double wldx, double wldy, int* scrx, int* scry)
{
  wldx -= MINX;
  wldx /= (MAXX-MINX);
  wldx *= (st->xgwa.width-1);

  wldy -= MINY;
  wldy /= (MAXY-MINY);
  wldy *= (st->xgwa.height-1);

  *scrx = (int)(wldx + 0.5);
  *scry = (int)(wldy + 0.5);

  /* 
     deviceX = (int)((ball->position.x-MINX)/(MAXX-MINX) * st->xgwa.width);
     deviceY = (int)((ball->position.y-MINY)/(MAXY-MINY) * st->xgwa.height);
  */
}


/* Ball class */

static void
Ball_init(struct state* st, Ball* ball)
{ 
  number vel;
  int angle;

  if (st->tiny) /* tiny mode: 95% small balls */
    {
      int tinyProb = new_number_random_int(1, 100);
      if (tinyProb < 95)
        ball->mass = new_number_random(MIN_MASS, MIN_MASS + 0.1 * (MAX_MASS-MIN_MASS));
      else /* insert a few large balls */
        ball->mass = new_number_random(MIN_MASS + 0.9*(MAX_MASS-MIN_MASS), MAX_MASS);
    }
  else
    {
      ball->mass = new_number_random(MIN_MASS, MAX_MASS);
    }

  ball->radius = ((MAX_RADIUS - MIN_RADIUS) * (ball->mass - MIN_MASS) / MAX_MASS) + MIN_RADIUS;
  /* TODO: cache mapped radius?
     map_length(wld_radius, &radius); */

  if (st->startAngles == ANGLES_ALL)
    {
      ball->dx = new_number_random(-1.0, 1.0);
      if (st->onedee)
        ball->dy = 0.0;
      else
        ball->dy = new_number_random(-1.0, 1.0);
    }
  else
    {
      int whichAngle;

      whichAngle = new_number_random_int(0, st->numStartAngles - 1);
      /* printf("whichAngle=%d\n", whichAngle); */
      angle = st->startAngles[whichAngle]; 
      ball->dx = cos(M_PI/180.0 * angle);
      ball->dy = sin(M_PI/180.0 * angle);
    }

  /* normalize (dx,dy)-Vector */
  vel = sqrt(ball->dx*ball->dx + ball->dy*ball->dy);
  ball->dx /= vel;
  ball->dy /= vel;

  ball->velocity = new_number_random(MIN_VELOCITY, MAX_VELOCITY);
}

static void
Ball_randomizePosition (struct state* st, Ball* ball)
{
  Vector2D position;
  position = new_vector_random (MINX + ball->radius, MAXX - ball->radius,
                                MINY + ball->radius, MAXY - ball->radius);
  ball->x = position.x;
  if (st->onedee)
    ball->y = 0.0;
  else
    ball->y = position.y;
}

static int
Ball_collision_check(Ball* b1, Ball* b2)
{
  double r1pr2, dx, dy;
  r1pr2 = b1->radius + b2->radius;
  dx = b2->x - b1->x;
  dy = b2->y - b1->y;
  if (dx*dx + dy*dy <= r1pr2*r1pr2)
    return 1;
  else
    return 0;
}

static void
Ball_draw (struct state* st, Drawable w, Ball *ball, int ballIdx)
{
  XGCValues gcv;
  unsigned long flags;
  GC gc;
  int deviceX, deviceY, radius;

  map_length(st, ball->radius, &radius);
  map2screen(st, ball->x, ball->y,
             &deviceX, &deviceY);

  flags = GCForeground;
  gcv.cap_style = CapProjecting;
  gcv.join_style = JoinMiter;
  gcv.foreground = st->colors[ballIdx % st->ncolors].pixel;
  flags |= (GCLineWidth | GCCapStyle | GCJoinStyle);
  gc = XCreateGC (st->dpy, w, flags, &gcv);

#ifdef LOGGING
  printf("st->ncolors=%d\n", st->ncolors);
#endif

  XFillArc (st->dpy, w, gc,
            deviceX - radius,
            deviceY - radius,
            radius*2, radius*2,
            0, 360*64);

  if (st->showVelocityVectors) 
    {
      int velocityVectorX, velocityVectorY;
      Vector2D velocityVector;

      velocityVector.x = ball->dx;
      velocityVector.y = ball->dy;
      Vector2D_multScalar(&velocityVector, ball->radius / MAX_VELOCITY);

      velocityVector.x += ball->x;
      velocityVector.y += ball->y;

      map2screen(st, velocityVector.x, velocityVector.y,
                 &velocityVectorX, &velocityVectorY);

      gcv.foreground = ~0L;
      XChangeGC(st->dpy, gc, flags, &gcv);
      XDrawLine(st->dpy, w, gc,
                deviceX, deviceY,
                velocityVectorX, velocityVectorY);
    }

  XFreeGC(st->dpy, gc);
}

static void *
ball2d_init (Display *dpy, Window window)
{
  struct state *st = (struct state *) calloc (1, sizeof(*st));
  XGCValues gcv;
  int i, j;
  Bool conflict;
  number max_radius;
  char* startAngles;

  st->dpy = dpy;
  st->window = window;
  st->count = get_integer_resource (st->dpy, "count", "Integer");
  st->ncolors = get_integer_resource (st->dpy, "ncolors", "Integer");
  st->delay = get_integer_resource (st->dpy, "delay", "Integer");
  st->dbuf = get_boolean_resource (st->dpy, "doubleBuffer", "Boolean");
  st->onedee = get_boolean_resource(st->dpy, "1D", "Boolean");
  st->showVelocityVectors = get_boolean_resource(st->dpy, "showVelocityVectors", "Boolean");
  startAngles = get_string_resource(st->dpy, "startAngles", "String");
  if (strcasecmp(startAngles, "all") == 0)
    {
      st->startAngles = ANGLES_ALL;
    }
  else if (strcasecmp(startAngles, "right") == 0)
    {
      st->numStartAngles = 4;
      st->startAngles = (int*)calloc(sizeof(int), 4);
      st->startAngles[0] = 0;
      st->startAngles[1] = 90;
      st->startAngles[2] = 180;
      st->startAngles[3] = 270;
    }
  else
    {
      char* token;

      st->numStartAngles = 1;
      for (i = 0; i < strlen(startAngles); i++)
        {
          if (startAngles[i] == ',')
            st->numStartAngles++;
        }
      st->startAngles = (int*)calloc(sizeof(int), st->numStartAngles);
      i = 0;
      token = strtok(startAngles, ",");
      do
        {
          /* printf("token='%s'\n", token); */
          st->startAngles[i++] = atoi(token);
        }
      while ((token = strtok(NULL, ",")) != NULL);
    }
  /*
  for (i = 0; i < st->numStartAngles; i++)
    {
      printf("startAngle[%d] = %d\n", i, st->startAngles[i]);
    }
  exit(0);
  */

  st->tiny = get_boolean_resource(st->dpy, "tiny", "Boolean");
  st->slow = get_boolean_resource(st->dpy, "slow", "Boolean");
  st->startGrid = get_boolean_resource(st->dpy, "startGrid", "Boolean");

  if (st->slow)
    {
      SINGLE_STEP /= 100.0;
    }

# ifdef HAVE_DOUBLE_BUFFER_EXTENSION
  st->dbeclear_p = get_boolean_resource (st->dpy, "useDBEClear", "Boolean");
#endif

# ifdef HAVE_COCOA	/* Don't second-guess Quartz's double-buffering */
  st->dbuf = False;
# endif

  XGetWindowAttributes (st->dpy, st->window, &st->xgwa);

  /* st->transparent_p = get_boolean_resource(st->dpy, "transparent", "Transparent"); */

  st->colors = (XColor *) calloc (sizeof(*st->colors), st->ncolors);

  if (get_boolean_resource(st->dpy, "mono", "Boolean"))
    {
    MONO:
      st->ncolors = 1;
      st->colors[0].pixel = get_pixel_resource(st->dpy, st->xgwa.colormap,
                                           "foreground", "Foreground");
    }
  else
    {
      make_random_colormap (st->dpy, st->xgwa.visual, st->xgwa.colormap,
                            st->colors, &st->ncolors, True, True, 0, True);
      if (st->ncolors < 2)
        goto MONO;
    }

  if (st->dbuf)
    {
#ifdef HAVE_DOUBLE_BUFFER_EXTENSION
      if (st->dbeclear_p)
        st->b = xdbe_get_backbuffer (st->dpy, st->window, XdbeBackground);
      else
        st->b = xdbe_get_backbuffer (st->dpy, st->window, XdbeUndefined);
      st->backb = st->b;
#endif /* HAVE_DOUBLE_BUFFER_EXTENSION */

      if (!st->b)
        {
          st->ba = XCreatePixmap (st->dpy, st->window, st->xgwa.width, st->xgwa.height,st->xgwa.depth);
          st->bb = XCreatePixmap (st->dpy, st->window, st->xgwa.width, st->xgwa.height,st->xgwa.depth);
          st->b = st->ba;
        }
    }
  else
    {
      st->b = st->window;
    }

  st->balls = (Ball*) calloc(st->count, sizeof(Ball));
  max_radius = -100.0;
  for (i = 0; i < st->count; i++)
    { 
      Ball_init(st, &st->balls[i]);
      if (st->balls[i].radius > max_radius)
        max_radius = st->balls[i].radius;
    }


#ifdef LOGGING
  printf("ball2d_init()\n");
#endif

  if (st->startGrid)
    {
      number gridWidth, gridHeight, x, y;

      gridWidth = max_radius*2 + 5.0;
      gridHeight = max_radius*2 + 5.0;
      
      x = MINX + max_radius + 5;
      y = MINY + max_radius + 5;
      for (i = 0; i < st->count; i++)
        {
          st->balls[i].x = x;
          st->balls[i].y = y;

          x += gridWidth; 
          if (x >= MAXX - max_radius)
            {
              x = MINX + max_radius + 5;
              y += gridHeight;
            }
        }
    }
  else
    {
      Ball_randomizePosition(st, &st->balls[0]);
      for (i = 1; i < st->count; i++)
        {
          conflict = True;
          while (conflict)
            {
              Ball_randomizePosition(st, &st->balls[i]);
              conflict = False;
              for (j = 0; j < i; j++)
                {
                  if (Ball_collision_check(st->balls + i, st->balls + j))
                    conflict = True;
                }
            }
        }
    }

  gcv.foreground = get_pixel_resource (st->dpy, st->xgwa.colormap,
                                       "background", "Background");
  st->erase_gc = XCreateGC (st->dpy, st->b, GCForeground, &gcv);

  if (st->ba) XFillRectangle (st->dpy, st->ba, st->erase_gc, 0, 0, st->xgwa.width, st->xgwa.height);
  if (st->bb) XFillRectangle (st->dpy, st->bb, st->erase_gc, 0, 0, st->xgwa.width, st->xgwa.height);

  return st;
}

static void
Ball_doCollision(Ball* b1, Ball* b2)
{
  double move1, move2;
  Vector2D position_b1, position_b2, line_of_sight, b1_velocity_before, b2_velocity_before, b1_velocity_after, b2_velocity_after;
  double b1_velocity_in_line_of_sight, b2_velocity_in_line_of_sight, b1_velocity_in_line_of_sight_before, b2_velocity_in_line_of_sight_before, b1_change_in_line_of_sight_velocity, b2_change_in_line_of_sight_velocity;

#ifdef LOGGING
  printf("Ball_doCollision()\n");
#endif

  /* separate the balls so that they are non-overlapping */
  move1 = SEPARATE_STEP * b1->velocity / MAX_VELOCITY;
  move2 = SEPARATE_STEP * b2->velocity / MAX_VELOCITY;
  while (Ball_collision_check(b1, b2))
    {
      b1->x -= b1->dx * move1;
      b1->y -= b1->dy * move1;
      b2->x -= b2->dx * move2;
      b2->y -= b2->dy * move2;
    }
  
  position_b1.x = b1->x; position_b1.y = b1->y;
  position_b2.x = b2->x; position_b2.y = b2->y;
  line_of_sight = Vector2D_sub(&position_b2, &position_b1);
  /*
  if (show_details)
    show_vector(line_of_sight, b1->x, b1->y, NEAREST[CCWHITE]);
  */
  Vector2D_normalize(&line_of_sight);

  b1_velocity_before.x = b1->dx * b1->velocity; b1_velocity_before.y = b1->dy * b1->velocity;
  b2_velocity_before.x = b2->dx * b2->velocity; b2_velocity_before.y = b2->dy * b2->velocity;

  b1_velocity_in_line_of_sight_before = Vector2D_scalarProjectionOfVec1OntoVec2(&b1_velocity_before, &line_of_sight);
  b2_velocity_in_line_of_sight_before = Vector2D_scalarProjectionOfVec1OntoVec2(&b2_velocity_before, &line_of_sight);

  b1_velocity_in_line_of_sight = b1_velocity_in_line_of_sight_before * (b1->mass - b2->mass) / (b1->mass + b2->mass) + b2_velocity_in_line_of_sight_before * 2.0 * b2->mass / (b1->mass + b2->mass);
  b2_velocity_in_line_of_sight = b1_velocity_in_line_of_sight_before * 2.0 * b1->mass / (b1->mass + b2->mass) + b2_velocity_in_line_of_sight_before * (b2->mass - b1->mass) / (b2->mass + b1->mass);

  b1_change_in_line_of_sight_velocity = b1_velocity_in_line_of_sight - b1_velocity_in_line_of_sight_before;
  b2_change_in_line_of_sight_velocity = b2_velocity_in_line_of_sight - b2_velocity_in_line_of_sight_before;

  b1_velocity_after.x = b1_velocity_before.x + line_of_sight.x * b1_change_in_line_of_sight_velocity;
  b1_velocity_after.y = b1_velocity_before.y + line_of_sight.y * b1_change_in_line_of_sight_velocity;

  b2_velocity_after.x = b2_velocity_before.x + line_of_sight.x * b2_change_in_line_of_sight_velocity;
  b2_velocity_after.y = b2_velocity_before.y + line_of_sight.y * b2_change_in_line_of_sight_velocity;

  /*
  if (show_details)
    {
        //show_vector_in_ball(b1->scrx, b1->scry, b1->radius, from_b1_to_b2 * b1_velocity_in_line_of_sight_before, NEAREST[CCRED]);
        //show_vector_in_ball(b1->scrx, b1->scry, b1->radius, from_b1_to_b2 * b1_velocity_in_line_of_sight, NEAREST[CCGREEN]);
        //show_vector_in_ball(b2->scrx, b2->scry, b2->radius, from_b2_to_b1 * b2_velocity_in_line_of_sight_before, NEAREST[CCRED]);
        //show_vector_in_ball(b2->scrx, b2->scry, b2->radius, from_b2_to_b1 * b2_velocity_in_line_of_sight, NEAREST[CCGREEN]);
      char str[1000];
      showstrxy(b1->scrx - 4, b1->scry - 4, "1", b1->contrastindex);// clipping!!!!!!
      showstrxy(b2->scrx - 4, b2->scry - 4, "2", b2->contrastindex);
      BallManager::map2screen(b1->x, b1->y, &b1->scrx, &b1->scry);
      BallManager::map2screen(b2->x, b2->y, &b2->scrx, &b2->scry);
      showstrxy(b1->scrx - 4, b1->scry - 4, "1", b1->contrastindex);// clipping!!!!!!
      showstrxy(b2->scrx - 4, b2->scry - 4, "2", b2->contrastindex);
      sprintf(str, "Masses: m1 = %G; m2 = %G", b1->mass, b2->mass);
      showstrxy(10, 20, str, NEAREST[CCWHITE]);
      sprintf(str, "Positions: b1 = (%G, %G); b2 = (%G, %G);", b1->x, b1->y, b2->x, b2->y);
      showstrxy(10, 30, str, NEAREST[CCWHITE]);
      sprintf(str, "Line of Sight = (%G, %G);", line_of_sight.x, line_of_sight.y);
      showstrxy(10, 40, str, NEAREST[CCWHITE]);
      sprintf(str, "Velocities before collision: vi1 = (%G, %G); vi2 = (%G, %G);", b1_velocity_before.x, b1_velocity_before.y, b2_velocity_before.x, b2_velocity_before.y);
      showstrxy(10, 50, str, NEAREST[CCWHITE]);
      sprintf(str, "Velocities before collision in line of sight: vi1' = %G; vi2' = %G", b1_velocity_in_line_of_sight_before, b2_velocity_in_line_of_sight_before);
      showstrxy(10, 60, str, NEAREST[CCWHITE]);
      sprintf(str, "Velocities after collision in line of sight: v1' = %G; v2' = %G", b1_velocity_in_line_of_sight, b2_velocity_in_line_of_sight);
      showstrxy(10, 70, str, NEAREST[CCWHITE]);
      sprintf(str, "Velocities after collision: v1 = (%G, %G); v2 = (%G, %G);", b1_velocity_after.x, b1_velocity_after.y, b2_velocity_after.x, b2_velocity_after.y);
      showstrxy(10, 80, str, NEAREST[CCWHITE]);
      flipoffscreenmem();
    }
*/

  b1->velocity = Vector2D_length(&b1_velocity_after);
  b2->velocity = Vector2D_length(&b2_velocity_after);
  Vector2D_normalize(&b1_velocity_after);
  Vector2D_normalize(&b2_velocity_after);
  b1->dx = b1_velocity_after.x;
  b1->dy = b1_velocity_after.y;
  b2->dx = b2_velocity_after.x;
  b2->dy = b2_velocity_after.y;
}

static void
Balls_move(struct state* st)
{
  int i, j, nBalls;
  Ball* balls;
  Bool reiterate;
  double thisway;
  Ball* cball;

#ifdef LOGGING
  printf("Balls_move()\n");
#endif

  balls = st->balls;
  nBalls = st->count;

  /* return; */

  for (i = 0; i < nBalls; i++)
    {
      balls[i].way_to_go = balls[i].velocity;
      balls[i].EOFiteration = 0;
    }
  do
    {
      for (i = 0; i < nBalls; i++)
        {
          cball = balls + i;
          if (cball->way_to_go > SINGLE_STEP)
            thisway = SINGLE_STEP;
          else
            {
              thisway = cball->way_to_go;
              cball->EOFiteration = 1;
            }
          cball->way_to_go -= thisway;
          cball->x += cball->dx * thisway;
          cball->y += cball->dy * thisway;
#ifdef LOGGING
          printf("Balls[%d] = (%1.f,%1.f)\n", i, cball->x, cball->y);
#endif
          if (cball->x + cball->radius > MAXX)
            {
              cball->x = MAXX - cball->radius - SINGLE_STEP;
              cball->dx = -cball->dx;
              cball->velocity *= PERCENT_AFTER_WALL_HIT;
              cball->EOFiteration = 0;
            }
          if (cball->y + cball->radius > MAXY)
            {
              cball->y = MAXY - cball->radius - SINGLE_STEP;
              cball->dy = -cball->dy;
              cball->velocity *= PERCENT_AFTER_WALL_HIT;
              cball->EOFiteration = 0;
            }
          if (cball->x < MINX + cball->radius)
            {
              cball->x = MINX + cball->radius + SINGLE_STEP;
              cball->dx = -cball->dx;
              cball->velocity *= PERCENT_AFTER_WALL_HIT;
              cball->EOFiteration = 0;
            }
          if (cball->y < MINY + cball->radius)
            {
              cball->y = MINY + cball->radius + SINGLE_STEP;
              cball->dy = -cball->dy;
              cball->velocity *= PERCENT_AFTER_WALL_HIT;
              cball->EOFiteration = 0;
            }
          /* check for collisions */
          for (j = 0; j < i; j++)
            {
              if (Ball_collision_check(cball, balls + j))
                {
                  Ball_doCollision(cball, balls + j);
                  /* reset way_to_go */
                  cball->way_to_go = cball->velocity;
                  balls[j].way_to_go = balls[j].velocity;
                  cball->EOFiteration = 0;
                }
            }
        }
      reiterate = False;
      for (i = 0; i < nBalls; i++)
        if (!balls[i].EOFiteration)
          reiterate = True;
    } while (reiterate);
}

static unsigned long
ball2d_draw (Display *dpy, Window window, void *closure)
{
  int i;
  struct state *st = (struct state *) closure;

#ifdef HAVE_DOUBLE_BUFFER_EXTENSION
  if (!st->dbeclear_p || !st->backb)
#endif /* HAVE_DOUBLE_BUFFER_EXTENSION */
    XFillRectangle (st->dpy, st->b, st->erase_gc, 0, 0, st->xgwa.width, st->xgwa.height);
  
  Balls_move(st);
  for (i = 0; i < st->count; i++)
    {
      Ball_draw(st, st->b, &st->balls[i], i);
    }



#ifdef HAVE_DOUBLE_BUFFER_EXTENSION
  if (st->backb)
    {
      XdbeSwapInfo info[1];
      info[0].swap_window = st->window;
      info[0].swap_action = (st->dbeclear_p ? XdbeBackground : XdbeUndefined);
      XdbeSwapBuffers (st->dpy, info, 1);
    }
  else
#endif /* HAVE_DOUBLE_BUFFER_EXTENSION */
    if (st->dbuf)
      {
        XCopyArea (st->dpy, st->b, st->window, st->erase_gc, 0, 0,
                   st->xgwa.width, st->xgwa.height, 0, 0);
        st->b = (st->b == st->ba ? st->bb : st->ba);
      }

  return st->delay;
}

static void
ball2d_reshape (Display *dpy, Window window, void *closure, 
                 unsigned int w, unsigned int h)
{
  struct state *st = (struct state *) closure;
  if (! st->dbuf) {   /* #### more complicated if we have a back buffer... */
    XGetWindowAttributes (st->dpy, st->window, &st->xgwa);
    XClearWindow (dpy, window);
  }
}

static Bool
ball2d_event (Display *dpy, Window window, void *closure, XEvent *event)
{
  return False;
}

static void
ball2d_free (Display *dpy, Window window, void *closure)
{
  struct state *st = (struct state *) closure;
  if (st->startAngles != NULL)
    free(st->startAngles);
  free(st->balls);
  free(st->colors);
}


static const char *ball2d_defaults [] = {
  ".background:		black",
  ".foreground:		white",
  "*ncolors:		20", /* TODO: bind this to `count' */
  "*delay:		10000",
  "*count:		20",
  "*1D: False",
  "*showVelocityVectors: True",
  "*startAngles: all",
  "*tiny: False",
  "*slow: False",
  "*startGrid: False",
  "*doubleBuffer:	True",
#ifdef HAVE_DOUBLE_BUFFER_EXTENSION
  "*useDBE:		True",
  "*useDBEClear:	True",
#endif /* HAVE_DOUBLE_BUFFER_EXTENSION */
  0
};

static XrmOptionDescRec ball2d_options [] = {
  { "-delay",		".delay",	XrmoptionSepArg, 0 },
  { "-1D",      ".1D", XrmoptionSepArg, "False" },
  { "-showVelocityVectors", ".showVelocityVectors", XrmoptionSepArg, "True" },
  { "-startAngles", ".startAngles", XrmoptionSepArg, "all" },
  { "-tiny", ".tiny", XrmoptionSepArg, "False" },
  { "-slow", ".slow", XrmoptionSepArg, "False" },
  { "-startGrid", ".startGrid", XrmoptionSepArg, "False" },
  { "-count",		".count",	XrmoptionSepArg, 0 },
  { "-ncolors",		".ncolors",	XrmoptionSepArg, 0 },
  { "-db",		".doubleBuffer", XrmoptionNoArg,  "True"  },
  { "-no-db",		".doubleBuffer", XrmoptionNoArg,  "False" },
  { 0, 0, 0, 0 }
};


XSCREENSAVER_MODULE ("Ball2D", ball2d)

/*
  Local Variables:
  compile-command: "make ball2d"
  End:
*/
