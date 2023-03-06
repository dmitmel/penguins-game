#pragma once

typedef struct Screen {
  int x_max;
  int y_max;
} Screen;

Screen* screen_new(void);