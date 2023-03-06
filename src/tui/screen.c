#include "screen.h"
#include <curses.h>
#include <stdlib.h>

Screen* screen_new(void) {
  Screen* self = malloc(sizeof(Screen));
  getmaxyx(stdscr, self->y_max, self->x_max);
  return self;
}