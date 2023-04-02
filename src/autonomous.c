#include "autonomous.h"
#include "arguments.h"
#include "board.h"
#include "bot.h"
#include "game.h"
#include "movement.h"
#include "placement.h"
#include "utils.h"
#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char* MY_AUTONOMOUS_PLAYER_NAME = "102D";

// These are specified by the board file format
#define MAX_PLAYERS 9
#define MIN_PLAYER_ID 1
#define MAX_PLAYER_ID 9

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
    fprintf(stderr, "Failed to parse the input file\n");
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
  assert(my_player_index >= 0);

  bool move_ok = false;
  BotState* bot = bot_state_new(&args->bot, game);
  if (args->action == ACTION_ARG_PLACEMENT) {
    placement_begin(game);
    game->current_player_index = my_player_index - 1;
    if (placement_switch_player(game) == my_player_index) {
      Coords target;
      move_ok = bot_make_placement(bot, &target);
      if (move_ok) {
        place_penguin(game, target);
      }
    }
    placement_end(game);
  } else if (args->action == ACTION_ARG_MOVEMENT) {
    movement_begin(game);
    game->current_player_index = my_player_index - 1;
    if (movement_switch_player(game) == my_player_index) {
      Coords penguin, target;
      move_ok = bot_make_move(bot, &penguin, &target);
      if (move_ok) {
        move_penguin(game, penguin, target);
      }
    }
    movement_end(game);
  }
  bot_state_free(bot);

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

  return move_ok ? EXIT_OK : EXIT_NO_POSSIBLE_MOVES;
}

static size_t read_line(FILE* file, char** buf, size_t* line_len) {
  *line_len = 0;
  size_t buf_size = 64;
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
  size_t line_len = 0;

  read_line(file, &line_buf, &line_len);
  int board_width, board_height;
  if (sscanf(line_buf, "%d %d", &board_height, &board_width) != 2) {
    fprintf(stderr, "Failed to parse the board size line: '%s'\n", line_buf);
    return false;
  }
  if (!(board_width > 0 && board_height > 0)) {
    fprintf(stderr, "Invalid board size: %d %d\n", board_width, board_height);
    return false;
  }
  setup_board(game, board_width, board_height);

  short player_ids[MAX_PLAYERS];
  char* player_names[MAX_PLAYERS];
  int player_scores[MAX_PLAYERS];

  int player_penguins_by_id[MAX_PLAYER_ID - MIN_PLAYER_ID + 1];
  bool taken_player_ids[MAX_PLAYER_ID - MIN_PLAYER_ID + 1];
  for (int i = MIN_PLAYER_ID; i <= MAX_PLAYER_ID; i++) {
    player_penguins_by_id[i - MIN_PLAYER_ID] = 0;
    taken_player_ids[i - MIN_PLAYER_ID] = false;
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
        set_tile(game, coords, WATER_TILE);
        continue;
      }
      char c1 = str[0], c2 = str[1];
      str += 2;

      if ('0' <= c1 && c1 <= '9' && c2 == '0') {
        short fish = c1 - '0';
        set_tile(game, coords, FISH_TILE(fish));
      } else if ('1' <= c2 && c2 <= '9' && c1 == '0') {
        short player_id = c2 - '0';
        set_tile(game, coords, PENGUIN_TILE(player_id));
        player_penguins_by_id[player_id - MIN_PLAYER_ID] += 1;
      } else {
        fprintf(stderr, "Invalid tile at x=%d y=%d: '%c%c'\n", x, y, c1, c2);
        return false;
      }

      // Skip the whitespace separators
      while (isspace(*str)) str++;
    }
  }

  int players_count = 0;
  for (int linenr = 0; linenr < MAX_PLAYERS; linenr++) {
    if (!read_line(file, &line_buf, &line_len)) {
      break;
    }
    char name[256];
    int id;
    int points;
    if (sscanf(line_buf, "%255s %d %d", name, &id, &points) != 3) {
      fprintf(stderr, "Failed to parse the player on line %d: '%s'\n", linenr, line_buf);
      return false;
    }
    if (!(MIN_PLAYER_ID <= id && id <= MAX_PLAYER_ID)) {
      fprintf(stderr, "Player ID on line %d falls out of the acceptable range: %d\n", linenr, id);
      return false;
    }
    if (taken_player_ids[id]) {
      fprintf(stderr, "Player ID on line %d is a duplicate: %d\n", linenr, id);
      return false;
    }
    if (*name == '\0') {
      fprintf(stderr, "Player name on line %d is empty\n", linenr);
      return false;
    }
    taken_player_ids[id] = true;
    int i = players_count;
    player_ids[i] = (short)id;
    player_names[i] = strdup(name);
    player_scores[i] = points;
    players_count += 1;
  }

  bool my_player_found = false;
  for (int i = 0; i < players_count; i++) {
    if (strcmp(player_names[i], my_player_name) == 0) {
      my_player_found = true;
      break;
    }
  }
  if (!my_player_found) {
    short free_id = -1;
    for (short id = MIN_PLAYER_ID; id <= MAX_PLAYER_ID; id++) {
      if (!taken_player_ids[id]) {
        free_id = id;
        break;
      }
    }
    if (free_id <= 0 || players_count >= MAX_PLAYERS) {
      fprintf(stderr, "No IDs left in the input file to assign to our own player\n");
      return false;
    }
    int i = players_count;
    player_ids[i] = free_id;
    player_names[i] = strdup(my_player_name);
    player_scores[i] = 0;
    players_count += 1;
  }

  int penguins_per_player = my_max(penguins_arg, 1);
  for (int i = 0; i < players_count; i++) {
    penguins_per_player = my_max(penguins_per_player, player_penguins_by_id[player_ids[i]]);
  }
  game_set_penguins_per_player(game, penguins_per_player);

  game_set_players_count(game, players_count);
  for (int i = 0; i < players_count; i++) {
    game_set_player_name(game, i, player_names[i]);
    Player* player = game_get_player(game, i);
    player->id = player_ids[i];
    player->points = player_scores[i];
    for (int y = 0; y < game->board_height; y++) {
      for (int x = 0; x < game->board_width; x++) {
        Coords coords = { x, y };
        short tile = get_tile(game, coords);
        if (get_tile_player_id(tile) == player->id) {
          game_add_player_penguin(game, i, coords);
        }
      }
    }
    assert(player->penguins_count == player_penguins_by_id[player->id - MIN_PLAYER_ID]);
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
      short tile = get_tile(game, coords);
      if (x != 0) {
        fprintf(file, " ");
      }
      if (0 <= tile && tile <= 9) {
        short fish = tile;
        fprintf(file, "%c0", '0' + fish);
      } else if (-9 <= tile && tile <= -1) {
        short player_id = -tile;
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
