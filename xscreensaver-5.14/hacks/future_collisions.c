typedef double number;

static number
pow2(number x)
{
  return x*x;
}

static number
Balls_findPathIntersection(Ball* b1, Ball* b2)
{
  number px1 = b1->x, py1 = b1->y, px2 = b2->x, py2 = b2->y;
  number vx1 = b1->dx*b1->velocity, vy1 = b1->dy*b1->velocity,
    x2 = b2->dx*b2->velocity, vy2 = b2->dy*b2->velocity;
  number r1 = b1->radius, r2 = b2->radius;
  number sqrtArg1, sqrtArg2;
  number t1, t2;
  
  sqrtArg = -pow2(px1)*pow2(vy1-vy2)+2*px1*(vy1-vy2)*(px2*(vy1-vy2)+(py1-py2)*(vx1-vx2))-pow2(px2)*pow2(vy1-vy2)+2*px2*(py1-py2)*(vx1-vx2)*(vy2-vy1)-pow2(py1)*(pow2(vx1)-2*vx1*vx2+pow2(vx2))+2*py1*py2*(pow2(vx1)-2*vx1*vx2+pow2(vx2))-pow2(py2)*(pow2(vx1)-2*vx1*vx2+pow2(vx2))+pow2(r1+r2)*(pow2(vx1)-2*vx1*vx2+pow2(vx2)+pow2(vy1-vy2));

  if (sqrtArg < 0)
    return -infty;

  t1=(sqrt(sqrtArg)    +px1*(vx2-vx1)+px2*(vx1-vx2)+(py1-py2)*(vy2-vy1))
    /(pow2(vx1)-2*vx1*vx2+pow2(vx2)+pow2(vy1-vy2));
  
  t2=-(sqrt(sqrtArg)   +px1*(vx1-vx2)+px2*(vx2-vx1)+(py1-py2)*(vy1-vy2))
    /(pow2(vx1)-2*vx1*vx2+pow2(vx2)+pow2(vy1-vy2));

  return (t1 < t2) ? t1 : t2;
}
