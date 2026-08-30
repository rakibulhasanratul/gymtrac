#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "../settings.h"
#include "../types.h"
#include "../utils/datetime_utils.h"
#include "../utils/input.h"
#include "../utils/string_util.h"
#include "auth.h"
#include "branch.h"
#include "lost_found.h"
#include "member.h"
#include "payment.h"
#include "policy.h"
#include "session.h"
#include "user.h"

static bool prompt_string_input(const char prompt[], char *destination, int destination_capacity)
{
  printf("%s ", prompt);
  char raw[LINE_BUFFER_SIZE];
  if (!input_string(raw, LINE_BUFFER_SIZE))
  {
    LOG_ERROR("Error: Failed to read input for '%s'.", prompt);
    return false;
  }
  char trimmed[LINE_BUFFER_SIZE];
  trim(raw, trimmed, LINE_BUFFER_SIZE);
  if (!sanitize_field(trimmed, destination, destination_capacity))
  {
    LOG_ERROR("Error: Failed to sanitize input for '%s'.", prompt);
    return false;
  }
  return true;
}

static bool prompt_integer_input(const char prompt[], int *destination)
{
  printf("%s ", prompt);
  if (!input_integer(destination))
  {
    LOG_ERROR("Error: Invalid integer input for '%s'.", prompt);
    return false;
  }
  return true;
}

static bool prompt_positive_input(const char prompt[], int *destination)
{
  printf("%s ", prompt);
  if (!input_positive_int(destination))
  {
    LOG_ERROR("Error: Invalid positive integer input for '%s'.", prompt);
    return false;
  }
  return true;
}

static inline const char *membership_status_text(membership_status_t status)
{
  switch (status)
  {
  case MEMBERSHIP_ON_HOLD:
    return "on_hold";
  case MEMBERSHIP_ACTIVE:
    return "active";
  case MEMBERSHIP_SUSPENDED:
    return "suspended";
  case MEMBERSHIP_CANCELLED:
    return "cancelled";
  default:
    return "unknown";
  }
}

static inline const char *payment_status_text(payment_status_t status)
{
  switch (status)
  {
  case PAYMENT_PENDING:
    return "pending";
  case PAYMENT_COMPLETED:
    return "completed";
  case PAYMENT_FAILED:
    return "failed";
  case PAYMENT_INVALID:
    return "invalid";
  default:
    return "unknown";
  }
}

static inline void print_datetime(const datetime_t datetime_payload)
{
  char buffer[DATETIME_BUFFER_SIZE];
  if (!format_datetime(datetime_payload, buffer, DATETIME_BUFFER_SIZE))
    printf("invalid-datetime");
  else
    printf("%s", buffer);
}

static inline void print_member_line(const gym_member_t member_payload)
{
  printf(
    "  id=%lu name=%s username=%s branch=%s status=%s due=%u plan=%u/%u days\n", (unsigned long)member_payload.id,
    member_payload.full_name, member_payload.username, member_payload.gym_branch,
    membership_status_text(member_payload.status), member_payload.due_amount, member_payload.plan.payable_amount,
    member_payload.plan.interval_days
  );
}

static inline void print_member_profile(const gym_member_t member_payload)
{
  printf(
    "Profile id=%lu name=%s email=%s phone=%s branch=%s username=%s status=%s\n", (unsigned long)member_payload.id,
    member_payload.full_name, member_payload.email, member_payload.phone_number, member_payload.gym_branch,
    member_payload.username, membership_status_text(member_payload.status)
  );
  printf(
    "  plan %u/%u days due=%u joined=", member_payload.plan.payable_amount, member_payload.plan.interval_days,
    member_payload.due_amount
  );
  print_datetime(member_payload.joined_at);
  printf(" last_payment=");
  print_datetime(member_payload.last_payment_date);
  printf("\n");
}

static inline void print_suspension_line(const suspension_record_t record_payload)
{
  printf("  id=%lu reason=%s suspension=", (unsigned long)record_payload.id, record_payload.reason);
  print_datetime(record_payload.suspension_date);
  printf(" unsuspension=");
  if (is_empty_datetime(record_payload.unsuspension_date))
    printf("-");
  else
    print_datetime(record_payload.unsuspension_date);
  printf("\n");
}

static inline void print_payment_line(const payment_t payment_payload)
{
  printf(
    "  id=%lu member=%lu amount=%u type=%s status=%s time=", (unsigned long)payment_payload.id,
    (unsigned long)payment_payload.gym_member_id, payment_payload.amount,
    payment_payload.transaction_type == CASH_TRANSACTION ? "cash" : "digital",
    payment_status_text(payment_payload.status)
  );
  print_datetime(payment_payload.transaction_time);
  if (!is_blank_string(payment_payload.transaction_id)) printf(" trx=%s", payment_payload.transaction_id);
  printf("\n");
}

static inline void print_lost_found_line(const lost_and_found_record_t record_payload)
{
  printf(
    "  id=%lu branch=%s reporter=%s resolver=%s time=", (unsigned long)record_payload.id, record_payload.gym_branch,
    record_payload.reporter_username,
    is_blank_string(record_payload.resolver_username) ? "-" : record_payload.resolver_username
  );
  print_datetime(record_payload.reported_at);
  printf(" desc=%s\n", record_payload.description);
}

// Runs the auto-suspend sweep after a menu action. Menu-driven stand-in for a
// cron job or background thread, both unavailable in this project. Silent when
// nothing is overdue; announces the count when a member is auto-suspended.
static inline void run_auto_suspend_sweep()
{
  int suspended = auto_suspend_overdue_members();
  if (suspended > 0) printf("Auto-suspended %d overdue member(s).\n", suspended);
}

// Fills destination with the branch the current action should target. Sysadmin is
// prompted for any branch; non-sysadmin sessions get their own branch.
static inline bool resolve_target_branch(const char prompt[], char *destination, int destination_capacity)
{
  if (session_is_sysadmin()) return prompt_string_input(prompt, destination, destination_capacity);
  const char *_branch = get_branch_name_from_session();
  if (_branch == NULL)
  {
    LOG_ERROR("Error: Branch name retrieval failed.");
    return false;
  }
  if (destination_capacity <= (int)strlen(_branch))
  {
    LOG_ERROR("Error: Destination buffer too tight for branch name.");
    return false;
  }
  strcpy(destination, _branch);
  return true;
}

static void handle_add_branch()
{
  if (!ensure_branch_creation_is_allowed()) return;
  char name[BRANCH_NAME_BUFFER_SIZE];
  if (!prompt_string_input("Enter branch name:", name, BRANCH_NAME_BUFFER_SIZE)) return;
  if (is_blank_string(name))
  {
    printf("Branch name cannot be empty.\n");
    return;
  }
  if (add_branch(name))
    printf("Branch '%s' added.\n", name);
  else
    printf("Failed to add branch '%s'.\n", name);
}

static void handle_delete_branch()
{
  char name[BRANCH_NAME_BUFFER_SIZE];
  if (!prompt_string_input("Enter branch name to delete:", name, BRANCH_NAME_BUFFER_SIZE)) return;
  if (!ensure_branch_deletion_is_allowed(name)) return;
  if (delete_branch(name))
    printf("Branch '%s' deleted.\n", name);
  else
    printf("Failed to delete branch '%s'.\n", name);
}

static void handle_rename_branch()
{
  if (!ensure_branch_rename_is_allowed()) return;
  char old_name[BRANCH_NAME_BUFFER_SIZE];
  char new_name[BRANCH_NAME_BUFFER_SIZE];
  if (!prompt_string_input("Enter current branch name:", old_name, BRANCH_NAME_BUFFER_SIZE)) return;
  if (!prompt_string_input("Enter new branch name:", new_name, BRANCH_NAME_BUFFER_SIZE)) return;
  if (update_branch_name(old_name, new_name))
    printf("Branch renamed to '%s'.\n", new_name);
  else
    printf("Failed to rename branch.\n");
}

static void handle_list_branches()
{
  if (!ensure_branch_listing_is_allowed()) return;
  int count = get_branch_count();
  if (count == 0)
  {
    printf("No branches.\n");
    return;
  }
  for (int i = 0; i < count; i++)
  {
    const char *name = get_branch_name(i);
    if (name != NULL) printf("  %d. %s\n", i + 1, name);
  }
}

static void handle_create_staff()
{
  char full_name[FULL_NAME_BUFFER_SIZE];
  char email[EMAIL_BUFFER_SIZE];
  char phone[PHONE_BUFFER_SIZE];
  char branch[BRANCH_NAME_BUFFER_SIZE];
  char username[USERNAME_BUFFER_SIZE];
  char password[USERNAME_BUFFER_SIZE];
  int role_choice = 0;
  if (!prompt_string_input("Enter full name:", full_name, FULL_NAME_BUFFER_SIZE)) return;
  if (!prompt_string_input("Enter email:", email, EMAIL_BUFFER_SIZE)) return;
  if (!prompt_string_input("Enter phone (01XXXXXXXXX):", phone, PHONE_BUFFER_SIZE)) return;
  if (!prompt_string_input("Enter branch name:", branch, BRANCH_NAME_BUFFER_SIZE)) return;
  if (!ensure_branch_name_is_valid(branch)) return;
  if (!prompt_string_input("Enter username:", username, USERNAME_BUFFER_SIZE)) return;
  if (!prompt_string_input("Enter password:", password, USERNAME_BUFFER_SIZE)) return;
  if (!prompt_integer_input("Choose role (1. Branch Manager  2. Trainer): ", &role_choice)) return;
  staff_role_t role;
  switch (role_choice)
  {
  case 1:
    role = BRANCH_MANAGER;
    break;
  case 2:
    role = TRAINER;
    break;
  default:
    printf("Invalid role. Choose 1 for Manager or 2 for Trainer.\n");
    return;
  }
  if (!ensure_staff_creation_is_allowed(branch, role)) return;
  char hash[PASSWORD_HASH_BUFFER_SIZE];
  hash_password(password, hash);
  id_t id = create_branch_staff(full_name, email, phone, branch, username, hash, role);
  if (id != 0)
    printf("Staff created with id %lu.\n", (unsigned long)id);
  else
    printf("Failed to create staff.\n");
}

static void handle_delete_staff()
{
  int staff_id = 0;
  if (!prompt_positive_input("Enter staff id to delete:", &staff_id)) return;
  if (!ensure_staff_deletion_is_allowed((id_t)staff_id)) return;
  if (delete_branch_staff((id_t)staff_id))
    printf("Staff %d deleted.\n", staff_id);
  else
    printf("Failed to delete staff %d.\n", staff_id);
}

static void handle_register_member()
{
  char full_name[FULL_NAME_BUFFER_SIZE];
  char email[EMAIL_BUFFER_SIZE];
  char phone[PHONE_BUFFER_SIZE];
  char branch[BRANCH_NAME_BUFFER_SIZE];
  char username[USERNAME_BUFFER_SIZE];
  char password[USERNAME_BUFFER_SIZE];
  if (!prompt_string_input("Enter full name:", full_name, FULL_NAME_BUFFER_SIZE)) return;
  if (!prompt_string_input("Enter email:", email, EMAIL_BUFFER_SIZE)) return;
  if (!prompt_string_input("Enter phone (01XXXXXXXXX):", phone, PHONE_BUFFER_SIZE)) return;
  if (!prompt_string_input("Enter branch name:", branch, BRANCH_NAME_BUFFER_SIZE)) return;
  if (!ensure_branch_name_is_valid(branch)) return;
  if (!ensure_gym_member_creation_is_allowed(branch)) return;
  if (!prompt_string_input("Enter username:", username, USERNAME_BUFFER_SIZE)) return;
  if (!prompt_string_input("Enter password:", password, USERNAME_BUFFER_SIZE)) return;
  subscription_plan_t plan = {0, 0};
  char hash[PASSWORD_HASH_BUFFER_SIZE];
  hash_password(password, hash);
  id_t id = create_gym_member(full_name, email, phone, branch, username, hash, plan, MEMBERSHIP_ON_HOLD);
  if (id != 0)
    printf("Member registered with id %lu (on_hold).\n", (unsigned long)id);
  else
    printf("Failed to register member.\n");
}

static void handle_approve_member()
{
  int member_id = 0;
  if (!prompt_positive_input("Enter member id to approve:", &member_id)) return;
  if (!ensure_membership_approval_is_allowed((id_t)member_id)) return;
  if (approve_gym_member((id_t)member_id))
    printf("Member %d approved.\n", member_id);
  else
    printf("Failed to approve member %d.\n", member_id);
}

static void handle_suspend_member()
{
  int member_id = 0;
  if (!prompt_positive_input("Enter member id to suspend:", &member_id)) return;
  if (!ensure_membership_suspension_is_allowed((id_t)member_id)) return;
  char reason[REASON_BUFFER_SIZE];
  if (!prompt_string_input("Enter suspension reason:", reason, REASON_BUFFER_SIZE)) return;
  if (is_blank_string(reason))
  {
    printf("Reason cannot be empty.\n");
    return;
  }
  if (suspend_gym_member((id_t)member_id, reason))
    printf("Member %d suspended.\n", member_id);
  else
    printf("Failed to suspend member %d.\n", member_id);
}

static void handle_unsuspend_member()
{
  int member_id = 0;
  if (!prompt_positive_input("Enter member id to unsuspend:", &member_id)) return;
  if (!ensure_membership_unsuspension_is_allowed((id_t)member_id)) return;
  if (unsuspend_gym_member((id_t)member_id))
    printf("Member %d unsuspended.\n", member_id);
  else
    printf("Failed to unsuspend member %d.\n", member_id);
}

static void handle_delete_member()
{
  int member_id = 0;
  if (!prompt_positive_input("Enter member id to delete:", &member_id)) return;
  if (!ensure_member_deletion_is_allowed((id_t)member_id)) return;
  if (delete_gym_member((id_t)member_id))
    printf("Member %d deleted.\n", member_id);
  else
    printf("Failed to delete member %d.\n", member_id);
}

static void handle_view_members_by_branch()
{
  char branch[BRANCH_NAME_BUFFER_SIZE];
  if (!resolve_target_branch("Enter branch name (empty for all):", branch, BRANCH_NAME_BUFFER_SIZE)) return;
  if (is_blank_string(branch) && !session_is_sysadmin()) return;
  if (!is_blank_string(branch) && !ensure_member_listing_is_allowed(branch)) return;
  int found = 0;
  id_t ids[64];
  gym_member_t member;
  for (membership_status_t s = MEMBERSHIP_ON_HOLD; s <= MEMBERSHIP_CANCELLED; s++)
  {
    int count = get_gym_member_ids_by_status(s, ids, 64);
    for (int i = 0; i < count; i++)
    {
      if (!get_gym_member_by_id(ids[i], &member)) continue;
      if (!is_blank_string(branch) && strcmp(member.gym_branch, branch) != 0) continue;
      if (!session_is_sysadmin() && !session_belongs_to_branch(member.gym_branch)) continue;
      print_member_line(member);
      found++;
    }
  }
  if (found == 0) printf("No members found.\n");
}

static void handle_view_own_profile()
{
  if (!session_is_member()) return;
  id_t id = get_user_id_from_session();
  if (!ensure_member_profile_view_allowed(id)) return;
  gym_member_t member;
  if (!get_gym_member_by_id(id, &member)) return;
  print_member_profile(member);
}

static void handle_record_digital_payment()
{
  int member_id = 0;
  if (session_is_member())
    member_id = (int)get_user_id_from_session();
  else if (!prompt_positive_input("Enter member id:", &member_id))
    return;
  if (!ensure_digital_payment_is_allowed((id_t)member_id)) return;
  int amount = 0;
  if (!prompt_positive_input("Enter amount:", &amount)) return;
  char trx[TRX_ID_BUFFER_SIZE];
  if (!prompt_string_input("Enter transaction id:", trx, TRX_ID_BUFFER_SIZE)) return;
  char status_text[32];
  if (!prompt_string_input("Enter status (pending/completed/failed/invalid):", status_text, 32)) return;
  to_lowercase(status_text);
  payment_status_t status;
  if (strcmp(status_text, "pending") == 0)
    status = PAYMENT_PENDING;
  else if (strcmp(status_text, "completed") == 0)
    status = PAYMENT_COMPLETED;
  else if (strcmp(status_text, "failed") == 0)
    status = PAYMENT_FAILED;
  else if (strcmp(status_text, "invalid") == 0)
    status = PAYMENT_INVALID;
  else
  {
    printf("Invalid payment status.\n");
    return;
  }
  digital_payment_request_t req = {0, (id_t)member_id, (unsigned int)amount, now_datetime(), "", status};
  strcpy(req.transaction_id, trx);
  if (record_digital_payment(req))
    printf("Digital payment recorded.\n");
  else
    printf("Failed to record digital payment.\n");
}

static void handle_record_cash_payment()
{
  int member_id = 0;
  if (!prompt_positive_input("Enter member id:", &member_id)) return;
  if (!ensure_cash_payment_recording_is_allowed((id_t)member_id)) return;
  int amount = 0;
  if (!prompt_positive_input("Enter amount:", &amount)) return;
  if (record_cash_payment((id_t)member_id, (unsigned int)amount))
    printf("Cash payment recorded.\n");
  else
    printf("Failed to record cash payment.\n");
}

static void handle_view_payments()
{
  int member_id = 0;
  if (!prompt_positive_input("Enter member id to view:", &member_id)) return;
  if (!ensure_payment_view_allowed((id_t)member_id)) return;
  payment_t list[32];
  int count = get_payments_for_member((id_t)member_id, list, 32);
  if (count == 0)
  {
    printf("No payments for member %d.\n", member_id);
    return;
  }
  for (int i = 0; i < count; i++) print_payment_line(list[i]);
  if (count == 32) printf("  (showing first 32)\n");
}

static void handle_view_own_suspensions()
{
  id_t id = get_user_id_from_session();
  suspension_record_t list[16];
  int count = get_suspensions_for_member(id, list, 16);
  if (count == 0)
  {
    printf("No suspensions.\n");
    return;
  }
  for (int i = 0; i < count; i++) print_suspension_line(list[i]);
}

static void handle_report_lost_found()
{
  char branch[BRANCH_NAME_BUFFER_SIZE];
  if (!resolve_target_branch("Enter branch name:", branch, BRANCH_NAME_BUFFER_SIZE)) return;
  if (!ensure_branch_name_is_valid(branch)) return;
  if (!ensure_lost_found_report_is_allowed(branch)) return;
  char desc[DESCRIPTION_BUFFER_SIZE];
  if (!prompt_string_input("Enter description:", desc, DESCRIPTION_BUFFER_SIZE)) return;
  if (report_lost_item(get_username_from_session(), branch, desc))
    printf("Report submitted.\n");
  else
    printf("Failed to submit report.\n");
}

static void handle_view_lost_found()
{
  char branch[BRANCH_NAME_BUFFER_SIZE];
  if (!resolve_target_branch("Enter branch name:", branch, BRANCH_NAME_BUFFER_SIZE)) return;
  if (!ensure_lost_found_view_is_allowed(branch)) return;
  lost_and_found_record_t list[32];
  int count = get_lost_and_found_for_branch(branch, list, 32);
  if (count == 0)
  {
    printf("No reports for branch '%s'.\n", branch);
    return;
  }
  for (int i = 0; i < count; i++) print_lost_found_line(list[i]);
  if (count == 32) printf("  (showing first 32)\n");
}

static void handle_resolve_lost_found()
{
  int record_id = 0;
  if (!prompt_positive_input("Enter report id to resolve:", &record_id)) return;
  lost_and_found_record_t item;
  if (!get_lost_and_found_by_id((id_t)record_id, &item)) return;
  if (!ensure_lost_found_resolution_is_allowed(item)) return;
  if (resolve_lost_item((id_t)record_id, get_username_from_session()))
    printf("Report %d resolved.\n", record_id);
  else
    printf("Failed to resolve report %d.\n", record_id);
}

static void run_sysadmin_menu()
{
  while (session_is_sysadmin())
  {
    printf("\n-- Sysadmin Menu --\n");
    printf(" 1. Add branch\n");
    printf(" 2. Delete branch\n");
    printf(" 3. Rename branch\n");
    printf(" 4. List branches\n");
    printf(" 5. Create staff\n");
    printf(" 6. Delete staff\n");
    printf(" 7. Register member\n");
    printf(" 8. Approve member\n");
    printf(" 9. Suspend member\n");
    printf("10. Unsuspend member\n");
    printf("11. Delete member\n");
    printf("12. List members\n");
    printf("13. Digital pay\n");
    printf("14. Cash pay\n");
    printf("15. View payments\n");
    printf("16. Report Lost & Found\n");
    printf("17. View Lost & Found\n");
    printf("18. Resolve Lost & Found\n");
    printf(" 0. Logout\n");
    int choice = 0;
    if (!prompt_integer_input("\nChoose option:", &choice)) continue;
    printf("\n");
    switch (choice)
    {
    case 1:
      handle_add_branch();
      run_auto_suspend_sweep();
      break;
    case 2:
      handle_delete_branch();
      run_auto_suspend_sweep();
      break;
    case 3:
      handle_rename_branch();
      run_auto_suspend_sweep();
      break;
    case 4:
      handle_list_branches();
      run_auto_suspend_sweep();
      break;
    case 5:
      handle_create_staff();
      run_auto_suspend_sweep();
      break;
    case 6:
      handle_delete_staff();
      run_auto_suspend_sweep();
      break;
    case 7:
      handle_register_member();
      run_auto_suspend_sweep();
      break;
    case 8:
      handle_approve_member();
      run_auto_suspend_sweep();
      break;
    case 9:
      handle_suspend_member();
      run_auto_suspend_sweep();
      break;
    case 10:
      handle_unsuspend_member();
      run_auto_suspend_sweep();
      break;
    case 11:
      handle_delete_member();
      run_auto_suspend_sweep();
      break;
    case 12:
      handle_view_members_by_branch();
      run_auto_suspend_sweep();
      break;
    case 13:
      handle_record_digital_payment();
      run_auto_suspend_sweep();
      break;
    case 14:
      handle_record_cash_payment();
      run_auto_suspend_sweep();
      break;
    case 15:
      handle_view_payments();
      run_auto_suspend_sweep();
      break;
    case 16:
      handle_report_lost_found();
      run_auto_suspend_sweep();
      break;
    case 17:
      handle_view_lost_found();
      run_auto_suspend_sweep();
      break;
    case 18:
      handle_resolve_lost_found();
      run_auto_suspend_sweep();
      break;
    case 0:
      auth_logout();
      printf("Logged out.\n");
      break;
    default:
      printf("Invalid option.\n");
      break;
    }
  }
}

static void run_branch_manager_menu()
{
  while (session_is_branch_manager())
  {
    printf("\n-- Branch Manager Menu (%s) --\n", get_branch_name_from_session());
    printf(" 1. List branches\n");
    printf(" 2. Approve member\n");
    printf(" 3. Suspend member\n");
    printf(" 4. Unsuspend member\n");
    printf(" 5. Delete member\n");
    printf(" 6. Delete trainer\n");
    printf(" 7. List members\n");
    printf(" 8. Cash pay\n");
    printf(" 9. View payments\n");
    printf("10. View Lost & Found\n");
    printf("11. Resolve Lost & Found\n");
    printf("12. Report Lost & Found\n");
    printf("13. Create trainer\n");
    printf(" 0. Logout\n");
    int choice = 0;
    if (!prompt_integer_input("\nChoose option:", &choice)) continue;
    printf("\n");
    switch (choice)
    {
    case 1:
      handle_list_branches();
      run_auto_suspend_sweep();
      break;
    case 2:
      handle_approve_member();
      run_auto_suspend_sweep();
      break;
    case 3:
      handle_suspend_member();
      run_auto_suspend_sweep();
      break;
    case 4:
      handle_unsuspend_member();
      run_auto_suspend_sweep();
      break;
    case 5:
      handle_delete_member();
      run_auto_suspend_sweep();
      break;
    case 6:
      handle_delete_staff();
      run_auto_suspend_sweep();
      break;
    case 7:
      handle_view_members_by_branch();
      run_auto_suspend_sweep();
      break;
    case 8:
      handle_record_cash_payment();
      run_auto_suspend_sweep();
      break;
    case 9:
      handle_view_payments();
      run_auto_suspend_sweep();
      break;
    case 10:
      handle_view_lost_found();
      run_auto_suspend_sweep();
      break;
    case 11:
      handle_resolve_lost_found();
      run_auto_suspend_sweep();
      break;
    case 12:
      handle_report_lost_found();
      run_auto_suspend_sweep();
      break;
    case 13:
      handle_create_staff();
      run_auto_suspend_sweep();
      break;
    case 0:
      auth_logout();
      printf("Logged out.\n");
      break;
    default:
      printf("Invalid option.\n");
      break;
    }
  }
}

static void run_trainer_menu()
{
  while (session_is_trainer())
  {
    printf("\n-- Trainer Menu (%s) --\n", get_branch_name_from_session());
    printf(" 1. List members\n");
    printf(" 2. List payments\n");
    printf(" 3. Cash pay\n");
    printf(" 4. View Lost & Found\n");
    printf(" 5. Report Lost & Found\n");
    printf(" 0. Logout\n");
    int choice = 0;
    if (!prompt_integer_input("\nChoose option:", &choice)) continue;
    printf("\n");
    switch (choice)
    {
    case 1:
      handle_view_members_by_branch();
      run_auto_suspend_sweep();
      break;
    case 2:
      handle_view_payments();
      run_auto_suspend_sweep();
      break;
    case 3:
      handle_record_cash_payment();
      run_auto_suspend_sweep();
      break;
    case 4:
      handle_view_lost_found();
      run_auto_suspend_sweep();
      break;
    case 5:
      handle_report_lost_found();
      run_auto_suspend_sweep();
      break;
    case 0:
      auth_logout();
      printf("Logged out.\n");
      break;
    default:
      printf("Invalid option.\n");
      break;
    }
  }
}

static void run_member_menu()
{
  while (session_is_member())
  {
    printf("\n-- Member Menu (%s) --\n", get_branch_name_from_session());
    printf(" 1. View profile\n");
    printf(" 2. Digital pay\n");
    printf(" 3. View payments\n");
    printf(" 4. View suspensions\n");
    printf(" 5. Report Lost & Found\n");
    printf(" 6. View Lost & Found\n");
    printf(" 0. Logout\n");
    int choice = 0;
    if (!prompt_integer_input("\nChoose option:", &choice)) continue;
    printf("\n");
    switch (choice)
    {
    case 1:
      handle_view_own_profile();
      run_auto_suspend_sweep();
      break;
    case 2:
      handle_record_digital_payment();
      run_auto_suspend_sweep();
      break;
    case 3:
      handle_view_payments();
      run_auto_suspend_sweep();
      break;
    case 4:
      handle_view_own_suspensions();
      run_auto_suspend_sweep();
      break;
    case 5:
      handle_report_lost_found();
      run_auto_suspend_sweep();
      break;
    case 6:
      handle_view_lost_found();
      run_auto_suspend_sweep();
      break;
    case 0:
      auth_logout();
      printf("Logged out.\n");
      break;
    default:
      printf("Invalid option.\n");
      break;
    }
  }
}

static void handle_login()
{
  char username[USERNAME_BUFFER_SIZE];
  char password[USERNAME_BUFFER_SIZE];
  if (!prompt_string_input("Enter username:", username, USERNAME_BUFFER_SIZE)) return;
  if (!prompt_string_input("Enter password:", password, USERNAME_BUFFER_SIZE)) return;
  user_role_t role = 0;
  if (!auth_login(username, password, &role))
  {
    printf("Login failed.\n");
    return;
  }
  printf("Login successful as %s.\n", username);
  switch (role)
  {
  case USER_ROLE_SYSADMIN:
    run_sysadmin_menu();
    break;
  case USER_ROLE_BRANCH_MANAGER:
    run_branch_manager_menu();
    break;
  case USER_ROLE_TRAINER:
    run_trainer_menu();
    break;
  case USER_ROLE_MEMBER:
    run_member_menu();
    break;
  default:
    printf("Unknown role.\n");
    auth_logout();
    break;
  }
}

void run_main_menu()
{
  while (true)
  {
    printf("\n== Gymtrac ==\n");
    printf(" 1. Login\n");
    printf(" 2. Register as member\n");
    printf(" 0. Exit\n");
    int choice = 0;
    if (!prompt_integer_input("\nChoose option:", &choice)) continue;
    printf("\n");
    switch (choice)
    {
    case 1:
      handle_login();
      run_auto_suspend_sweep();
      break;
    case 2:
      handle_register_member();
      run_auto_suspend_sweep();
      break;
    case 0:
      printf("Goodbye.\n");
      return;
    default:
      printf("Invalid option.\n");
      break;
    }
  }
}
