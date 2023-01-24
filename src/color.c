#include "color.h"
#include "game.h"
#include <stdio.h>

void red(void) {
  printf("\033[1;41m");
}
void yellow(void) {
  printf("\033[1;43m");
}
void green(void) {
  printf("\033[1;42m");
}
void blue(void) {
  printf("\033[1;44m");
}
void magenta(void) {
  printf("\033[1;45m");
}

void water_color(void) {
  printf("\033[1;46;37m");
}

void ice_color(void) {
  printf("\033[1;47;37m");
}

void reset_color(void) {
  printf("\033[0m");
}

Colors check_color(Player* current_player) {
  if (current_player->color == 1) {
    return RED;
  } else if (current_player->color == 2) {
    return GREEN;
  } else if (current_player->color == 3) {
    return YELLOW;
  } else if (current_player->color == 4) {
    return BLUE;
  } else if (current_player->color == 5) {
    return MAGENTA;
  } else {
    return NONE;
  }
}

void player_color(Player* current_player) {
  Colors check = check_color(current_player);
  switch (check) {
    case RED: red(); break;
    case GREEN: green(); break;
    case YELLOW: yellow(); break;
    case BLUE: blue(); break;
    case MAGENTA: magenta(); break;
    default: printf("ERROR"); break;
  }
}
