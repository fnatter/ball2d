  /* top left */
  st->balls[1].x = MINX + 5 + st->balls[1].radius;
  st->balls[1].y = MINY + 5 + st->balls[1].radius;
  dir.x = -st->balls[1].x; dir.y = -st->balls[1].y;
  Vector2D_normalize(&dir);
  st->balls[1].dx = dir.x; st->balls[1].dy = dir.y;

  /* top */
  st->balls[2].x = MINX + (MAXX-MINX)/2.0;
  st->balls[2].y = MINY + 5 + st->balls[2].radius;
  dir.x = -st->balls[2].x; dir.y = -st->balls[2].y;
  Vector2D_normalize(&dir);
  st->balls[2].dx = dir.x; st->balls[2].dy = dir.y;

  /* top right */
  st->balls[3].x = MAXX - 5 - st->balls[3].radius;
  st->balls[3].y = MINY + 5 + st->balls[3].radius;
  dir.x = -st->balls[3].x; dir.y = -st->balls[3].y;
  Vector2D_normalize(&dir);
  st->balls[3].dx = dir.x; st->balls[3].dy = dir.y;

  /* right */
  st->balls[4].x = MAXX - 5 - st->balls[4].radius;
  st->balls[4].y = MINY + (MAXY-MINY)/2.0;
  dir.x = -st->balls[4].x; dir.y = -st->balls[4].y;
  Vector2D_normalize(&dir);
  st->balls[4].dx = dir.x; st->balls[4].dy = dir.y;

  /* bottom right */
  st->balls[5].x = MAXX - 5 - st->balls[5].radius;
  st->balls[5].y = MAXY - 5 - st->balls[5].radius;
  dir.x = -st->balls[5].x; dir.y = -st->balls[5].y;
  Vector2D_normalize(&dir);
  st->balls[5].dx = dir.x; st->balls[5].dy = dir.y;
      
  /* bottom */
  st->balls[6].x = MINX + (MAXX-MINX)/2.0;
  st->balls[6].y = MAXY - 5 - st->balls[6].radius;
  dir.x = -st->balls[6].x; dir.y = -st->balls[6].y;
  Vector2D_normalize(&dir);
  st->balls[6].dx = dir.x; st->balls[6].dy = dir.y;

  /* bottom left */
  st->balls[7].x = MINX + 5 + st->balls[7].radius;
  st->balls[7].y = MAXY - 5 - st->balls[7].radius;
  dir.x = -st->balls[7].x; dir.y = -st->balls[7].y;
  Vector2D_normalize(&dir);
  st->balls[7].dx = dir.x; st->balls[7].dy = dir.y;
