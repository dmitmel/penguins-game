#include "game.h"

typedef enum Colors {
  NONE,
  RED,
  GREEN,
  YELLOW,
  BLUE,
  MAGENTA,
} Colors;

void red(void);
void yellow(void);
void green(void);
void blue(void);
void magenta(void);

void water_color(void);
void ice_color(void);
void reset_color(void);

Colors check_color(Player* current_player);
void player_color(Player* current_player);
