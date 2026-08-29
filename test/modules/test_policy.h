#ifndef GYMTRAC_TEST_POLICY_H
#define GYMTRAC_TEST_POLICY_H

/* membership approval */
void test_membership_approval_sysadmin_allows_any();
void test_membership_approval_manager_own_branch_allows();
void test_membership_approval_manager_other_branch_denies();
void test_membership_approval_trainer_denies();
void test_membership_approval_member_denies();
void test_membership_approval_inactive_session_denies();
void test_membership_approval_unknown_member_id_denies();

/* membership suspension */
void test_membership_suspension_sysadmin_allows_any();
void test_membership_suspension_manager_own_branch_allows();
void test_membership_suspension_manager_other_branch_denies();
void test_membership_suspension_trainer_denies();
void test_membership_suspension_member_denies();
void test_membership_suspension_inactive_session_denies();
void test_membership_suspension_unknown_member_id_denies();

/* membership unsuspension */
void test_membership_unsuspension_sysadmin_allows_any();
void test_membership_unsuspension_manager_own_branch_allows();
void test_membership_unsuspension_manager_other_branch_denies();
void test_membership_unsuspension_trainer_denies();
void test_membership_unsuspension_member_denies();
void test_membership_unsuspension_inactive_session_denies();
void test_membership_unsuspension_unknown_member_id_denies();

/* status change request (trainer) */
void test_status_change_request_sysadmin_allows_any();
void test_status_change_request_trainer_own_branch_allows();
void test_status_change_request_trainer_other_branch_denies();
void test_status_change_request_manager_denies();
void test_status_change_request_member_denies();
void test_status_change_request_inactive_session_denies();
void test_status_change_request_unknown_member_id_denies();

/* plan change request (member) */
void test_plan_change_request_sysadmin_allows_any();
void test_plan_change_request_member_self_allows();
void test_plan_change_request_member_other_denies();
void test_plan_change_request_trainer_denies();
void test_plan_change_request_inactive_session_denies();

/* profile edit request (member) */
void test_profile_edit_request_sysadmin_allows_any();
void test_profile_edit_request_member_self_allows();
void test_profile_edit_request_member_other_denies();
void test_profile_edit_request_trainer_denies();
void test_profile_edit_request_inactive_session_denies();

/* digital payment (member self) */
void test_digital_payment_sysadmin_allows_any();
void test_digital_payment_member_self_allows();
void test_digital_payment_member_other_denies();
void test_digital_payment_trainer_denies();
void test_digital_payment_inactive_session_denies();

/* cash payment (staff) */
void test_cash_payment_sysadmin_allows_any();
void test_cash_payment_manager_own_branch_allows();
void test_cash_payment_trainer_own_branch_allows();
void test_cash_payment_manager_other_branch_denies();
void test_cash_payment_member_denies();
void test_cash_payment_inactive_session_denies();
void test_cash_payment_unknown_member_id_denies();

/* payment view */
void test_payment_view_sysadmin_allows_any();
void test_payment_view_member_self_allows();
void test_payment_view_member_other_denies();
void test_payment_view_manager_own_branch_allows();
void test_payment_view_manager_other_branch_denies();
void test_payment_view_inactive_session_denies();
void test_payment_view_unknown_member_id_denies();

/* member profile view */
void test_member_profile_view_sysadmin_allows_any();
void test_member_profile_view_member_self_allows();
void test_member_profile_view_member_other_denies();
void test_member_profile_view_manager_own_branch_allows();
void test_member_profile_view_manager_other_branch_denies();
void test_member_profile_view_inactive_session_denies();
void test_member_profile_view_unknown_member_id_denies();

/* lost and found resolution */
void test_lost_found_resolution_sysadmin_allows_any();
void test_lost_found_resolution_manager_own_branch_allows();
void test_lost_found_resolution_manager_other_branch_denies();
void test_lost_found_resolution_trainer_denies();
void test_lost_found_resolution_inactive_session_denies();

/* branch deletion */
void test_branch_deletion_sysadmin_allows();
void test_branch_deletion_manager_denies();
void test_branch_deletion_trainer_denies();
void test_branch_deletion_member_denies();
void test_branch_deletion_inactive_session_denies();
void test_branch_deletion_blank_branch_denies();
void test_branch_deletion_null_branch_denies();

/* member deletion */
void test_member_deletion_sysadmin_allows_any();
void test_member_deletion_manager_own_branch_zero_dues_allows();
void test_member_deletion_manager_own_branch_with_dues_denies();
void test_member_deletion_manager_other_branch_denies();
void test_member_deletion_trainer_denies();
void test_member_deletion_inactive_session_denies();
void test_member_deletion_unknown_member_id_denies();

/* staff deletion */
void test_staff_deletion_sysadmin_allows_any();
void test_staff_deletion_manager_own_branch_trainer_allows();
void test_staff_deletion_manager_own_branch_manager_denies();
void test_staff_deletion_manager_other_branch_denies();
void test_staff_deletion_trainer_denies();
void test_staff_deletion_inactive_session_denies();
void test_staff_deletion_unknown_staff_id_denies();

/* branch creation */
void test_branch_creation_sysadmin_allows();
void test_branch_creation_manager_denies();
void test_branch_creation_trainer_denies();
void test_branch_creation_member_denies();
void test_branch_creation_inactive_session_denies();

/* branch rename */
void test_branch_rename_sysadmin_allows();
void test_branch_rename_manager_denies();
void test_branch_rename_trainer_denies();
void test_branch_rename_member_denies();
void test_branch_rename_inactive_session_denies();

/* staff creation */
void test_staff_creation_sysadmin_trainer_any_branch_allows();
void test_staff_creation_sysadmin_manager_any_branch_allows();
void test_staff_creation_manager_trainer_own_branch_allows();
void test_staff_creation_manager_manager_own_branch_denies();
void test_staff_creation_manager_trainer_other_branch_denies();
void test_staff_creation_trainer_denies();
void test_staff_creation_member_denies();
void test_staff_creation_inactive_session_denies();
void test_staff_creation_blank_branch_denies();
void test_staff_creation_unknown_branch_denies();
void test_staff_creation_invalid_role_denies();

/* gym member creation */
void test_gym_member_creation_blank_branch_denies();
void test_gym_member_creation_unknown_branch_denies();
void test_gym_member_creation_valid_branch_allows();

/* branch name validity */
void test_branch_name_is_valid_blank_denies();
void test_branch_name_is_valid_null_denies();
void test_branch_name_is_valid_unknown_denies();
void test_branch_name_is_valid_known_allows();

/* branch listing */
void test_branch_listing_sysadmin_allows();
void test_branch_listing_manager_allows();
void test_branch_listing_member_allows();
void test_branch_listing_inactive_session_denies();

/* member listing */
void test_member_listing_sysadmin_any_branch_allows();
void test_member_listing_sysadmin_empty_branch_allows();
void test_member_listing_manager_own_branch_allows();
void test_member_listing_manager_other_branch_denies();
void test_member_listing_trainer_own_branch_allows();
void test_member_listing_trainer_other_branch_denies();
void test_member_listing_inactive_session_denies();
void test_member_listing_blank_branch_non_sysadmin_denies();

/* lost and found view */
void test_lost_found_view_sysadmin_any_branch_allows();
void test_lost_found_view_manager_own_branch_allows();
void test_lost_found_view_manager_other_branch_denies();
void test_lost_found_view_trainer_own_branch_allows();
void test_lost_found_view_trainer_other_branch_denies();
void test_lost_found_view_inactive_session_denies();
void test_lost_found_view_blank_branch_non_sysadmin_denies();

/* lost and found report */
void test_lost_found_report_sysadmin_any_branch_allows();
void test_lost_found_report_manager_own_branch_allows();
void test_lost_found_report_manager_other_branch_denies();
void test_lost_found_report_member_own_branch_allows();
void test_lost_found_report_member_other_branch_denies();
void test_lost_found_report_inactive_session_denies();
void test_lost_found_report_blank_branch_denies();

#endif
