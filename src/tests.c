// NOTE: Refer to <https://github.com/nemequ/munit/blob/master/example.c> for
// information on using our testing library.

#include <munit.h>
#include <stdio.h>

static MunitResult test_example(const MunitParameter params[], void* data) {
  munit_assert(2 + 2 == 4);
  return MUNIT_OK;
}

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

static const MunitSuite test_suite = {
  .prefix = "/penguins",
  .tests = test_suite_tests,
  .suites = NULL,
  .iterations = 1,
  .options = MUNIT_SUITE_OPTION_NONE,
};

int main(int argc, char* argv[]) {
  return munit_suite_main(&test_suite, NULL, argc, argv);
}
