#include <stdio.h>
#include <math.h>

#define Bool int
#define True 1
#define False 0

static Bool
Ball_PointInside(double bx, double by, double radius, float x, float y)
{
  float dx, dy;

  dx = bx - x;
  dy = by - y;
  if (dx*dx + dy*dy <= radius*radius)
    return True;
  else
    return False;
}

static Bool
Ball_Obstacle_collision_check_exp(double bx, double by, double radius,
                                  double x1, double y1, double x2, double y2)
{
  /* 1. center of ball is inside rectangle */
  if (bx >= x1 && bx <= x2 &&
      by >= y1 && by <= y2)
    return True;

  /* 2. ball intersects with edge of rectangle (in simple way) */
  /* 2.1 left edge */
  if (bx < x1 &&
      by >= y1 && by <= y2 &&
      bx + radius >= x1)
    return True;
  /* 2.2 right edge */
  if (bx > x2 &&
      by >= y1 && by <= y2 &&
      bx - radius <= x2)
    return True;
  /* 2.3 top edge */
  if (by < y1 &&
      bx >= x1 && bx <= x2 &&
      by + radius >= y1)
    return True;
  /* 2.4 bottom edge */
  if (by > y2 &&
      bx >= x1 && bx <= x2 &&
      by - radius <= y2)
    return True;

  /* 3. ball intersects with a corner of rectangle 
     (these intersections are not found by 2.) */
  if (Ball_PointInside(bx, by, radius, x1, y1) || /* top left */
      Ball_PointInside(bx, by, radius, x2, y1) || /* top right */
      Ball_PointInside(bx, by, radius, x1, y2) || /* bottom left */
      Ball_PointInside(bx, by, radius, x2, y2)) /* bottom right */
    return True;

  return False;
}

static double dist(double x1, double y1, double x2, double y2)
{
  return sqrt((x2-x1)*(x2-x1) + (y2-y1)*(y2-y1));
}

static int
Ball_Obstacle_collision_check(double bx, double by, double radius,
                              double x1, double y1, double x2, double y2)
{
  int result = 0;
  double angle;
  
  printf("dist(x1,y1, bx, by)=%.4f\n", dist(x1,y1, bx,by));
  printf("dist(x2,y1, bx, by)=%.4f\n", dist(x2,y1, bx,by));
  printf("dist(x1,y2, bx, by)=%.4f\n", dist(x1,y2, bx,by));
  printf("dist(x2,y2, bx, by)=%.4f\n", dist(x2,y2, bx,by));

  /* 1. center of ball is inside rectangle */
  if (bx >= x1 && bx <= x2 &&
      by >= y1 && by <= y2)
    return True;

  /* NOTE: we need a step size < 0.01° !!! */
  /* 2. check for intersection */
  for (angle = 0.0; angle <= 2*M_PI; angle += 0.00001745)
    {
      double x,y;
      x = bx + cos(angle)*radius;
      y = by + sin(angle)*radius;
      /*printf("(angle=%.5f, x=%.5f, y=%.5f)\n", angle*180.0/M_PI, x, y); */
      if (x >= x1 && x <= x2 && y >= y1 && y <= y2)
        {
          result = 1;
          break;
        }
    }

  return result;
}

int main()
{
  int result = Ball_Obstacle_collision_check(-52.16333, -46.70006,
                                             7.04089,
                                             -50.0, -40.0, -40.0, 20.0);
  int resultExp = Ball_Obstacle_collision_check_exp(-52.16333, -46.70006,
                                                    7.04089,
                                                    -50.0, -40.0, -40.0, 20.0);
  printf("result=%d, resultExp=%d\n", result, resultExp);

  return 0;
}
