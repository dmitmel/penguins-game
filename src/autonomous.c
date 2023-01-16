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
  Game* game = game_new();
  FILE *input_file, *output_file;

  game_begin_setup(game);
  if ((input_file = fopen(args->input_board_file, "r")) == NULL) {
    perror("Failed to open the input board file");
    return EXIT_INTERNAL_ERROR;
  }
  int penguins_arg = args->phase == PHASE_ARG_PLACEMENT ? args->penguins : 0;
  if (!load_game_state(game, input_file, penguins_arg)) {
    return EXIT_INPUT_FILE_ERROR;
  }
  fclose(input_file);
  game_end_setup(game);

  if (args->phase == PHASE_ARG_PLACEMENT) {
    placement_begin(game);
  } else if (args->phase == PHASE_ARG_MOVEMENT) {
    movement_begin(game);
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

  return EXIT_OK;
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

bool load_game_state(Game* game, FILE* file, int penguins_arg) {
  char* line_buf = NULL;
  int line_len = 0;

  read_line(file, &line_buf, &line_len);
  int board_width, board_height;
  sscanf(line_buf, "%d %d", &board_height, &board_width);
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
    if (id == i + 1) {
      player_names[i] = strdup(name);
      player_scores[i] = points;
      players_count += 1;
    }
  }

  bool my_player_found = false;
  for (int i = 0; i < players_count; i++) {
    if (strcmp(player_names[i], MY_AUTONOMOUS_PLAYER_NAME) == 0) {
      my_player_found = true;
      break;
    }
  }
  if (!my_player_found && players_count < MAX_PLAYERS) {
    int i = players_count;
    player_names[i] = strdup(MY_AUTONOMOUS_PLAYER_NAME);
    player_scores[i] = 0;
    players_count += 1;
  }

  game_set_players_count(game, players_count);
  for (int i = 0; i < players_count; i++) {
    game_set_player_name(game, i, player_names[i]);
    Player* player = game_get_player(game, i);
    player->points = player_scores[i];
    player->penguins = player_penguins[i];
  }

  game->penguins_per_player = penguins_arg > 0 ? penguins_arg : 0;

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
        fprintf(file, "%d0", fish);
      } else if (-9 <= tile && tile <= -1) {
        int player_id = -tile;
        fprintf(file, "0%d", player_id);
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
