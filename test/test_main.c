#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "../src/utils/rng.h"
#include "modules/test_auth.h"
#include "modules/test_branch.h"
#include "utils/test_date_util.h"
#include "utils/test_file_util.h"
#include "utils/test_hash.h"
#include "utils/test_input.h"
#include "utils/test_string_util.h"

int main()
{
  srand((unsigned int)time(NULL));
  seed_rng((unsigned int)time(NULL));
  run_all_string_util_tests();
  run_all_file_util_tests();
  run_all_input_tests();
  run_all_hash_tests();
  run_all_auth_tests();
  run_all_date_util_tests();
  run_all_branch_tests();
  printf("All tests passed.\n");
  return 0;
}
