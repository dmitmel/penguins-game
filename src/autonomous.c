#include "autonomous.h"
#include "arguments.h"
#include "board.h"
#include "game.h"
#include "movement.h"
#include "placement.h"
#include "random.h"
#include "utils.h"
#include <ctype.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char* MY_AUTONOMOUS_PLAYER_NAME = "102D";

int run_autonomous_mode(const Arguments* args) {
  const char* my_player_name = args->set_name != NULL ? args->set_name : MY_AUTONOMOUS_PLAYER_NAME;
  if (args->action == ACTION_ARG_PRINT_NAME) {
    printf("%s\n", my_player_name);
    return EXIT_OK;
  }

  Game* game = game_new();
  FILE *input_file, *output_file;

  game_begin_setup(game);
  if ((input_file = fopen(args->input_board_file, "r")) == NULL) {
    perror("Failed to open the input board file");
    return EXIT_INTERNAL_ERROR;
  }
  int penguins_arg = args->action == ACTION_ARG_PLACEMENT ? args->penguins : 0;
  if (!load_game_state(game, input_file, penguins_arg, my_player_name)) {
    return EXIT_INPUT_FILE_ERROR;
  }
  fclose(input_file);
  game_end_setup(game);

  int my_player_index = -1;
  for (int i = 0; i < game->players_count; i++) {
    if (strcmp(game_get_player(game, i)->name, my_player_name) == 0) {
      my_player_index = i;
      break;
    }
  }
  if (my_player_index < 0) {
    fprintf(stderr, "Failed to add our player to the game\n");
    return EXIT_INTERNAL_ERROR;
  }

  bool move_done = false;
  if (args->action == ACTION_ARG_PLACEMENT) {
    placement_begin(game);
    move_done = do_autonomous_placement(game, my_player_index);
    placement_end(game);
  } else if (args->action == ACTION_ARG_MOVEMENT) {
    movement_begin(game);
    move_done = do_autonomous_movement(game, my_player_index);
    movement_end(game);
  }

  if ((output_file = fopen(args->output_board_file, "w")) == NULL) {
    perror("Failed to open the output board file");
    return EXIT_INTERNAL_ERROR;
  }
  if (!save_game_state(game, output_file)) {
    return EXIT_INTERNAL_ERROR;
  }
  fflush(output_file);
  fclose(output_file);

  game_free(game);

  return move_done ? EXIT_OK : EXIT_NO_POSSIBLE_MOVES;
}

bool do_autonomous_placement(Game* game, int my_player_index) {
  game->current_player_index = my_player_index - 1;
  if (placement_switch_player(game) != my_player_index) {
    return false;
  }
  if (game_get_current_player(game)->penguins_count < game->penguins_per_player) {
    for (int y = 0; y < game->board_height; y++) {
      for (int x = 0; x < game->board_width; x++) {
        Coords coords = { x, y };
        if (validate_placement(game, coords) == PLACEMENT_VALID) {
          place_penguin(game, coords);
          return true;
        }
      }
    }
  }
  return false;
}

bool do_autonomous_movement(Game* game, int my_player_index) {
  game->current_player_index = my_player_index - 1;
  if (movement_switch_player(game) != my_player_index) {
    return false;
  }
  Player* player = game_get_current_player(game);
  for (int i = 0; i < player->penguins_count; i++) {
    Coords penguin = player->penguins[i];
    PossibleMoves moves = calculate_all_possible_moves(game, penguin);
    if (moves.steps_right + moves.steps_down + moves.steps_left + moves.steps_up == 0) {
      continue;
    }
    int dx = 0, dy = 0, steps = 0;
    do {
      // clang-format off
      switch (random_range(0, 3)) {
        case 0: dx = 1;  dy = 0;  steps = moves.steps_right; break;
        case 1: dx = 0;  dy = 1;  steps = moves.steps_down;  break;
        case 2: dx = -1; dy = 0;  steps = moves.steps_left;  break;
        case 3: dx = 0;  dy = -1; steps = moves.steps_up;    break;
      }
      // clang-format on
    } while (steps == 0);
    steps = random_range(1, steps);
    Coords target = penguin;
    while (steps > 0) {
      target.x += dx, target.y += dy;
      steps--;
    }
    move_penguin(game, penguin, target);
    return true;
  }
  return false;
}

static int read_line(FILE* file, char** buf, int* line_len) {
  *line_len = 0;
  int buf_size = 64;
  *buf = realloc(*buf, buf_size);
  int c;
  while ((c = fgetc(file)) != EOF) {
    if (*line_len + 1 >= buf_size) {
      buf_size *= 2;
      *buf = realloc(*buf, buf_size);
    }
    (*buf)[*line_len] = (char)c;
    *line_len += 1;
    if (c == '\n') {
      break;
    }
  }
  (*buf)[*line_len] = '\0';
  return *line_len;
}

bool load_game_state(Game* game, FILE* file, int penguins_arg, const char* my_player_name) {
  char* line_buf = NULL;
  int line_len = 0;

  read_line(file, &line_buf, &line_len);
  int board_width, board_height;
  if (sscanf(line_buf, "%d %d", &board_height, &board_width) != 2) {
    return false;
  }
  if (!(board_width > 0 && board_height > 0)) {
    return false;
  }
  setup_board(game, board_width, board_height);

  static const int MAX_PLAYERS = 9; // This is specified by the file format
  char* player_names[MAX_PLAYERS];
  int player_scores[MAX_PLAYERS];
  int player_penguins[MAX_PLAYERS];

  for (int i = 0; i < MAX_PLAYERS; i++) {
    player_penguins[i] = 0;
  }

  for (int y = 0; y < board_height; y++) {
    read_line(file, &line_buf, &line_len);
    char* str = line_buf;
    // Skip the leading whitespace
    while (isspace(*str)) str++;
    for (int x = 0; x < board_width; x++) {
      Coords coords = { x, y };

      if (str[0] == '\0' || str[1] == '\0') {
        // Reached the end of the string prematurely
        set_tile(game, coords, 0);
        continue;
      }
      char c1 = str[0], c2 = str[1];
      str += 2;

      if ('0' <= c1 && c1 <= '9' && c2 == '0') {
        int fish = c1 - '0';
        set_tile(game, coords, fish);
      } else if ('1' <= c2 && c2 <= '9' && c1 == '0') {
        int player_id = c2 - '0';
        set_tile(game, coords, -player_id);
        player_penguins[player_id - 1] += 1;
      } else {
        // Invalid
      }

      // Skip the whitespace separators
      while (isspace(*str)) str++;
    }
  }

  int players_count = 0;
  for (int i = 0; i < MAX_PLAYERS; i++) {
    read_line(file, &line_buf, &line_len);
    char name[256];
    int id;
    int points;
    if (sscanf(line_buf, "%255s %d %d", name, &id, &points) != 3) {
      break;
    }
    // TODO: Handle non-consequential IDs
    if (id == i + 1) {
      player_names[i] = strdup(name);
      player_scores[i] = points;
      players_count += 1;
    }
  }

  bool my_player_found = false;
  for (int i = 0; i < players_count; i++) {
    if (strcmp(player_names[i], my_player_name) == 0) {
      my_player_found = true;
      break;
    }
  }
  if (!my_player_found && players_count < MAX_PLAYERS) {
    int i = players_count;
    player_names[i] = strdup(my_player_name);
    player_scores[i] = 0;
    players_count += 1;
  } else {
    // Not enough place to insert our own player
  }

  int penguins_per_player = my_max(penguins_arg, 1);
  for (int i = 0; i < players_count; i++) {
    penguins_per_player = my_max(penguins_per_player, player_penguins[i]);
  }
  game_set_penguins_per_player(game, penguins_per_player);

  game_set_players_count(game, players_count);
  for (int i = 0; i < players_count; i++) {
    game_set_player_name(game, i, player_names[i]);
    Player* player = game_get_player(game, i);
    player->points = player_scores[i];
    for (int y = 0; y < game->board_height; y++) {
      for (int x = 0; x < game->board_width; x++) {
        Coords coords = { x, y };
        if (get_tile(game, coords) == -player->id) {
          game_add_player_penguin(game, i, coords);
        }
      }
    }
  }

  free(line_buf);
  for (int i = 0; i < players_count; i++) {
    free(player_names[i]);
  }
  return true;
}

bool save_game_state(const Game* game, FILE* file) {
  fprintf(file, "%d %d\n", game->board_height, game->board_width);

  for (int y = 0; y < game->board_height; y++) {
    for (int x = 0; x < game->board_width; x++) {
      Coords coords = { x, y };
      int tile = get_tile(game, coords);
      if (x != 0) {
        fprintf(file, " ");
      }
      if (0 <= tile && tile <= 9) {
        int fish = tile;
        fprintf(file, "%c0", '0' + fish);
      } else if (-9 <= tile && tile <= -1) {
        int player_id = -tile;
        fprintf(file, "0%c", '0' + player_id);
      }
    }
    fprintf(file, "\n");
  }

  for (int i = 0; i < game->players_count; i++) {
    Player* player = game_get_player(game, i);
    fprintf(file, "%s %d %d\n", player->name, player->id, player->points);
  }

  return true;
}
