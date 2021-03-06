#/* xscreensaver, Copyright (c) 1992, 1995, 1996, 1997, 1998
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
  Copyleft 2011, Felix Natter <fnatter@gmx.net>
  
  If you want to understand the physics for ball collisions, just drop me a
  mail (fnatter@gmx.net) and I'll send you a pdf explaining this (you can also
  ask about other aspects of ball2d.c :-).
 */

#include <math.h>
#include "screenhack.h"
#include "alpha.h"
#ifdef HAVE_DOUBLE_BUFFER_EXTENSION
# include "xdbe.h"
#endif /* HAVE_DOUBLE_BUFFER_EXTENSION */
#include <time.h>
#include <X11/Xmu/Drawing.h>

/* #define LOGGING 1 */

/*
  TODO:
  - dbuf + reshape (deluxe.c!)
*/

/* typedefs / structs */
typedef double number;

/* collision type for Ball vs axis-aligned rectangular obstacles */
typedef enum {
  LEFT_EDGE = 1, RIGHT_EDGE, TOP_EDGE, BOTTOM_EDGE,
  TOPLEFT_CORNER, TOPRIGHT_CORNER, BOTTOMLEFT_CORNER, BOTTOMRIGHT_CORNER,
  CENTER_INSIDE /* this shouldn't happen */
} ObstacleCollisionType;

typedef struct _Vector2D {
  number x;
  number y;
} Vector2D;

typedef struct _Obstacle
{
  number x1, y1, x2, y2;
} Obstacle;

struct _Ball;

typedef enum {
  NONE,
  LEFT_WALL_HIT, RIGHT_WALL_HIT, TOP_WALL_HIT, BOTTOM_WALL_HIT,
  OBSTACLE_COLLISION, BALL_COLLISION
} EventType;

typedef struct _Event {
  EventType type;
  Obstacle* obstacle;
  struct _Ball* otherBall;
} Event;


typedef struct _Ball {
  number x,y;
  number dx,dy;
  number mass;
  number radius;
  unsigned long color;
  double velocity;
  double way_to_go;
  int EOFiteration;

  struct _Event lastEvent;

  GC gc;
} Ball;

typedef struct {
  Ball* b1;
  Ball* b2;
  number t;
} FutureBallCollision;

struct state {
  Display *dpy;
  Window window;

  long iteration_number;
  time_t startTime;
  time_t lastZombieTime;

  Ball* balls;
  Obstacle* obstacles;
  int nObstacles;
  
  Bool dbuf;
  int delay;
  int count;
  int ncolors;
  Bool showVelocityVectors;
  int* startAngles;
  int numStartAngles;
  Bool tiny;
  Bool startGrid;
  GC velocityVectorGC, obstacleGC, highlightGC;
  Bool zombies;
  Bool corners;
  Bool debian;
  Bool slowStart;
  number max_radius;
  Bool showFutureCollisions;
  FutureBallCollision nextBallCollision;

  XColor *colors;
  GC erase_gc;
  XWindowAttributes xgwa;
  Pixmap b, ba, bb;	/* double-buffer to reduce flicker */

# ifdef HAVE_DOUBLE_BUFFER_EXTENSION
  Bool dbeclear_p;
  XdbeBackBuffer backb;
# endif /* HAVE_DOUBLE_BUFFER_EXTENSION */
};

/* globals */
number MINX = -100.0;
number MAXX = 100.0;
number MINY = -100.0;
number MAXY = 100.0;
#define ANGLES_ALL (NULL)

/* was: MAX_RADIUS = 6.0 */ /* was: SINGLE_STEP = 0.25, SEPARATE_STEP=0.01, MAX_MASS=2.0, MIN_MASS=0.1 */
const double MIN_RADIUS = 1.0, MAX_RADIUS = 10.0; 
double MIN_VELOCITY = 0.0, MAX_VELOCITY = 2.0, MIN_MASS = 0.5, MAX_MASS = 2.0, SINGLE_STEP = 0.05, SEPARATE_STEP = 0.01;
#define MAX_MOMENTUM (MAX_VELOCITY * MAX_MASS)
const double PERCENT_AFTER_MOVE = 1.0, PERCENT_AFTER_WALL_HIT = 1.0; /* was: 0.9999 */

static number
pow2(number x)
{
  return x*x;
}

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
Vector2D_rotate(Vector2D* vec, number degrees, Bool clockWise)
{
  number x, y, angle = degrees * M_PI/180.0,
    cosAngle, sinAngle;

  /* see: http://en.wikipedia.org/wiki/Cartesian_coordinate_system#Rotation 
     (CW/CCW is reversed in the article, because it assumes that top
     means high y value (which is not true for ball2d))
  */
  /* also: cos(angle) = cos(-angle) = sin(angle) = -sin(-angle) */
  cosAngle = cos(angle);
  sinAngle = sin(angle);
  if (clockWise)
    {
      x = vec->x*cosAngle - vec->y*sinAngle;
      y = vec->x*sinAngle + vec->y*cosAngle;
    }
  else
    {
      x = vec->x*cosAngle  + vec->y*sinAngle;
      y = -vec->x*sinAngle + vec->y*cosAngle;
    }

  vec->x = x;
  vec->y = y;
}

static void
recomputeYRange(struct state* st)
{
  double wld_height = (MAXX-MINX) * st->xgwa.height/(number)st->xgwa.width;
  MINY = -wld_height/2.0;
  MAXY = wld_height/2.0;
}

/*
static void
map_length(struct state* st, double wld, int* screen)
{
  wld /= (MAXY-MINY);
  wld *= (st->xgwa.height-1);
  *screen = (int)(wld + 0.5);
}
*/

static void
map_length_x(struct state* st, double wld, int* screen)
{
  wld /= (MAXX-MINX);
  wld *= (st->xgwa.width-1);
  *screen = (int)(wld + 0.5);
}

static void
map_length_y(struct state* st, double wld, int* screen)
{
  wld /= (MAXY-MINY);
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
     deviceX = (int)((ball>position.x-MINX)/(MAXX-MINX) * st->xgwa.width);
     deviceY = (int)((ball->position.y-MINY)/(MAXY-MINY) * st->xgwa.height);
  */
}


static void
Ball_init(struct state* st, Ball* ball, int ballIdx)
{ 
  int angle;

  if (st->tiny) /* tiny mode: 95% small balls */
    {
      int tinyProb = new_number_random_int(1, 100);
      if (tinyProb < 95)
        ball->mass = new_number_random(MIN_MASS, MIN_MASS + 0.1 * (MAX_MASS-MIN_MASS));
      else /* insert a few large balls */
        ball->mass = new_number_random(MIN_MASS + 0.9*(MAX_MASS-MIN_MASS), MAX_MASS);
    }
  else if (st->debian)
    {
      /* insert only small balls in "debian mode" */
      ball->mass = new_number_random(MIN_MASS, MIN_MASS + 0.1 * (MAX_MASS-MIN_MASS));
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
      double angle = new_number_random(0.0, 360.0)*M_PI/180.0;
      ball->dx = cos(angle);
      ball->dy = sin(angle);
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

  ball->velocity = new_number_random(MIN_VELOCITY, MAX_VELOCITY);
  if (st->debian)
    {
      ball->velocity = MIN_VELOCITY + (MAX_VELOCITY-MIN_VELOCITY)/2.0;
    }
  else if (st->corners)
    {
      ball->velocity = MIN_VELOCITY + (MAX_VELOCITY-MIN_VELOCITY)/2.0;
    }

  {
    XGCValues gcv;
    unsigned long flags;
    
    flags = GCForeground | GCBackground;
    gcv.foreground = st->colors[ballIdx % st->ncolors].pixel;
    gcv.background = 0L;
    ball->gc = XCreateGC (st->dpy, st->b, flags, &gcv);
  }  

  ball->lastEvent.type = NONE;
  ball->lastEvent.obstacle = NULL;
  ball->lastEvent.otherBall = NULL;
}

static void
Ball_randomizePosition (struct state* st, Ball* ball)
{
  Vector2D position;
  position = new_vector_random (MINX + ball->radius, MAXX - ball->radius,
                                MINY + ball->radius, MAXY - ball->radius);
  ball->x = position.x;
  ball->y = position.y;
}

static number
Balls_findPathIntersection(Ball* b1, Ball* b2)
{
  number px1 = b1->x, py1 = b1->y, px2 = b2->x, py2 = b2->y;
  number vx1 = b1->dx*b1->velocity, vy1 = b1->dy*b1->velocity,
    vx2 = b2->dx*b2->velocity, vy2 = b2->dy*b2->velocity;
  number r1 = b1->radius, r2 = b2->radius;
  number sqrtArg;
  number t1, t2;
  
  sqrtArg = -pow2(px1)*pow2(vy1-vy2)+2*px1*(vy1-vy2)*(px2*(vy1-vy2)+(py1-py2)*(vx1-vx2))-pow2(px2)*pow2(vy1-vy2)+2*px2*(py1-py2)*(vx1-vx2)*(vy2-vy1)-pow2(py1)*(pow2(vx1)-2*vx1*vx2+pow2(vx2))+2*py1*py2*(pow2(vx1)-2*vx1*vx2+pow2(vx2))-pow2(py2)*(pow2(vx1)-2*vx1*vx2+pow2(vx2))+pow2(r1+r2)*(pow2(vx1)-2*vx1*vx2+pow2(vx2)+pow2(vy1-vy2));

  if (sqrtArg < 0)
    return INFINITY;

  t1=(sqrt(sqrtArg)    +px1*(vx2-vx1)+px2*(vx1-vx2)+(py1-py2)*(vy2-vy1))
    /(pow2(vx1)-2*vx1*vx2+pow2(vx2)+pow2(vy1-vy2));
  
  t2=-(sqrt(sqrtArg)   +px1*(vx1-vx2)+px2*(vx2-vx1)+(py1-py2)*(vy1-vy2))
    /(pow2(vx1)-2*vx1*vx2+pow2(vx2)+pow2(vy1-vy2));

  if (t1 < 0)
    return (t2 < 0) ? INFINITY : t2;
  else if (t2 < 0)
    return t1;
  else
    return (t1 < t2) ? t1 : t2;
}

static Bool
Ball_collision_check(Ball* b1, Ball* b2)
{
  double r1pr2, dx, dy;
  r1pr2 = b1->radius + b2->radius;
  dx = b2->x - b1->x;
  dy = b2->y - b1->y;
  if (dx*dx + dy*dy <= r1pr2*r1pr2)
    return True;
  else
    return False;
}

static Bool
Ball_collision_with_other(struct state* st, Ball* b)
{
  int i;
  for (i = 0; i < st->count; i++)
    {
      Ball* other = &st->balls[i];
      if (other == b)
        continue;
      if (Ball_collision_check(b, other))
        return True;
    }
  return False;
}

static void
Ball_setDirectionForTarget(Ball* b, number tx, number ty)

{
  Vector2D dir;
  dir.x = tx - b->x;
  dir.y = ty - b->y;
  Vector2D_normalize(&dir);
  b->dx = dir.x;
  b->dy = dir.y;
}

static Bool
Ball_PointInside(Ball* ball, number x, number y)
{
  number dx, dy;

  dx = ball->x - x;
  dy = ball->y - y;
  if (dx*dx + dy*dy <= ball->radius*ball->radius)
    return True;
  else
    return False;
}

static Bool
Ball_Obstacle_collision_check(Ball* ball, Obstacle* obstacle,
                                           ObstacleCollisionType* type)
{
  /* 1. center of ball is inside rectangle */
  if (ball->x >= obstacle->x1 && ball->x <= obstacle->x2 &&
      ball->y >= obstacle->y1 && ball->y <= obstacle->y2)
    {
      *type = CENTER_INSIDE;
      return True;
    }

  /* 2. ball intersects with edge of rectangle (in simple way) */
  /* 2.1 left edge */
  if (ball->x < obstacle->x1 &&
      ball->y >= obstacle->y1 && ball->y <= obstacle->y2 &&
      ball->x + ball->radius >= obstacle->x1)
    {
      *type = LEFT_EDGE;
      return True;
    }
  /* 2.2 right edge */
  if (ball->x > obstacle->x2 &&
      ball->y >= obstacle->y1 && ball->y <= obstacle->y2 &&
      ball->x - ball->radius <= obstacle->x2)
    {
      *type = RIGHT_EDGE;
      return True;
    }
  /* 2.3 top edge */
  if (ball->y < obstacle->y1 &&
      ball->x >= obstacle->x1 && ball->x <= obstacle->x2 &&
      ball->y + ball->radius >= obstacle->y1)
    {
      *type = TOP_EDGE;
      return True;
    }
  /* 2.4 bottom edge */
  if (ball->y > obstacle->y2 &&
      ball->x >= obstacle->x1 && ball->x <= obstacle->x2 &&
      ball->y - ball->radius <= obstacle->y2)
    {
      *type = BOTTOM_EDGE;
      return True;
    }

  /* 3. ball intersects with a corner of rectangle 
     (these intersections are not found by 2.) */
  if (Ball_PointInside(ball, obstacle->x1, obstacle->y1))
    {
      *type = TOPLEFT_CORNER;
      return True;
    }
  if (Ball_PointInside(ball, obstacle->x2, obstacle->y1))
    {
      *type = TOPRIGHT_CORNER;
      return True;
    }
  if (Ball_PointInside(ball, obstacle->x1, obstacle->y2))
    {
      *type = BOTTOMLEFT_CORNER;
      return True;
    }
  if (Ball_PointInside(ball, obstacle->x2, obstacle->y2))
    {
      *type = BOTTOMRIGHT_CORNER;
      return True;
    }

  return False;
}

static Bool
Ball_Obstacle_collision_check_iterative(Ball* ball, Obstacle* obstacle,
                              ObstacleCollisionType* type)
{
  /*
  Bool experimentalResult = Ball_Obstacle_collision_check_experimental(ball, obstacle, type);
  return experimentalResult;
  */
  Bool result = False;
  number angle;


  /* 1. center of ball is inside rectangle */
  if (ball->x >= obstacle->x1 && ball->x <= obstacle->x2 &&
      ball->y >= obstacle->y1 && ball->y <= obstacle->y2)
    return True;

  /* 2. check for intersection */
  /* NOTE: we do need a very small step size in order not to miss anything,
     even 0.01° (0.0001745 ist not enough!!)
   */
  for (angle = 0.0; angle <= 2*M_PI; angle += 0.0001745)
    {
      number x,y;
      x = ball->x + cos(angle)*ball->radius;
      y = ball->y + sin(angle)*ball->radius;
      /* printf("(angle=%.2f, x=%.2f, y=%.2f)\n", angle, x, y);*/
      if (x >= obstacle->x1 && x <= obstacle->x2 &&
          y >= obstacle->y1 && y <= obstacle->y2)
        {
          result = True;
          break;
        }
    }
  /*
  if (result != experimentalResult) {
    printf("Warning: result!=experimentalResult: ball->x=%.5f, ball->y=%.5f, ball->radius=%.5f, obstacle=(%.2f,%2.f),(%.2f,%.2f), expResult=%d\n", ball->x, ball->y, ball->radius, obstacle->x1, obstacle->y1, obstacle->x2, obstacle->y2, experimentalResult);
  }
  */
  return result;
}

static void
Obstacle_draw(struct state* st, Drawable w, Obstacle* obstacle)
{
  int x1, y1, x2, y2;
  
  map2screen(st, obstacle->x1, obstacle->y1, &x1, &y1);
  map2screen(st, obstacle->x2, obstacle->y2, &x2, &y2);
  
  /*
  XFillRectangle(st->dpy, w, st->obstacleGC,
                 x1, y1, x2-x1, y2-y1);
  */
  XmuFillRoundedRectangle(st->dpy, w, st->obstacleGC,
                          x1, y1, x2-x1, y2-y1,
                          1, 1);
}

static void
Ball_draw (struct state* st, Drawable w, Ball *ball)
{
  int deviceX, deviceY, radiusX, radiusY;

  map_length_x(st, ball->radius, &radiusX);
  map_length_y(st, ball->radius, &radiusY);
  map2screen(st, ball->x, ball->y,
             &deviceX, &deviceY);

  XFillArc (st->dpy, w, ball->gc,
            deviceX - radiusX,
            deviceY - radiusY,
            radiusX*2, radiusY*2,
            0, 360*64);
  if (st->nextBallCollision.b1 == ball ||
      st->nextBallCollision.b2 == ball)
    {
      Ball* otherBall = (st->nextBallCollision.b1 == ball) ?
        st->nextBallCollision.b2 : st->nextBallCollision.b1;
      int deviceX2, deviceY2;

      XDrawArc (st->dpy, w, st->highlightGC,
                deviceX - radiusX,
                deviceY - radiusY,
                radiusX*2, radiusY*2,
                0, 360*64);

      map2screen(st, otherBall->x, otherBall->y, &deviceX2, &deviceY2);
      /*
        XDrawLine(st->dpy, w, st->velocityVectorGC,
                deviceX, deviceY, deviceX2, deviceY2);
      */
    }

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

      XDrawLine(st->dpy, w, st->velocityVectorGC,
                deviceX, deviceY,
                velocityVectorX, velocityVectorY);
    }
}

static void
Ball_doRotate(Ball* b, number degrees, Bool clockWise)
{
  Vector2D dirVec;
  
  dirVec.x = b->dx;
  dirVec.y = b->dy;

  Vector2D_rotate(&dirVec, degrees, clockWise);
  
  b->dx = dirVec.x;
  b->dy = dirVec.y;
}

static void
Ball_doRotate45Degrees(Ball* b, Bool clockWise)
{
  number dx, dy;
  number cos_45 = cos(M_PI/4.0);

  /* see: http://en.wikipedia.org/wiki/Cartesian_coordinate_system#Rotation 
     (CW/CCW is reversed in the article, because it assumes that top
     means high y value (which is not true for ball2d)
  */
  /* also: cos(45°) = cos(-45°) = sin(45°) = -sin(-45°) */

  if (clockWise)
    {
      dx = b->dx*cos_45 - b->dy*cos_45;
      dy = b->dx*cos_45 + b->dy*cos_45;
    }
  else
    {
      dx = b->dx*cos_45 + b->dy*cos_45;
      dy = -b->dx*cos_45 + b->dy*cos_45;
    }
  b->dx = dx;
  b->dy = dy;
}

static Bool
Ball_doObstacleCollision(struct state* st, Ball* b, Obstacle* obstacle)
{
  double move, dist;
  int numiter;
  ObstacleCollisionType lastCollisionType;

#ifdef LOGGING
  printf("Ball_doObstacleCollision(): move apart\n");
#endif

  /* do not collide with the same obstacle with no event in between! */
  if (b->lastEvent.type == OBSTACLE_COLLISION &&
      b->lastEvent.obstacle == obstacle)
    return False;

  /* Ball_Obstacle_collision_check(b, obstacle, &lastCollisionType);*/

  move = SEPARATE_STEP * b->velocity / MAX_VELOCITY;
  numiter = 0;
  dist = 0.0;
  while (Ball_Obstacle_collision_check(b, obstacle, &lastCollisionType))
    {
      b->x -= b->dx * move;
      b->y -= b->dy * move;
      dist += move;
      numiter++;
    }

  b->x -= b->dx * move;
  b->y -= b->dy * move;
  b->x -= b->dx * move;
  b->y -= b->dy * move;
  b->x -= b->dx * move;
  b->y -= b->dy * move;
  b->x -= b->dx * move;
  b->y -= b->dy * move;

#ifdef DEBUG_BALL_OBSTACLE
  if (lastCollisionType == TOPLEFT_CORNER ||
      lastCollisionType == TOPRIGHT_CORNER ||
      lastCollisionType == BOTTOMLEFT_CORNER ||
      lastCollisionType == BOTTOMRIGHT_CORNER)
    {
      int ballIdx = -1, i;
      for (i = 0; i < st->count; i++)
        {
          if (&st->balls[i] == b)
           ballIdx = i;
        }

      printf("Ball_obstacle move_apart: ballIdx=%d, dist=%.2f, numiter=%d (SINGLE_STEP=%.2f)\n", ballIdx, dist, numiter, SINGLE_STEP);
      Ball_draw(st, st->b, b, True);
      {
        XdbeSwapInfo info[1];
        info[0].swap_window = st->window;
        info[0].swap_action = (st->dbeclear_p ? XdbeBackground : XdbeUndefined);
        XdbeSwapBuffers (st->dpy, info, 1);
      }
      st->highlightedBall = b;
      getchar();
    }
#endif

#ifdef LOGGING
  printf("Ball_doObstacleCollision(): lastCollisionType=%d\n", lastCollisionType);
#endif

  switch(lastCollisionType)
    {
      Vector2D center_of_ball, corner, virtualWall;
      number cosAngle, angle;
      
    case LEFT_EDGE:
    case RIGHT_EDGE:
      b->dx = -b->dx;
      break;
    case TOP_EDGE:
    case BOTTOM_EDGE:
      b->dy = -b->dy;
      break;
    case TOPLEFT_CORNER:
#ifdef LOGGING
      printf("Ball_doObstacleCollision(): TOPLEFT_CORNER\n");
#endif
      center_of_ball.x = b->x;
      center_of_ball.y = b->y;
      corner.x = obstacle->x1;
      corner.y = obstacle->y1;
      
      /* calculate the normal to the virtual wall */
      virtualWall = Vector2D_sub(&center_of_ball, &corner);
      Vector2D_normalize(&virtualWall);
      /* convert normal to virtual wall */
      Vector2D_rotate(&virtualWall, 90.0, False);
      /* determine angle to x axis (scalar product between virtualWall and
         obstacle edge (0,1)) */
      cosAngle = virtualWall.y;
      angle = acos(cosAngle) * 180.0/M_PI;

      /* 1. rotate the setup so that the virtual wall is parallel to y
         axis */
      Ball_doRotate(b, angle, False);
      /* 2. treat it like a top wall hit */
      b->dx = -b->dx;
      /* 3. rotate back */
      Ball_doRotate(b, angle, True);

      break;
    case TOPRIGHT_CORNER:
#ifdef LOGGING
      printf("Ball_doObstacleCollision(): TOPRIGHT_CORNER\n");
#endif

      center_of_ball.x = b->x;
      center_of_ball.y = b->y;
      corner.x = obstacle->x2;
      corner.y = obstacle->y1;
      
      /* calculate the normal to the virtual wall */
      virtualWall = Vector2D_sub(&center_of_ball, &corner);
      Vector2D_normalize(&virtualWall);
      /* convert normal to virtual wall */
      Vector2D_rotate(&virtualWall, 90.0, False);
      /* determine angle to x axis (scalar product between virtualWall and
         obstacle edge (-1, 0)) */
      cosAngle = -virtualWall.x;
      angle = acos(cosAngle) * 180.0/M_PI;

      /* 1. rotate the setup so that the virtual wall is parallel to x
         axis */
      Ball_doRotate(b, angle, False);
      /* 2. treat it like a bottom wall hit */
      b->dy = -b->dy;
      /* 3. rotate back */
      Ball_doRotate(b, angle, True);
      break;

    case BOTTOMLEFT_CORNER:
#ifdef LOGGING
      printf("Ball_doObstacleCollision(): BOTTOMLEFT_CORNER\n");
#endif

      center_of_ball.x = b->x;
      center_of_ball.y = b->y;
      corner.x = obstacle->x1;
      corner.y = obstacle->y2;
      
      /* calculate the normal to the virtual wall */
      virtualWall = Vector2D_sub(&center_of_ball, &corner);
      Vector2D_normalize(&virtualWall);
      /* convert normal to virtual wall */
      Vector2D_rotate(&virtualWall, 90.0, False);
      /* determine angle to x axis (scalar product between virtualWall and
         obstacle edge (1,0)) */
      cosAngle = virtualWall.x;
      angle = acos(cosAngle) * 180.0/M_PI;

      /* 1. rotate the setup so that the virtual wall is parallel to x
         axis */
      Ball_doRotate(b, angle, False);
      /* 2. treat it like a top wall hit */
      b->dy = -b->dy;
      /* 3. rotate back */
      Ball_doRotate(b, angle, True);
      break;

    case BOTTOMRIGHT_CORNER:
#ifdef LOGGING
      printf("Ball_doObstacleCollision(): BOTTOMRIGHT_CORNER\n");
#endif

      center_of_ball.x = b->x;
      center_of_ball.y = b->y;
      corner.x = obstacle->x2;
      corner.y = obstacle->y2;
      
      /* calculate the normal to the virtual wall */
      virtualWall = Vector2D_sub(&center_of_ball, &corner);
      Vector2D_normalize(&virtualWall);
      /* convert normal to virtual wall */
      Vector2D_rotate(&virtualWall, 90.0, False);
      /* determine angle to y axis (scalar product between virtualWall and
         obstacle edge (0,-1)) */
      cosAngle = -virtualWall.y;
      angle = acos(cosAngle) * 180.0/M_PI;

      /* 1. rotate the setup so that the virtual wall is parallel to y
         axis */
      Ball_doRotate(b, angle, False);
      /* 2. treat it like a right wall hit */
      b->dx = -b->dx;
      /* 3. rotate back */
      Ball_doRotate(b, angle, True);
      break;

    case CENTER_INSIDE:
      printf("Warning: last obstacle collision type is CENTER_INSIDE. Need smaller move apart step size?\n");
      break;
    }

  b->lastEvent.type = OBSTACLE_COLLISION;
  b->lastEvent.obstacle = obstacle;

  return True;
}

static Bool
Ball_doCollision(Ball* b1, Ball* b2)
{
  double move1, move2, totalMoved;
  Bool separationWithInverseVelocitiesSucceeded;
  Vector2D position_b1, position_b2, line_of_sight, b1_velocity_before, b2_velocity_before, b1_velocity_after, b2_velocity_after;
  double b1_velocity_in_line_of_sight, b2_velocity_in_line_of_sight, b1_velocity_in_line_of_sight_before, b2_velocity_in_line_of_sight_before, b1_change_in_line_of_sight_velocity, b2_change_in_line_of_sight_velocity;

#ifdef LOGGING
  printf("Ball_doCollision()\n");
#endif

  if (b1->lastEvent.type == BALL_COLLISION &&
      b1->lastEvent.otherBall == b2 &&
      b2->lastEvent.type == BALL_COLLISION &&
      b2->lastEvent.otherBall == b1)
    return False;

  position_b1.x = b1->x;
  position_b1.y = b1->y;
  position_b2.x = b2->x;
  position_b2.y = b2->y;

  /* separate the balls so that they are non-overlapping */
  move1 = SEPARATE_STEP * b1->velocity / MAX_VELOCITY;
  move2 = SEPARATE_STEP * b2->velocity / MAX_VELOCITY;
  totalMoved = 0.0;
  separationWithInverseVelocitiesSucceeded = True;
  while (Ball_collision_check(b1, b2))
    {
      b1->x -= b1->dx * move1;
      b1->y -= b1->dy * move1;
      b2->x -= b2->dx * move2;
      b2->y -= b2->dy * move2;

      totalMoved += move1 + move2;
      if (totalMoved >= SINGLE_STEP*2)
        {
          separationWithInverseVelocitiesSucceeded = False;
          break;
        }
    }
  if (separationWithInverseVelocitiesSucceeded)
    {
      /* make sure that the balls have some room */
      /*
      b1->x -= b1->dx * move1;
      b1->y -= b1->dy * move1;
      b2->x -= b2->dx * move2;
      b2->y -= b2->dy * move2;
      */
    }
  else
    {
      /* separating via inverse velocities failed (could be one ball
         chasing the other (two balls having very similar velocity vectors)
         => reset ball positions and separate balls along line of sight */
      /* direction b1 -> b2 */
      line_of_sight = Vector2D_sub(&position_b2, &position_b1); 
      Vector2D_normalize(&line_of_sight);
      while (Ball_collision_check(b1, b2))
        {
          b1->x -= line_of_sight.x * move1;
          b1->y -= line_of_sight.y * move1;
          b2->x += line_of_sight.x * move2;
          b2->y += line_of_sight.y * move2;
        }
      /* make sure that the balls have some room */
      /*
      b1->x -= line_of_sight.x * move1;
      b1->y -= line_of_sight.y * move1;
      b2->x += line_of_sight.x * move2;
      b2->y += line_of_sight.y * move2;
      */
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

  b1->velocity = Vector2D_length(&b1_velocity_after);
  b2->velocity = Vector2D_length(&b2_velocity_after);
  Vector2D_normalize(&b1_velocity_after);
  Vector2D_normalize(&b2_velocity_after);
  b1->dx = b1_velocity_after.x;
  b1->dy = b1_velocity_after.y;
  b2->dx = b2_velocity_after.x;
  b2->dy = b2_velocity_after.y;

  b1->lastEvent.type = BALL_COLLISION;
  b1->lastEvent.otherBall = b2;
  b2->lastEvent.type = BALL_COLLISION;
  b2->lastEvent.otherBall = b1;

  return True;
}

static void
Balls_move(struct state* st)
{
  int i, j, nBalls;
  Ball* balls;
  Bool reiterate;
  double thisway;
  Ball* cball;
  time_t timeDelta, currentTime;

#ifdef LOGGING
  printf("Balls_move()\n");
#endif

  balls = st->balls;
  nBalls = st->count;

  /*return; */

  /*
  if (st->iteration_number % 10 == 0)
    {
      for (i = 0; i < nBalls; i++)
        {
          Ball_doRotate45Degrees(&st->balls[i], True);
        }
    }
  */

  /* printf("iteration_number=%ld\n", st->iteration_number); */

  currentTime = time(NULL);
  timeDelta = currentTime - st->startTime;

  if (st->zombies && st->lastZombieTime != currentTime &&
      timeDelta > 0 && timeDelta % 10 == 0)
    {
      int angle = new_number_random_int(-1,7)*45;

      for (i = 0; i < nBalls; i++)
        {
          if (angle == -45)
            Ball_setDirectionForTarget(&balls[i], 0.0, 0.0);
          else
            {
              balls[i].dx = cos(angle/180.0*M_PI);
              balls[i].dy = sin(angle/180.0*M_PI);
            }
        }
      st->lastZombieTime = currentTime; 
    }

  for (i = 0; i < nBalls; i++)
    {
      balls[i].way_to_go = balls[i].velocity;
      balls[i].EOFiteration = 0;
    }
  do
    {
      ObstacleCollisionType obsCollType;

      for (i = 0; i < nBalls; i++)
        {
          cball = balls + i;
          if (cball->way_to_go > SINGLE_STEP)
            thisway = SINGLE_STEP;
          else
            {
              thisway = cball->way_to_go;
              cball->EOFiteration = True;
            }
          cball->way_to_go -= thisway;
          cball->x += cball->dx * thisway;
          cball->y += cball->dy * thisway;
#ifdef LOGGING
          printf("Balls[%d] = (%.1f,%.1f)\n", i, cball->x, cball->y);
#endif
          if (cball->x + cball->radius > MAXX)
            {
              cball->x = MAXX - cball->radius - SINGLE_STEP;
              cball->dx = -cball->dx;
              cball->velocity *= PERCENT_AFTER_WALL_HIT;
              cball->EOFiteration = False;
              cball->lastEvent.type = RIGHT_WALL_HIT;
            }
          if (cball->y + cball->radius > MAXY)
            {
              cball->y = MAXY - cball->radius - SINGLE_STEP;
              cball->dy = -cball->dy;
              cball->velocity *= PERCENT_AFTER_WALL_HIT;
              cball->EOFiteration = False;
              cball->lastEvent.type = BOTTOM_WALL_HIT;
            }
          if (cball->x < MINX + cball->radius)
            {
              cball->x = MINX + cball->radius + SINGLE_STEP;
              cball->dx = -cball->dx;
              cball->velocity *= PERCENT_AFTER_WALL_HIT;
              cball->EOFiteration = False;
              cball->lastEvent.type = LEFT_WALL_HIT;
            }
          if (cball->y < MINY + cball->radius)
            {
              cball->y = MINY + cball->radius + SINGLE_STEP;
              cball->dy = -cball->dy;
              cball->velocity *= PERCENT_AFTER_WALL_HIT;
              cball->EOFiteration = False;
              cball->lastEvent.type = TOP_WALL_HIT;
            }
          
          /* check for collisions with obstacle(s) */
          for (j = 0; j < st->nObstacles; j++)
            {
              if (Ball_Obstacle_collision_check(cball, &st->obstacles[j], &obsCollType))
                {
                  if (Ball_doObstacleCollision(st, cball, &st->obstacles[j]))
                    {
                      cball->way_to_go += SINGLE_STEP;
                      cball->EOFiteration = False;
                      break;
                    }
                }
            }

          /* check for collisions */
          for (j = 0; j < i; j++)
            {
              if (Ball_collision_check(cball, balls + j))
                {
                  if (Ball_doCollision(cball, balls + j))
                    {
                      cball->way_to_go += SINGLE_STEP;
                      cball->EOFiteration = False;
                      break;
                    }
                }
            }
        }
      reiterate = False;
      for (i = 0; i < nBalls; i++)
        {
          if (!balls[i].EOFiteration)
            reiterate = True;
        }
    } while (reiterate);

  if (st->showFutureCollisions)
    {
      st->nextBallCollision.t = INFINITY;
      st->nextBallCollision.b1 = NULL;
      st->nextBallCollision.b2 = NULL;
      for (i = 0; i < nBalls; i++)
        {
          cball = &st->balls[i];
          for (j = 0; j < i; j++)
            {
              number t = Balls_findPathIntersection(cball, &st->balls[j]);
              if (t < st->nextBallCollision.t)
                {
                  st->nextBallCollision.t = t;
                  st->nextBallCollision.b1 = cball;
                  st->nextBallCollision.b2 = &st->balls[j];
                }
            }
        }
    }
}

static Bool
Balls_placeOnCorner(struct state* st, int* ballCounter,
                    number cornerX, number cornerY,
                    int numberBalls)
{
  int i;
  Vector2D pos;

  pos.x = cornerX;
  pos.y = cornerY;

  for (i = 0; i < numberBalls; i++)
    {
      st->balls[*ballCounter].x = pos.x;
      st->balls[*ballCounter].y = pos.y;
      Ball_setDirectionForTarget(&st->balls[*ballCounter], 0.0, 0.0);
          
      if (Ball_collision_with_other(st, &st->balls[*ballCounter]))
        return True;
      pos.x += st->balls[*ballCounter].dx * 2*st->max_radius;
      pos.y += st->balls[*ballCounter].dy * 2*st->max_radius;
      (*ballCounter)++;
    }
  return False;
}

static void
Balls_placeOnCorners(struct state* st)
{
  int numberBallsPerCorner = st->count / 8;
  int ballCounter = 0;

  /* left */
  if (Balls_placeOnCorner(st, &ballCounter, 
                          MINX + SINGLE_STEP + st->max_radius,
                          MINY + (MAXY-MINY)/2.0,
                          numberBallsPerCorner))
    {
      /* collision - stop placing balls */
      st->count = ballCounter;
      return;
    }

  /* top left */
  if (Balls_placeOnCorner(st, &ballCounter, 
                          MINX + SINGLE_STEP + st->max_radius,
                          MINY + SINGLE_STEP + st->max_radius,
                          numberBallsPerCorner))
    {
      /* collision - stop placing balls */
      st->count = ballCounter;
      return;
    }

  /* top */
  if (Balls_placeOnCorner(st, &ballCounter, 
                          MINX + (MAXX-MINX)/2.0,
                          MINY + SINGLE_STEP + st->max_radius,
                          numberBallsPerCorner))
    {
      /* collision - stop placing balls */
      st->count = ballCounter;
      return;
    }

  /* top right */
  if (Balls_placeOnCorner(st, &ballCounter, 
                          MAXX - SINGLE_STEP - st->max_radius,
                          MINY + SINGLE_STEP + st->max_radius,
                          numberBallsPerCorner))
    {
      /* collision - stop placing balls */
      st->count = ballCounter;
      return;
    }

  /* right */
  if (Balls_placeOnCorner(st, &ballCounter, 
                          MAXX - SINGLE_STEP - st->max_radius,
                          MINY + (MAXY-MINY)/2.0,
                          numberBallsPerCorner))
    {
      /* collision - stop placing balls */
      st->count = ballCounter;
      return;
    }

  /* bottom right */
  if (Balls_placeOnCorner(st, &ballCounter, 
                          MAXX - SINGLE_STEP - st->max_radius,
                          MAXY - SINGLE_STEP - st->max_radius,
                          numberBallsPerCorner))
    {
      /* collision - stop placing balls */
      st->count = ballCounter;
      return;
    }
      
  /* bottom */
  if (Balls_placeOnCorner(st, &ballCounter, 
                          MINX + (MAXX-MINX)/2.0,
                          MAXY - SINGLE_STEP - st->max_radius,
                          numberBallsPerCorner))
    {
      /* collision - stop placing balls */
      st->count = ballCounter;
      return;
    }

  /* bottom left */
  if (Balls_placeOnCorner(st, &ballCounter, 
                          MINX + SINGLE_STEP + st->max_radius,
                          MAXY - SINGLE_STEP - st->max_radius,
                          numberBallsPerCorner))
    {
      /* collision - stop placing balls */
      st->count = ballCounter;
      return;
    }
}

static void *
ball2d_init (Display *dpy, Window window)
{
  struct state *st = (struct state *) calloc (1, sizeof(*st));
  XGCValues gcv;
  int i, j;
  Bool conflict, onedee;
  char* startAngles;

  st->iteration_number = 0;
  st->startTime = time(NULL);
  st->lastZombieTime = 0;
  st->dpy = dpy;
  st->nextBallCollision.b1 = NULL;
  st->nextBallCollision.b2 = NULL;

  st->window = window;
  XGetWindowAttributes (st->dpy, st->window, &st->xgwa);
  recomputeYRange(st);

  st->tiny = get_boolean_resource(st->dpy, "tiny", "Boolean");
  st->startGrid = get_boolean_resource(st->dpy, "startGrid", "Boolean");
  st->corners = get_boolean_resource(st->dpy, "corners", "Boolean");
  st->debian = get_boolean_resource(st->dpy, "debian", "Boolean");
  st->slowStart = get_boolean_resource(st->dpy, "slowStart", "Boolean");
  st->count = get_integer_resource (st->dpy, "count", "Integer");
  st->ncolors = get_integer_resource (st->dpy, "ncolors", "Integer");
  st->delay = get_integer_resource (st->dpy, "delay", "Integer");
  st->dbuf = get_boolean_resource (st->dpy, "doubleBuffer", "Boolean");
  onedee = get_boolean_resource(st->dpy, "1D", "Boolean");
  st->zombies = get_boolean_resource(st->dpy, "zombies", "Boolean");
  st->showFutureCollisions = get_boolean_resource(st->dpy, "showFutureCollisions", "Boolean");
  if (get_boolean_resource(st->dpy, "obstacles", "Boolean") &&
      !st->corners)
    {
      st->nObstacles = 2;
      st->obstacles = (Obstacle*) calloc(st->nObstacles, sizeof(Obstacle));

      st->obstacles[0].x1 = -50.0;
      st->obstacles[0].y1 = -40.0;
      st->obstacles[0].x2 = -40.0;
      st->obstacles[0].y2 =  20.0;

      st->obstacles[1].x1 =  30.0;
      st->obstacles[1].y1 = -10.0;
      st->obstacles[1].x2 =  50.0;
      st->obstacles[1].y2 =  10.0;
    }
  else if (get_boolean_resource(st->dpy, "obstacles2", "Boolean") &&
           !st->corners)
    {
      double obstacleSize = 10.0;
      int numCols = 5, numRows = 3;
      double xSpacing = (MAXX-MINX)/(numCols+1);
      double ySpacing = (MAXY-MINY)/(numRows+1);
      int obstacleCounter = 0;

      st->startGrid = False;
      st->nObstacles = numRows * numCols;
      st->obstacles = (Obstacle*) calloc(st->nObstacles, sizeof(Obstacle));

      for (i = 1; i <= numRows; i++)
        {
          for (j = 1; j <= numCols; j++)
            {
              double x = MINX + j*xSpacing;
              double y = MINY + i*ySpacing;

              st->obstacles[obstacleCounter].x1 = x-obstacleSize/2;
              st->obstacles[obstacleCounter].y1 = y-obstacleSize/2;
              st->obstacles[obstacleCounter].x2 = x+obstacleSize/2;
              st->obstacles[obstacleCounter].y2 = y+obstacleSize/2;
              
              obstacleCounter++;
            }
        }

    }
  else
    {
      st->nObstacles = 0;
      st->obstacles = NULL;
    }

  st->showVelocityVectors = get_boolean_resource(st->dpy, "showVelocityVectors", "Boolean");
  
  if (onedee)
    {
      if (new_number_random_int(0,1) == 0)
        startAngles = strdup("90,270");
      else
        startAngles = strdup("0,180");
      st->startGrid = True;
      st->nObstacles = 0;
    }
  else if (st->debian)
    {
      startAngles = strdup("45");
    }
  else
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

# ifdef HAVE_DOUBLE_BUFFER_EXTENSION
  st->dbeclear_p = get_boolean_resource (st->dpy, "useDBEClear", "Boolean");
#endif

# ifdef HAVE_COCOA	/* Don't second-guess Quartz's double-buffering */
  st->dbuf = False;
# endif


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
      /*
      make_smooth_colormap (st->dpy, st->xgwa.visual, st->xgwa.colormap,
                            st->colors, &st->ncolors, True, 0, True);
      */
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

  if (st->corners)
    {
      if (st->count < 8)
        st->count = 8;
      else if (st->count > 30)
        st->count = 30;
      else
        st->count -= st->count % 8;
    }

  st->balls = (Ball*) calloc(st->count, sizeof(Ball));
  st->max_radius = -100.0;
  for (i = 0; i < st->count; i++)
    { 
      Ball_init(st, &st->balls[i], i);
      if (st->balls[i].radius > st->max_radius)
        st->max_radius = st->balls[i].radius;
    }


  /* create custom GCs */
  {
    XGCValues gcv;
    unsigned long flags;
    XColor obstacleColor, obstacleColor2, highlightColor, highlightColor2;
    
    flags = GCForeground;
    gcv.foreground = ~0L;
    st->velocityVectorGC = XCreateGC (st->dpy, st->b, flags, &gcv);

    XAllocNamedColor(st->dpy, st->xgwa.colormap, "grey",
                     &obstacleColor2, &obstacleColor);
    gcv.foreground = obstacleColor.pixel;
    flags |= GCCapStyle;
    gcv.cap_style = CapRound;
    st->obstacleGC = XCreateGC (st->dpy, st->b, flags, &gcv);

    XAllocNamedColor(st->dpy, st->xgwa.colormap, "yellow",
                     &highlightColor2, &highlightColor);
    gcv.foreground = highlightColor.pixel;
    /* gcv.background = highlightColor.pixel; */
    st->highlightGC = XCreateGC (st->dpy, st->b, flags, &gcv);
  }  

#ifdef LOGGING
  printf("ball2d_init()\n");
#endif

  if (st->debian)
    {
      number angle, radius;
      int ballCounter = 0;

      st->nObstacles = 0;
      st->slowStart = True;
      st->tiny = False;

      for (angle = -45.0, radius = 0.0; angle <= 720.0;
           angle += 20.0, radius += st->max_radius*1.5)
        {
          st->balls[ballCounter].x = cos(angle * M_PI/180.0) * radius;
          st->balls[ballCounter].y = sin(angle * M_PI/180.0) * radius;

          /* no more space left? */
          if (st->balls[ballCounter].x - st->balls[ballCounter].radius < MINX ||
              st->balls[ballCounter].x + st->balls[ballCounter].radius > MAXX ||
              st->balls[ballCounter].y - st->balls[ballCounter].radius < MINY ||
              st->balls[ballCounter].y + st->balls[ballCounter].radius > MAXY)
            {
              st->count = ballCounter;
              break;
            }
            
          ballCounter++;
          /* all balls placed? */
          if (ballCounter == st->count)
            break;
        }
    }
  else if (st->corners)
    {
      st->slowStart = True;
      Balls_placeOnCorners(st);
    }
  else if (st->startGrid)
    {
      number gridWidth, gridHeight, x, y;
      ObstacleCollisionType obsCollType;

      gridWidth = st->max_radius*2 + 5.0;
      gridHeight = st->max_radius*2 + 5.0;
      
      x = MINX + st->max_radius + 5;
      y = MINY + st->max_radius + 5;
      for (i = 0; i < st->count;)
        {
          Bool conflict;

          st->balls[i].x = x;
          st->balls[i].y = y;
          
          conflict = False;
          for (j = 0; j < st->nObstacles; j++)
            {
              if (Ball_Obstacle_collision_check(&st->balls[i], &st->obstacles[j],
                                                &obsCollType))
                conflict = True;
            }
          if (!conflict)
            i++;

          x += gridWidth; 
          if (x >= MAXX - st->max_radius)
            {
              x = MINX + st->max_radius + 5;
              y += gridHeight;
              if (y + gridHeight > MAXY)
                { /* no space left: ignore the st->count - i remaining balls */
                  st->count = i;
                  break;
                }
            }
        }
    }
  else
    {
      ObstacleCollisionType obsCollType;

      for (i = 0; i < st->count; i++)
        {
          int numberConflicts = 0;

          conflict = True;
          while (conflict)
            {
              Ball_randomizePosition(st, &st->balls[i]);
              conflict = False;

              /* check for collision with obstacle(s) */
              for (j = 0; j < st->nObstacles; j++)
                {
                  if (Ball_Obstacle_collision_check(&st->balls[i],
                                                    &st->obstacles[j],
                                                    &obsCollType))
                    {
                      conflict = True;
                      break;
                    }
                }

              /* check for collision with other balls */
              for (j = 0; j < i; j++)
                {
                  if (Ball_collision_check(&st->balls[i], &st->balls[j])) {
                    conflict = True;
                    break;
                  }
                }
              if (conflict && ++numberConflicts > 10000)
                {
                  /* it is very likely that there's no space left for the
                     i'th ball */
                  st->count = i-1;
                  break;
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


static unsigned long
ball2d_draw (Display *dpy, Window window, void *closure)
{
  int i;
  struct state *st = (struct state *) closure;
  time_t timeDelta, currentTime;

  st->iteration_number++;

#ifdef HAVE_DOUBLE_BUFFER_EXTENSION
  if (!st->dbeclear_p || !st->backb)
#endif /* HAVE_DOUBLE_BUFFER_EXTENSION */
    XFillRectangle (st->dpy, st->b, st->erase_gc, 0, 0, st->xgwa.width, st->xgwa.height);
  
  Balls_move(st);
  for (i = 0; i < st->nObstacles; i++)
    {
      Obstacle_draw(st, st->b, &st->obstacles[i]);
    }
  for (i = 0; i < st->count; i++)
    {
      Ball_draw(st, st->b, &st->balls[i]);
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

  currentTime = time(NULL);
  timeDelta = currentTime - st->startTime;

  if (st->slowStart && timeDelta <= 15)
    return st->delay * 2;
  else
    return st->delay;
}

static void
ball2d_reshape (Display *dpy, Window window, void *closure, 
                 unsigned int w, unsigned int h)
{
  struct state *st = (struct state *) closure;
  if (! st->dbuf) {   /* #### more complicated if we have a back buffer... */
    XGetWindowAttributes (st->dpy, st->window, &st->xgwa);
    recomputeYRange(st);
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
  int i;

  if (st->startAngles != NULL)
    free(st->startAngles);
  free_colors(st->dpy, st->xgwa.colormap, st->colors, st->ncolors);
  for (i = 0; i < st->count; i++)
    {
      XFreeGC(st->dpy, st->balls[i].gc);
    }
  free(st->balls);
  if (st->obstacles != NULL)
    free(st->obstacles);
  XFreeGC(st->dpy, st->velocityVectorGC);
  XFreeGC(st->dpy, st->obstacleGC);
}


static const char *ball2d_defaults [] = {
  ".background:		black",
  ".foreground:		white",
  "*ncolors:		50", /* TODO: bind this to `count'? */
  "*delay:	 20000",
  "*count:		20",
  "*1D: False",
  "*showVelocityVectors: True",
  "*startAngles: all",
  "*tiny: False",
  "*startGrid: False",
  "*obstacles: False",
  "*obstacles2: False",
  "*zombies: False",
  "*corners: False",
  "*debian: False",
  "*showFutureCollisions: False",
  "*slowStart: False",
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
  { "-startGrid", ".startGrid", XrmoptionSepArg, "False" },
  { "-obstacles", ".obstacles", XrmoptionSepArg, "False" },
  { "-obstacles2", ".obstacles2", XrmoptionSepArg, "False" },
  { "-zombies", ".zombies", XrmoptionSepArg, "False" },
  { "-corners", ".corners", XrmoptionSepArg, "False" },
  { "-debian", ".debian", XrmoptionSepArg, "False" },
  { "-showFutureCollisions", ".showFutureCollisions", XrmoptionSepArg, "False" },
  { "-slowStart", ".slowStart", XrmoptionSepArg, "False" },
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
