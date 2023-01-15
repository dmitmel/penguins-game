#include "board.h"
#include "game.h"
#include "io.h"
#include "movement.h"
#include "placement.h"
#include "random.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

int main(int UNUSED(argc), char* argv[] UNUSED_ATTR) {
  random_init();

  clear_screen();

  Game* game = game_new();

  game_begin_setup(game);

  int players_count;
  get_players_count(&players_count);
  game_set_players_count(game, players_count);
  for (int i = 0; i < players_count; i++) {
    char name[32];
    get_player_name(i + 1, name);
    game_set_player_name(game, i, name);
  }

  int penguin_count;
  get_penguin_count(&penguin_count);
  game_set_penguins_per_player(game, penguin_count);

  int board_width;
  int board_height;
  get_board_dimensions(&board_width, &board_height);
  setup_board(game, board_width, board_height);
  generate_random_board(game);

  game_end_setup(game);

  interactive_placement(game);
  interactive_movement(game);
  game_end(game);

  game_free(game);

  return 0;
}
