// NOTE: Refer to <https://github.com/nemequ/munit/blob/master/example.c> for
// information on using our testing library.

#include "board.h"
#include "game.h"
#include "movement.h"
#include "placement.h"
#include "utils.h"
#include <munit.h>
#include <stdio.h>

static MunitResult test_game_clone(const MunitParameter* params, void* data) {
  UNUSED(params), UNUSED(data);
  Game* game = game_new();
  game_begin_setup(game);
  setup_board(game, 20, 20);
  game_set_players_count(game, 3);
  game_set_player_name(game, 0, "A");
  game_set_player_name(game, 1, "B");
  game_set_player_name(game, 2, NULL);
  game_set_penguins_per_player(game, 2);
  Game* clone = game_clone(game);
  game_free(game);
  game_free(clone);
  return MUNIT_OK;
}

static void setup_test_game(
  Game* game,
  int players_count,
  int penguins_per_player,
  int board_width,
  int board_height,
  // The encoding for this is:
  // ~    - a water tile
  // 0-9  - a fish tile
  // A-Z  - a penguin tile
  const char* board_data
) {
  game_begin_setup(game);
  game_set_players_count(game, players_count);
  game_set_penguins_per_player(game, penguins_per_player);
  for (int i = 0; i < players_count; i++) {
    char name[] = { 'A' + i, '\0' };
    game_set_player_name(game, i, name);
  }
  setup_board(game, board_width, board_height);
  for (int y = 0; y < board_height; y++) {
    for (int x = 0; x < board_width; x++) {
      Coords coords = { x, y };
      char tile = *board_data;
      if ('1' <= tile && tile <= '9') {
        short fish = tile - '0';
        set_tile(game, coords, fish);
      } else if ('A' <= tile && tile <= 'Z') {
        int player_idx = tile - 'A';
        Player* player = game_get_player(game, player_idx);
        set_tile(game, coords, PENGUIN_TILE(player->id));
        game_add_player_penguin(game, player_idx, coords);
      } else {
        set_tile(game, coords, WATER_TILE);
      }
      if (tile != '\0') {
        board_data++;
      }
    }
  }
  game_end_setup(game);
}

static MunitResult test_placeable_spot_exists(const MunitParameter* params, void* data) {
  UNUSED(params), UNUSED(data);
  Game* game = game_new();
  const char* board = "~12"
                      "A21";
  setup_test_game(game, /*players*/ 1, /*penguins*/ 1, /*width*/ 3, /*height*/ 2, board);
  munit_assert_true(any_valid_placement_exists(game));
  game_free(game);
  return MUNIT_OK;
}

static MunitResult test_placeable_spot_does_not_exist(const MunitParameter* params, void* data) {
  UNUSED(params), UNUSED(data);
  Game* game = game_new();
  const char* board = "~~2"
                      "AB3";
  setup_test_game(game, /*players*/ 2, /*penguins*/ 1, /*width*/ 3, /*height*/ 2, board);
  munit_assert_false(any_valid_placement_exists(game));
  game_free(game);
  return MUNIT_OK;
}

static MunitResult
test_valid_movement_exists_for_player(const MunitParameter* params, void* data) {
  UNUSED(params), UNUSED(data);
  Game* game = game_new();
  const char* board = "B~A"
                      "~13";
  setup_test_game(game, /*players*/ 2, /*penguins*/ 1, /*width*/ 3, /*height*/ 2, board);
  munit_assert_true(any_valid_player_move_exists(game, 0));
  game_free(game);
  return MUNIT_OK;
}

static MunitResult
test_no_valid_movement_exists_for_player(const MunitParameter* params, void* data) {
  UNUSED(params), UNUSED(data);
  Game* game = game_new();
  const char* board = "B~A"
                      "~13";
  setup_test_game(game, /*players*/ 2, /*penguins*/ 1, /*width*/ 3, /*height*/ 2, board);
  munit_assert_false(any_valid_player_move_exists(game, 1));
  game_free(game);
  return MUNIT_OK;
}

static MunitResult test_detect_valid_movement(const MunitParameter* params, void* data) {
  UNUSED(params), UNUSED(data);
  Game* game = game_new();
  const char* board = "A13";
  setup_test_game(game, /*players*/ 1, /*penguins*/ 1, /*width*/ 3, /*height*/ 1, board);
  game_set_current_player(game, 0);
  Coords fail = { -1, -1 };
  MovementError result = validate_movement(game, (Coords){ 0, 0 }, (Coords){ 2, 0 }, &fail);
  munit_assert_int(result, ==, MOVEMENT_VALID);
  game_free(game);
  return MUNIT_OK;
}

static MunitResult
test_movement_over_empty_space_invalid(const MunitParameter* params, void* data) {
  UNUSED(params), UNUSED(data);
  Game* game = game_new();
  const char* board = "1A~3";
  setup_test_game(game, /*players*/ 1, /*penguins*/ 1, /*width*/ 4, /*height*/ 1, board);
  game_set_current_player(game, 0);
  Coords fail = { -1, -1 };
  MovementError result = validate_movement(game, (Coords){ 1, 0 }, (Coords){ 3, 0 }, &fail);
  munit_assert_int(result, ==, MOVEMENT_OVER_EMPTY_TILE);
  game_free(game);
  return MUNIT_OK;
}

static MunitResult test_movement_over_penguin_invalid(const MunitParameter* params, void* data) {
  UNUSED(params), UNUSED(data);
  Game* game = game_new();
  const char* board = "1AB3";
  setup_test_game(game, /*players*/ 2, /*penguins*/ 1, /*width*/ 4, /*height*/ 1, board);
  game_set_current_player(game, 0);
  Coords fail = { -1, -1 };
  MovementError result = validate_movement(game, (Coords){ 1, 0 }, (Coords){ 3, 0 }, &fail);
  munit_assert_int(result, ==, MOVEMENT_OVER_PENGUIN);
  game_free(game);
  return MUNIT_OK;
}

static MunitResult
test_move_penguin_and_calculate_points(const MunitParameter* params, void* data) {
  UNUSED(params), UNUSED(data);
  Game* game = game_new();
  const char* board = "A13";
  setup_test_game(game, /*players*/ 1, /*penguins*/ 1, /*width*/ 3, /*height*/ 1, board);
  movement_begin(game);
  game_set_current_player(game, 0);
  move_penguin(game, (Coords){ 0, 0 }, (Coords){ 2, 0 });
  munit_assert_int(get_tile(game, (Coords){ 0, 0 }), ==, WATER_TILE);
  munit_assert_int(get_tile(game, (Coords){ 1, 0 }), ==, FISH_TILE(1));
  munit_assert_int(get_tile(game, (Coords){ 2, 0 }), ==, PENGUIN_TILE(1));
  munit_assert_int(game_get_player(game, 0)->points, ==, 3);
  game_free(game);
  return MUNIT_OK;
}

static MunitTest board_suite_tests[] = {
  {
    .name = "/cloning the Game produces a deep copy",
    .test = test_game_clone,
  },
  {
    .name = "/detects that a placeable spot exists somewhere on the board",
    .test = test_placeable_spot_exists,
    .setup = NULL,
    .tear_down = NULL,
    .options = MUNIT_TEST_OPTION_NONE,
    .parameters = NULL,
  },
  {
    .name = "/detects that there are no valid placement slots on the board",
    .test = test_placeable_spot_does_not_exist,
    .setup = NULL,
    .tear_down = NULL,
    .options = MUNIT_TEST_OPTION_NONE,
    .parameters = NULL,
  },
  {
    .name = "/detects that there are valid movement options for a player",
    .test = test_valid_movement_exists_for_player,
    .setup = NULL,
    .tear_down = NULL,
    .options = MUNIT_TEST_OPTION_NONE,
    .parameters = NULL,
  },
  {
    .name = "/detects that there are no valid movement options for a player",
    .test = test_no_valid_movement_exists_for_player,
    .setup = NULL,
    .tear_down = NULL,
    .options = MUNIT_TEST_OPTION_NONE,
    .parameters = NULL,
  },
  {
    .name = "/detects that a move is going to be valid",
    .test = test_detect_valid_movement,
    .setup = NULL,
    .tear_down = NULL,
    .options = MUNIT_TEST_OPTION_NONE,
    .parameters = NULL,
  },
  {
    .name = "/detects that a move over an empty space is going to be invalid",
    .test = test_movement_over_empty_space_invalid,
    .setup = NULL,
    .tear_down = NULL,
    .options = MUNIT_TEST_OPTION_NONE,
    .parameters = NULL,
  },
  {
    .name = "/detects that a move over a penguin is going to be invalid",
    .test = test_movement_over_penguin_invalid,
    .setup = NULL,
    .tear_down = NULL,
    .options = MUNIT_TEST_OPTION_NONE,
    .parameters = NULL,
  },
  {
    .name = "/updates the board and returns number of fish captured during a movement",
    .test = test_move_penguin_and_calculate_points,
    .setup = NULL,
    .tear_down = NULL,
    .options = MUNIT_TEST_OPTION_NONE,
    .parameters = NULL,
  },
  // Marker of the end of the array, don't touch.
  { NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
};

static const MunitSuite board_suite = {
  .prefix = "/board",
  .tests = board_suite_tests,
  .suites = NULL,
  .iterations = 1,
  .options = MUNIT_SUITE_OPTION_NONE,
};

int main(int argc, char* argv[]) {
  return munit_suite_main(&board_suite, NULL, argc, argv);
}
