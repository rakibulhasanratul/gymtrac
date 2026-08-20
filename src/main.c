#include <stdio.h>
#include <time.h>

#include "modules/auth.h"
#include "modules/branch.h"
#include "modules/user.h"
#include "utils/rng.h"

#define DEFAULT_SYSADMIN_USERNAME "admin"
#define DEFAULT_SYSADMIN_PASSWORD "admin123"

int main()
{
  seed_rng((unsigned int)time(NULL));
  load_branches();

  int sysadmin_count = load_sysadmins();
  if (sysadmin_count == 0)
  {
    char hashed_password[PASSWORD_HASH_BUFFER_SIZE];
    hash_password(DEFAULT_SYSADMIN_PASSWORD, hashed_password);
    create_sysadmin(DEFAULT_SYSADMIN_USERNAME, hashed_password);
  }

  printf("ENTRYPOINT\n");
  return 0;
}
