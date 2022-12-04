// NOTE: Refer to <https://github.com/nemequ/munit/blob/master/example.c> for
// information on using our testing library.

#include <munit.h>
#include <stdio.h>

#include "board.h"
#include "random.h"

// an implementation of the function defined in random.h and used in board.c
int random_range(int min, int max) {
  return munit_rand_int_range(min, max);
}

static MunitResult test_example(const MunitParameter params[], void* data) {
  munit_assert(2 + 2 == 4);
  munit_assert(random_range(0, 1) == 0);
  return MUNIT_OK;
}

static MunitResult test_placeable_spot_exists(const MunitParameter params, void* data) {
  int* grid[2] = { (int[3]){ 0, 1, 2 }, (int[3]){ -1, 2, 1 } };

  Board board = { .height = 2, .width = 3, .grid = grid };

  munit_assert_true(placeable_spot_exists(&board));
  return MUNIT_OK;
}

static MunitResult test_placeable_spot_does_not_exist(const MunitParameter params, void* data) {
  int* grid[2] = { (int[3]){ 0, 0, 2 }, (int[3]){ -1, 2, 1 } };

  Board board = { .height = 2, .width = 3, .grid = grid };

  // munit_assert_false(placeable_spot_exists(&board));
  return MUNIT_OK;
}

static MunitTest board_suite_tests[] = {
  {
    .name = "/placeable_spot_exists",
    .test = test_placeable_spot_exists,
    .setup = NULL,
    .tear_down = NULL,
    .options = MUNIT_TEST_OPTION_NONE,
    .parameters = NULL,
  },
  {
    .name = "/detects that there are no valid placement slots",
    .test = test_placeable_spot_does_not_exist,
    .setup = NULL,
    .tear_down = NULL,
    .options = MUNIT_TEST_OPTION_NONE,
    .parameters = NULL,
  },
  { NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL },
};

static MunitTest test_suite_tests[] = {
  {
    .name = "/example",
    .test = test_example,
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

// static const MunitSuite test_suite = {
//   .prefix = "/penguins",
//   .tests = test_suite_tests,
//   .suites = board_suite,
//   .iterations = 1,
//   .options = MUNIT_SUITE_OPTION_NONE,
// };

int main(int argc, char* argv[]) {
  return munit_suite_main(&board_suite, NULL, argc, argv);
}
