#include "game.h"


typedef enum Colors{
  RED,
  GREEN,
  YELLOW,
  BLUE,
  MAGENTA,
}Colors;


void red();
void yellow();
void green();
void blue();
void magenta();

void water_color();
void ice_color();
void reset_color();

Colors check_color(Player* current_player);
void player_color(Player* current_player);