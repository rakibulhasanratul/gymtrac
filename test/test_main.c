#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "test_auth.h"
#include "test_file_util.h"
#include "test_hash.h"
#include "test_input.h"
#include "test_string_util.h"

int main()
{
  srand((unsigned int)time(NULL));
  run_all_string_util_tests();
  run_all_file_util_tests();
  run_all_input_tests();
  run_all_hash_tests();
  run_all_auth_tests();
  printf("All tests passed.\n");
  return 0;
}
