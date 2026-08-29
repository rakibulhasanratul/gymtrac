#include <stdio.h>
#include <string.h>
#include <time.h>

#include "modules/auth.h"
#include "modules/branch.h"
#include "modules/lost_found.h"
#include "modules/member.h"
#include "modules/menu.h"
#include "modules/payment.h"
#include "modules/user.h"
#include "settings.h"
#include "utils/rng.h"

int main()
{
  if (strlen(FIELD_DELIMITER_STRING) != 1)
  {
    LOG_ERROR("FIELD_DELIMITER_STRING must be exactly one character long.");
    return 1;
  }

  seed_rng((unsigned int)time(NULL));

  load_branches();
  load_branch_staff();
  load_gym_members();
  load_suspensions();
  load_payments();
  load_lost_and_found_records();

  int sysadmin_count = load_sysadmins();
  if (sysadmin_count == 0)
  {
    char hashed_password[PASSWORD_HASH_BUFFER_SIZE];
    hash_password(DEFAULT_SYSADMIN_PASSWORD, hashed_password);
    create_sysadmin(DEFAULT_SYSADMIN_USERNAME, hashed_password);
  }

  int auto_suspended = auto_suspend_overdue_members();
  printf("Boot complete. Auto-suspended %d overdue member(s).\n", auto_suspended);

  run_main_menu();
  return 0;
}