// NOTE: Refer to <https://github.com/nemequ/munit/blob/master/example.c> for
// information on using our testing library.
#define MUNIT_ENABLE_ASSERT_ALIASES

#include "board.h"
#include "movement.h"
#include "placement.h"
#include "random.h"

#include <munit.h>
#include <stdio.h>

// an implementation of the function defined in random.h and used in board.c
int random_range(int min, int max) {
  return munit_rand_int_range(min, max);
}

static MunitResult
test_placeable_spot_exists(const MunitParameter* UNUSED(params), void* UNUSED(data)) {
  int grid[] = { 0,  1, 2, //
                 -1, 2, 1 };
  Game game = { .phase = GAME_PHASE_PLACEMENT,
                .players = NULL,
                .players_count = 1,
                .penguins_per_player = 1,
                .board_width = 3,
                .board_height = 2,
                .board_grid = grid,
                .current_player_index = 0 };
  assert_true(any_valid_placement_exists(&game));
  return MUNIT_OK;
}

static MunitResult
test_placeable_spot_does_not_exist(const MunitParameter* UNUSED(params), void* UNUSED(data)) {
  int grid[] = { 0,  0,  2, //
                 -1, -2, 3 };
  Game game = { .phase = GAME_PHASE_PLACEMENT,
                .players = NULL,
                .players_count = 1,
                .penguins_per_player = 1,
                .board_width = 3,
                .board_height = 2,
                .board_grid = grid,
                .current_player_index = 0 };

  assert_false(any_valid_placement_exists(&game));
  return MUNIT_OK;
}

static MunitResult
test_valid_movement_exists_for_player(const MunitParameter* UNUSED(params), void* UNUSED(data)) {
  int grid[] = { -2, 0, -1, //
                 0,  1, 3 };
  Player players[] = { { .id = 1,
                         .name = "1",
                         .points = 0,
                         .penguins_count = 1,
                         .penguins = &((Coords){ 2, 0 }),
                         .moves_count = 0 },
                       { .id = 2,
                         .name = "2",
                         .points = 0,
                         .penguins_count = 1,
                         .penguins = &((Coords){ 0, 0 }),
                         .moves_count = 0 } };
  Game game = { .phase = GAME_PHASE_MOVEMENT,
                .players = players,
                .players_count = 2,
                .penguins_per_player = 1,
                .board_width = 3,
                .board_height = 2,
                .board_grid = grid,
                .current_player_index = 0 };

  assert_true(any_valid_player_move_exists(&game, 0));
  return MUNIT_OK;
}

static MunitResult test_no_valid_movement_exists_for_player(
  const MunitParameter* UNUSED(params), void* UNUSED(data)
) {
  int grid[] = { -2, 0, -1, //
                 0,  1, 3 };
  Player players[] = { { .id = 1,
                         .name = "1",
                         .points = 0,
                         .penguins_count = 1,
                         .penguins = &((Coords){ 2, 0 }),
                         .moves_count = 0 },
                       { .id = 2,
                         .name = "2",
                         .points = 0,
                         .penguins_count = 1,
                         .penguins = &((Coords){ 0, 0 }),
                         .moves_count = 0 } };
  Game game = { .phase = GAME_PHASE_MOVEMENT,
                .players = players,
                .players_count = 2,
                .penguins_per_player = 1,
                .board_width = 3,
                .board_height = 2,
                .board_grid = grid,
                .current_player_index = 1 };

  assert_false(any_valid_player_move_exists(&game, 1));
  return MUNIT_OK;
}

static MunitResult
test_detect_valid_movement(const MunitParameter* UNUSED(params), void* UNUSED(data)) {
  int grid[] = { -1, 1, 3 };
  Player players[] = { { .id = 1,
                         .name = "1",
                         .points = 0,
                         .penguins_count = 1,
                         .penguins = &((Coords){ 0, 0 }),
                         .moves_count = 0 } };
  Game game = { .phase = GAME_PHASE_MOVEMENT,
                .players = players,
                .players_count = 1,
                .penguins_per_player = 1,
                .board_width = 3,
                .board_height = 1,
                .board_grid = grid,
                .current_player_index = 0 };
  Coords fail = { -1, -1 };

  assert_true(validate_movement(&game, (Coords){ 0, 0 }, (Coords){ 2, 0 }, &fail) == VALID_INPUT);
  return MUNIT_OK;
}

static MunitResult
test_movement_over_empty_space_invalid(const MunitParameter* UNUSED(params), void* UNUSED(data)) {
  int grid[] = { -1, 0, 3 };
  Player players[] = { { .id = 1,
                         .name = "1",
                         .points = 0,
                         .penguins_count = 1,
                         .penguins = &((Coords){ 0, 0 }),
                         .moves_count = 0 } };
  Game game = { .phase = GAME_PHASE_MOVEMENT,
                .players = players,
                .players_count = 1,
                .penguins_per_player = 1,
                .board_width = 3,
                .board_height = 1,
                .board_grid = grid,
                .current_player_index = 0 };
  Coords fail = { -1, -1 };

  assert_true(
    validate_movement(&game, (Coords){ 0, 0 }, (Coords){ 2, 0 }, &fail) == MOVE_OVER_EMPTY_TILE
  );
  return MUNIT_OK;
}

static MunitResult
test_movement_over_penguin_invalid(const MunitParameter* UNUSED(params), void* UNUSED(data)) {
  int grid[] = { -1, -2, 3 };
  Player players[] = { { .id = 1,
                         .name = "1",
                         .points = 0,
                         .penguins_count = 1,
                         .penguins = &((Coords){ 0, 0 }),
                         .moves_count = 0 } };
  Game game = { .phase = GAME_PHASE_MOVEMENT,
                .players = players,
                .players_count = 1,
                .penguins_per_player = 1,
                .board_width = 3,
                .board_height = 1,
                .board_grid = grid,
                .current_player_index = 0 };
  Coords fail = { -1, -1 };

  assert_true(
    validate_movement(&game, (Coords){ 0, 0 }, (Coords){ 2, 0 }, &fail) == MOVE_OVER_PENGUIN
  );
  return MUNIT_OK;
}

static MunitResult
test_move_penguin_and_calculate_points(const MunitParameter* UNUSED(params), void* UNUSED(data)) {
  int grid[] = { -1, 1, 3 };
  Player players[] = { { .id = 1,
                         .name = "1",
                         .points = 0,
                         .penguins_count = 1,
                         .penguins = &((Coords){ 0, 0 }),
                         .moves_count = 0 } };
  Game game = { .phase = GAME_PHASE_MOVEMENT,
                .players = players,
                .players_count = 1,
                .penguins_per_player = 1,
                .board_width = 3,
                .board_height = 1,
                .board_grid = grid,
                .current_player_index = 0 };

  assert_int(move_penguin(&game, (Coords){ 0, 0 }, (Coords){ 2, 0 }), ==, 3);
  // assert_int(game.board_grid[0], ==, 0);
  // assert_int(game.board_grid[1], ==, 1);
  // assert_int(game.board_grid[2], ==, -1);
  return MUNIT_OK;
}

static MunitTest board_suite_tests[] = {
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
