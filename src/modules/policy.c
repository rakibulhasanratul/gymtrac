#include <stdbool.h>

#include "../types.h"
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

bool ensure_status_change_request_is_allowed(id_t gym_member_id)
{
  if (session_is_sysadmin()) return true;
  if (!session_is_trainer()) return false;
  gym_member_t member;
  if (!get_gym_member_by_id(gym_member_id, &member)) return false;
  return session_belongs_to_branch(member.gym_branch);
}

bool ensure_plan_change_request_is_allowed(id_t gym_member_id)
{
  if (session_is_sysadmin()) return true;
  if (!session_is_member()) return false;
  return get_user_id_from_session() == gym_member_id;
}

bool ensure_profile_edit_request_is_allowed(id_t gym_member_id)
{
  if (session_is_sysadmin()) return true;
  if (!session_is_member()) return false;
  return get_user_id_from_session() == gym_member_id;
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

bool ensure_lost_found_resolution_is_allowed(lost_and_found_record_t item)
{
  if (session_is_sysadmin()) return true;
  if (!session_is_branch_manager()) return false;
  return session_belongs_to_branch(item.gym_branch);
}

bool ensure_branch_deletion_is_allowed()
{
  if (session_is_sysadmin()) return true;
  return false;
}

bool ensure_member_deletion_is_allowed(id_t gym_member_id)
{
  if (session_is_sysadmin()) return true;
  if (!session_is_branch_manager()) return false;
  gym_member_t member;
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
