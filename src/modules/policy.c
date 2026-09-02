#include <stdbool.h>

#include "../settings.h"
#include "../types.h"
#include "../utils/string_util.h"
#include "branch.h"
#include "session.h"
#include "user.h"

bool ensure_membership_approval_is_allowed(id_t gym_member_id)
{
  if (session_is_sysadmin()) return true;
  if (!session_is_branch_manager()) return false;
  gym_member_t member;
  if (!get_gym_member_by_id(gym_member_id, &member)) return false;
  return session_belongs_to_branch(member.gym_branch);
}

bool ensure_membership_suspension_is_allowed(id_t gym_member_id)
{
  if (session_is_sysadmin()) return true;
  if (!session_is_branch_manager()) return false;

  gym_member_t member;
  if (!get_gym_member_by_id(gym_member_id, &member)) return false;
  return session_belongs_to_branch(member.gym_branch);
}

bool ensure_membership_unsuspension_is_allowed(id_t gym_member_id)
{
  if (session_is_sysadmin()) return true;
  if (!session_is_branch_manager()) return false;
  gym_member_t member;
  if (!get_gym_member_by_id(gym_member_id, &member)) return false;
  return session_belongs_to_branch(member.gym_branch);
}

bool ensure_digital_payment_is_allowed(id_t gym_member_id)
{
  if (session_is_sysadmin()) return true;
  if (!session_is_member()) return false;
  return get_user_id_from_session() == gym_member_id;
}

bool ensure_cash_payment_recording_is_allowed(id_t gym_member_id)
{
  if (session_is_sysadmin()) return true;
  if (session_is_member()) return false;
  gym_member_t member;
  if (!get_gym_member_by_id(gym_member_id, &member)) return false;
  return session_belongs_to_branch(member.gym_branch);
}

bool ensure_payment_view_allowed(id_t gym_member_id)
{
  if (session_is_sysadmin()) return true;
  if (session_is_member()) return get_user_id_from_session() == gym_member_id;
  gym_member_t member;
  if (!get_gym_member_by_id(gym_member_id, &member)) return false;
  return session_belongs_to_branch(member.gym_branch);
}

bool ensure_member_profile_view_allowed(id_t gym_member_id)
{
  if (session_is_sysadmin()) return true;
  if (session_is_member()) return get_user_id_from_session() == gym_member_id;
  gym_member_t member;
  if (!get_gym_member_by_id(gym_member_id, &member)) return false;
  return session_belongs_to_branch(member.gym_branch);
}

bool ensure_lost_found_resolution_is_allowed(const lost_and_found_record_t item_payload)
{
  if (session_is_sysadmin()) return true;
  if (!session_is_branch_manager()) return false;
  return session_belongs_to_branch(item_payload.gym_branch);
}

bool ensure_branch_deletion_is_allowed(const char branch_name[])
{
  if (is_blank_string(branch_name)) return false;
  if (session_is_sysadmin()) return true;
  return false;
}

bool ensure_member_deletion_is_allowed(id_t gym_member_id)
{
  if (session_is_sysadmin()) return true;
  if (!session_is_branch_manager()) return false;
  gym_member_t member;
  if (!get_gym_member_by_id(gym_member_id, &member)) return false;
  if (!session_belongs_to_branch(member.gym_branch)) return false;
  return member.due_amount == 0;
}

bool ensure_staff_deletion_is_allowed(id_t staff_id)
{
  if (session_is_sysadmin()) return true;
  if (!session_is_branch_manager()) return false;
  branch_staff_t staff;
  if (!get_branch_staff_by_id(staff_id, &staff)) return false;
  if (!session_belongs_to_branch(staff.gym_branch)) return false;
  return staff.role == TRAINER;
}

bool ensure_branch_creation_is_allowed()
{
  if (!session_is_sysadmin()) return false;
  if (get_branch_count() >= BRANCH_COUNT_MAX) return false;
  return true;
}

bool ensure_branch_rename_is_allowed()
{
  return session_is_sysadmin();
}

bool ensure_staff_creation_is_allowed(const char branch_name[], staff_role_t role)
{
  if (is_blank_string(branch_name)) return false;
  if (find_branch(branch_name) == -1) return false;
  switch (role)
  {
  case BRANCH_MANAGER:
    if (branch_manager_count(branch_name) >= MAX_MANAGERS_PER_BRANCH) return false;
    break;
  case TRAINER:
    if (branch_trainer_count(branch_name) >= MAX_TRAINERS_PER_BRANCH) return false;
    break;
  default:
    return false;
  }
  if (session_is_sysadmin()) return true;
  if (role == TRAINER && session_is_branch_manager() && session_belongs_to_branch(branch_name)) return true;
  return false;
}

bool ensure_gym_member_creation_is_allowed(const char branch_name[])
{
  if (is_blank_string(branch_name)) return false;
  if (find_branch(branch_name) == -1) return false;
  if (branch_member_count(branch_name) >= MAX_MEMBERS_PER_BRANCH) return false;
  return true;
}

bool ensure_branch_name_is_valid(const char branch_name[])
{
  if (is_blank_string(branch_name)) return false;
  if (find_branch(branch_name) == -1) return false;
  return true;
}

bool ensure_branch_listing_is_allowed()
{
  return session_is_active();
}

bool ensure_member_listing_is_allowed(const char branch_name[])
{
  if (!session_is_active()) return false;
  if (session_is_sysadmin()) return true;
  if (is_blank_string(branch_name)) return false;
  return session_belongs_to_branch(branch_name);
}

bool ensure_lost_found_view_is_allowed(const char branch_name[])
{
  if (!session_is_active()) return false;
  if (session_is_sysadmin()) return true;
  if (is_blank_string(branch_name)) return false;
  return session_belongs_to_branch(branch_name);
}

bool ensure_lost_found_report_is_allowed(const char branch_name[])
{
  if (!session_is_active()) return false;
  if (is_blank_string(branch_name)) return false;
  if (session_is_sysadmin()) return true;
  return session_belongs_to_branch(branch_name);
}

bool ensure_staff_listing_is_allowed(const char branch_name[])
{
  if (!session_is_active()) return false;
  if (session_is_sysadmin()) return true;
  if (!session_is_branch_manager()) return false;
  if (is_blank_string(branch_name)) return false;
  return session_belongs_to_branch(branch_name);
}
