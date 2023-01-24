#include<stdio.h>
#include "game.h"
#include "color.h"




void red() {
  printf("\033[1;41m");
}
void yellow() {
  printf("\033[1;43m");
}
void green() {
  printf("\033[1;42m");
}
void blue() {
  printf("\033[1;44m");
}
void magenta() {
  printf("\033[1;45m");
}


void water_color() {
  printf("\033[1;46;37m");
}

void ice_color() {
  printf("\033[1;47;37m");
}

void reset_color() {
  printf("\033[0m");
}



Colors check_color(Player* current_player){
  if(current_player->color==1){
    return RED;
  }else if(current_player->color==2){
    return GREEN;
  }else if(current_player->color==3){
    return YELLOW;
  }else if(current_player->color==4){
    return BLUE;
  }else if(current_player->color==5){
    return MAGENTA;
  }
}

void player_color(Player* current_player) {
  Colors check=check_color(current_player);
  switch(check){
    case RED: red(); break;
    case GREEN: green(); break;
    case YELLOW: yellow(); break;
    case BLUE: blue(); break;
    case MAGENTA: magenta(); break;
    default: printf("ERROR"); break;
  }
}