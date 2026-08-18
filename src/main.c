#include <stdio.h>
#include <time.h>

#include "utils/rng.h"

int main()
{
  seed_rng((unsigned int)time(NULL));
  printf("ENTRYPOINT\n");
  return 0;
}
