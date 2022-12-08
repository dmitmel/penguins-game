// NOTE: Refer to <https://github.com/nemequ/munit/blob/master/example.c> for
// information on using our testing library.
#define MUNIT_ENABLE_ASSERT_ALIASES

#include <munit.h>
#include <stdio.h>

#include "board.h"
#include "random.h"

// an implementation of the function defined in random.h and used in board.c
int random_range(int min, int max) {
  return munit_rand_int_range(min, max);
}

static MunitResult test_placeable_spot_exists(const MunitParameter* params, void* data) {
  int* grid[2] = { (int[3]){ 0, 1, 2 }, (int[3]){ -1, 2, 1 } };
  Board board = { .height = 2, .width = 3, .grid = grid };

  assert_true(placeable_spot_exists(&board));
  return MUNIT_OK;
}

static MunitResult test_placeable_spot_does_not_exist(const MunitParameter* params, void* data) {
  int* grid[2] = { (int[3]){ 0, 0, 2 }, (int[3]){ -1, -2, 3 } };
  Board board = { .height = 2, .width = 3, .grid = grid };

  assert_false(placeable_spot_exists(&board));
  return MUNIT_OK;
}

static MunitResult
test_valid_movement_exists_for_player(const MunitParameter* params, void* data) {
  int* grid[2] = { (int[3]){ -1, 0, -2 }, (int[3]){ 0, 1, 3 } };
  Board board = { .height = 2, .width = 3, .grid = grid };

  assert_true(any_valid_player_move_exists(&board, 2));
  return MUNIT_OK;
}

static MunitResult
test_no_valid_movement_exists_for_player(const MunitParameter* params, void* data) {
  int* grid[2] = { (int[3]){ -1, 0, -2 }, (int[3]){ 0, 1, 3 } };
  Board board = { .height = 2, .width = 3, .grid = grid };

  assert_false(any_valid_player_move_exists(&board, 1));
  return MUNIT_OK;
}

static MunitResult test_detect_valid_movement(const MunitParameter* params, void* data) {
  int* grid[1] = { (int[3]){ -1, 1, 3 } };
  Board board = { .height = 1, .width = 3, .grid = grid };

  assert_true(movement_is_valid(&board, 0, 0, 2, 0));
  return MUNIT_OK;
}

static MunitResult
test_movement_over_empty_space_invalid(const MunitParameter* params, void* data) {
  int* grid[1] = { (int[3]){ -1, 0, 3 } };
  Board board = { .height = 1, .width = 3, .grid = grid };

  assert_false(movement_is_valid(&board, 0, 0, 2, 0));
  return MUNIT_OK;
}

static MunitResult test_movement_over_penguin_invalid(const MunitParameter* params, void* data) {
  int* grid[1] = { (int[3]){ -1, 2, 3 } };
  Board board = { .height = 1, .width = 3, .grid = grid };

  assert_false(movement_is_valid(&board, 0, 0, 2, 0));
  return MUNIT_OK;
}

static MunitResult
test_move_penguin_and_calculate_points(const MunitParameter* params, void* data) {
  int* grid[1] = { (int[3]){ -1, 1, 3 } };
  Board board = { .height = 1, .width = 3, .grid = grid };

  assert_int(move_penguin(&board, 0, 0, 2, 0, 1), ==, 4);
  assert_int(board.grid[0][0], ==, 0);
  assert_int(board.grid[0][1], ==, 0);
  assert_int(board.grid[0][2], ==, -1);
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
