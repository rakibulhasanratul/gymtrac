#include <stdio.h>

#include "test_file_util.h"
#include "test_input.h"
#include "test_string_util.h"

int main(void)
{
  run_all_string_util_tests();
  run_all_file_util_tests();
  run_all_input_tests();
  printf("All tests passed.\n");
  return 0;
}
