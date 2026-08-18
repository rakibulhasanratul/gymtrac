#include <stdio.h>
#include <time.h>

#include "modules/branch.h"
#include "utils/rng.h"

int main()
{
  seed_rng((unsigned int)time(NULL));
  load_branches();
  printf("ENTRYPOINT\n");
  return 0;
}
