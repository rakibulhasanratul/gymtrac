#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "../src/utils/rng.h"
#include "modules/test_auth.h"
#include "modules/test_branch.h"
#include "modules/test_lost_found.h"
#include "modules/test_member.h"
#include "modules/test_payment.h"
#include "modules/test_policy.h"
#include "modules/test_session.h"
#include "modules/test_user.h"
#include "utils/test_datetime_utils.h"
#include "utils/test_file_util.h"
#include "utils/test_hash.h"
#include "utils/test_input.h"
#include "utils/test_string_util.h"

int main()
{
  srand((unsigned int)time(NULL));
  seed_rng((unsigned int)time(NULL));

  /* Remove leftover test data files from any previous run. */
  cleanup_branches_file();
  cleanup_user_files();
  cleanup_member_files();
  cleanup_payment_files();
  cleanup_lost_found_files();

  /* string_util */
  test_trim_strips_whitespace();
  test_split_copies_fields();
  test_string_to_unsigned_int_converts_digits();
  test_string_to_unsigned_long_int_converts_digits();
  test_lowercase_and_uppercase_convert_letters();
  test_sanitize_field_strips_control_chars();

  /* file_util */
  test_write_read_round_trip();
  test_read_tolerates_crlf_line_ending();
  test_read_drains_overlong_line();
  test_read_line_rejects_invalid_arguments();
  test_write_line_rejects_invalid_arguments();
  test_write_read_lines_round_trip();
  test_read_lines_skip_empty_lines();
  test_write_lines_overwrite_existing_content();
  test_read_lines_respects_max_lines();
  test_read_lines_missing_file_returns_zero();
  test_write_lines_rejects_invalid_arguments();
  test_read_lines_rejects_invalid_arguments();

  /* input */
  test_input_string_reads_plain_lines();
  test_input_string_tolerates_crlf();
  test_input_string_caps_and_drains_overlong();
  test_input_string_reads_empty_line();
  test_input_string_rejects_invalid_arguments();
  test_input_integer_reads_valid_values();
  test_input_integer_rejects_bad_input();
  test_input_integer_drains_remaining_line();
  test_input_positive_int_keeps_positives_only();

  /* hash */
  test_create_hash_known_vectors();
  test_create_hash_null_returns_zero();
  test_generate_salt_length_and_charset();
  test_generate_salt_null_is_safe();
  test_mix_salt_sandwich_output();
  test_mix_salt_empty_password();
  test_mix_salt_null_is_safe();
  test_compare_hash_equal();
  test_compare_hash_different();
  test_hash_value_to_string_decimal();
  test_hash_value_to_string_null_is_safe();
  test_parse_hash_value_decimal();
  test_parse_hash_value_invalid_returns_zero();
  test_hash_value_to_string_parse_round_trip();
  test_create_hash_consistency();
  test_different_salts_different_hashes();

  /* auth */
  test_hash_password_valid_format();
  test_hash_password_unique_salts();
  test_verify_password_correct();
  test_verify_password_wrong();
  test_verify_password_empty_vs_nonempty();
  test_verify_password_empty_password();
  test_verify_password_null_returns_false();
  test_hash_password_null_is_safe();
  test_auth_login_sysadmin_success();
  test_auth_login_branch_manager_success();
  test_auth_login_trainer_success();
  test_auth_login_member_success();
  test_auth_login_wrong_password();
  test_auth_login_unknown_username();
  test_auth_login_null_inputs();
  test_auth_logout_clears_session();

  /* datetime_utils */
  test_format_datetime_writes_expected_text();
  test_format_datetime_rejects_invalid_arguments();
  test_parse_datetime_reads_valid_text();
  test_parse_datetime_rejects_invalid();
  test_format_parse_round_trip();
  test_datetime_to_seconds_known_values();
  test_seconds_round_trip();
  test_leap_year_handling();
  test_add_days_crosses_month_and_year_boundaries();
  test_add_months_clamps_day();
  test_add_months_year_boundary();
  test_compare_datetime_orders_fields();
  test_days_between_calculates_difference();
  test_now_datetime_returns_current_time();
  test_is_empty_datetime_checks_all_fields();

  /* branch */
  test_add_branch_and_exists();
  test_add_branch_rejects_duplicate();
  test_add_branch_rejects_empty();
  test_add_branch_rejects_null();
  test_find_branch_returns_negative_for_missing();
  test_find_branch_returns_negative_for_null();
  test_load_branches();
  test_load_branches_returns_zero_when_empty();
  test_add_branch_respects_max_count();
  test_branch_names_are_case_sensitive();
  test_get_branch_count();
  test_get_branch_name();
  test_ensure_branch_has_no_users();
  test_delete_branch_removes_and_persists();
  test_delete_branch_rejects_missing_null_and_empty();
  test_delete_branch_rejects_branch_with_users();
  test_update_branch_name_renames_and_persists();
  test_update_branch_name_cascades_to_user_records();
  test_update_branch_name_rejects_invalid_names();

  /* user: sysadmin */
  test_create_sysadmin_and_get();
  test_create_sysadmin_rejects_second();
  test_create_sysadmin_rejects_empty_username();
  test_create_sysadmin_rejects_null_username();
  test_create_sysadmin_rejects_empty_password();
  test_get_sysadmin_by_username();
  test_get_sysadmin_by_username_null();
  test_get_sysadmin_by_id_not_found();
  test_load_sysadmins_roundtrip();

  /* user: branch staff */
  test_create_branch_staff_and_get();
  test_create_branch_staff_auto_increment_id();
  test_create_branch_staff_rejects_duplicate_username();
  test_create_branch_staff_rejects_empty_fields();
  test_get_branch_staff_by_username();
  test_get_branch_staff_by_id_not_found();
  test_load_branch_staff_roundtrip();

  /* user: gym member */
  test_create_gym_member_and_get();
  test_create_gym_member_auto_increment_id();
  test_create_gym_member_rejects_duplicate_username();
  test_create_gym_member_rejects_empty_fields();
  test_get_gym_member_by_username();
  test_get_gym_member_by_id_not_found();
  test_load_gym_members_roundtrip();

  /* user: username_exists */
  test_username_exists_returns_false_when_empty();
  test_username_exists_finds_sysadmin();
  test_username_exists_finds_branch_staff();
  test_username_exists_finds_gym_member();
  test_username_exists_cross_table_uniqueness();
  test_username_exists_null();

  /* user: branch counts */
  test_branch_manager_count_zero_when_empty();
  test_branch_manager_count_one();
  test_branch_manager_count_ignores_trainers();
  test_branch_manager_count_ignores_other_branches();
  test_branch_trainer_count_zero_when_empty();
  test_branch_trainer_count_multiple();
  test_branch_trainer_count_ignores_managers();
  test_branch_member_count_zero_when_empty();
  test_branch_member_count_multiple();
  test_branch_member_count_ignores_other_branches();
  test_branch_counts_null_and_empty();

  /* user: cross-table uniqueness */
  test_staff_username_blocks_member_creation();
  test_member_username_blocks_staff_creation();

  /* user: deletion */
  test_delete_branch_staff_removes_and_persists();
  test_delete_branch_staff_rejects_unknown_id();
  test_delete_gym_member_removes_and_persists();
  test_delete_gym_member_rejects_member_with_dues();

  /* user: update */
  test_update_branch_staff_updates_fields_and_persists();
  test_update_branch_staff_rejects_unknown_id();
  test_update_branch_staff_rejects_empty_fields();
  test_update_gym_member_updates_fields_and_persists();
  test_update_gym_member_rejects_unknown_id();
  test_update_gym_member_rejects_empty_fields();
  test_update_gym_member_rejects_duplicate_username();
  test_update_gym_member_allows_same_username();
  test_update_gym_member_billing_clamps_and_persists();

  /* session */
  test_session_init_is_inactive();
  test_session_login_sets_context();
  test_session_logout_clears_context();
  test_session_is_active_after_login();
  test_session_is_active_after_logout();
  test_session_is_sysadmin();
  test_session_is_branch_manager();
  test_session_is_trainer();
  test_session_is_member();
  test_session_belongs_to_branch_sysadmin_sees_all();
  test_session_belongs_to_branch_matching();
  test_session_belongs_to_branch_non_matching();
  test_session_belongs_to_branch_inactive_returns_false();
  test_session_belongs_to_branch_null_returns_false();
  test_session_getters_return_empty_when_inactive();
  test_session_getters_return_values_when_active();

  /* member: approval */
  test_approve_on_hold_member_activates_with_default_plan();
  test_approve_rejects_non_on_hold_and_unknown_members();

  /* member: suspension */
  test_suspend_active_member_writes_dated_record();
  test_suspend_rejects_missing_reason_and_invalid_state();

  /* member: unsuspension */
  test_unsuspend_reactivates_member_and_closes_record();
  test_unsuspend_rejects_indebted_and_invalid_members();

  /* member: auto-suspend sweep */
  test_auto_suspend_sweeps_only_overdue_active_members();
  test_auto_suspend_returns_zero_when_nobody_overdue();

  /* member: suspension history getter */
  test_get_suspensions_for_member_handles_history_and_capacity();

  /* payment: digital */
  test_record_digital_completed_payment_settles_account();
  test_record_partial_digital_payment_reduces_due_by_paid_amount();

  /* payment: cash */
  test_record_cash_payment_settles_account();
  test_payment_clamps_overpayment_to_zero_due();

  /* payment: status handling, rejections, history */
  test_non_completed_digital_payment_records_history_without_settling();
  test_payment_rejects_invalid_members_amounts_and_details();
  test_get_payments_for_member_handles_history_and_capacity();

  /* lost & found */
  test_report_item_persists_with_branch_snapshot_and_open_state();
  test_report_works_for_any_role();
  test_get_lost_and_found_scopes_by_branch_reporter_and_resolver();
  test_resolve_lost_item_by_manager_and_sysadmin();
  test_report_rejects_invalid_input();
  test_resolve_rejects_invalid_input();

  /* policy: membership approval */
  test_membership_approval_sysadmin_allows_any();
  test_membership_approval_manager_own_branch_allows();
  test_membership_approval_manager_other_branch_denies();
  test_membership_approval_trainer_denies();
  test_membership_approval_member_denies();
  test_membership_approval_inactive_session_denies();
  test_membership_approval_unknown_member_id_denies();

  /* policy: membership suspension */
  test_membership_suspension_sysadmin_allows_any();
  test_membership_suspension_manager_own_branch_allows();
  test_membership_suspension_manager_other_branch_denies();
  test_membership_suspension_trainer_denies();
  test_membership_suspension_member_denies();
  test_membership_suspension_inactive_session_denies();
  test_membership_suspension_unknown_member_id_denies();

  /* policy: membership unsuspension */
  test_membership_unsuspension_sysadmin_allows_any();
  test_membership_unsuspension_manager_own_branch_allows();
  test_membership_unsuspension_manager_other_branch_denies();
  test_membership_unsuspension_trainer_denies();
  test_membership_unsuspension_member_denies();
  test_membership_unsuspension_inactive_session_denies();
  test_membership_unsuspension_unknown_member_id_denies();

  /* policy: status change request (trainer) */
  test_status_change_request_sysadmin_allows_any();
  test_status_change_request_trainer_own_branch_allows();
  test_status_change_request_trainer_other_branch_denies();
  test_status_change_request_manager_denies();
  test_status_change_request_member_denies();
  test_status_change_request_inactive_session_denies();
  test_status_change_request_unknown_member_id_denies();

  /* policy: plan change request (member) */
  test_plan_change_request_sysadmin_allows_any();
  test_plan_change_request_member_self_allows();
  test_plan_change_request_member_other_denies();
  test_plan_change_request_trainer_denies();
  test_plan_change_request_inactive_session_denies();

  /* policy: profile edit request (member) */
  test_profile_edit_request_sysadmin_allows_any();
  test_profile_edit_request_member_self_allows();
  test_profile_edit_request_member_other_denies();
  test_profile_edit_request_trainer_denies();
  test_profile_edit_request_inactive_session_denies();

  /* policy: digital payment (member self) */
  test_digital_payment_sysadmin_allows_any();
  test_digital_payment_member_self_allows();
  test_digital_payment_member_other_denies();
  test_digital_payment_trainer_denies();
  test_digital_payment_inactive_session_denies();

  /* policy: cash payment (staff) */
  test_cash_payment_sysadmin_allows_any();
  test_cash_payment_manager_own_branch_allows();
  test_cash_payment_trainer_own_branch_allows();
  test_cash_payment_manager_other_branch_denies();
  test_cash_payment_member_denies();
  test_cash_payment_inactive_session_denies();
  test_cash_payment_unknown_member_id_denies();

  /* policy: payment view */
  test_payment_view_sysadmin_allows_any();
  test_payment_view_member_self_allows();
  test_payment_view_member_other_denies();
  test_payment_view_manager_own_branch_allows();
  test_payment_view_manager_other_branch_denies();
  test_payment_view_inactive_session_denies();
  test_payment_view_unknown_member_id_denies();

  /* policy: member profile view */
  test_member_profile_view_sysadmin_allows_any();
  test_member_profile_view_member_self_allows();
  test_member_profile_view_member_other_denies();
  test_member_profile_view_manager_own_branch_allows();
  test_member_profile_view_manager_other_branch_denies();
  test_member_profile_view_inactive_session_denies();
  test_member_profile_view_unknown_member_id_denies();

  /* policy: lost and found resolution */
  test_lost_found_resolution_sysadmin_allows_any();
  test_lost_found_resolution_manager_own_branch_allows();
  test_lost_found_resolution_manager_other_branch_denies();
  test_lost_found_resolution_trainer_denies();
  test_lost_found_resolution_inactive_session_denies();

  /* policy: branch deletion */
  test_branch_deletion_sysadmin_allows();
  test_branch_deletion_manager_denies();
  test_branch_deletion_trainer_denies();
  test_branch_deletion_member_denies();
  test_branch_deletion_inactive_session_denies();
  test_branch_deletion_blank_branch_denies();
  test_branch_deletion_null_branch_denies();

  /* policy: member deletion */
  test_member_deletion_sysadmin_allows_any();
  test_member_deletion_manager_own_branch_zero_dues_allows();
  test_member_deletion_manager_own_branch_with_dues_denies();
  test_member_deletion_manager_other_branch_denies();
  test_member_deletion_trainer_denies();
  test_member_deletion_inactive_session_denies();
  test_member_deletion_unknown_member_id_denies();

  /* policy: staff deletion */
  test_staff_deletion_sysadmin_allows_any();
  test_staff_deletion_manager_own_branch_trainer_allows();
  test_staff_deletion_manager_own_branch_manager_denies();
  test_staff_deletion_manager_other_branch_denies();
  test_staff_deletion_trainer_denies();
  test_staff_deletion_inactive_session_denies();
  test_staff_deletion_unknown_staff_id_denies();

  /* policy: branch creation */
  test_branch_creation_sysadmin_allows();
  test_branch_creation_manager_denies();
  test_branch_creation_trainer_denies();
  test_branch_creation_member_denies();
  test_branch_creation_inactive_session_denies();

  /* policy: branch rename */
  test_branch_rename_sysadmin_allows();
  test_branch_rename_manager_denies();
  test_branch_rename_trainer_denies();
  test_branch_rename_member_denies();
  test_branch_rename_inactive_session_denies();

  /* policy: staff creation */
  test_staff_creation_sysadmin_trainer_any_branch_allows();
  test_staff_creation_sysadmin_manager_any_branch_allows();
  test_staff_creation_manager_trainer_own_branch_allows();
  test_staff_creation_manager_manager_own_branch_denies();
  test_staff_creation_manager_trainer_other_branch_denies();
  test_staff_creation_trainer_denies();
  test_staff_creation_member_denies();
  test_staff_creation_inactive_session_denies();
  test_staff_creation_blank_branch_denies();
  test_staff_creation_unknown_branch_denies();
  test_staff_creation_invalid_role_denies();

  /* policy: gym member creation */
  test_gym_member_creation_blank_branch_denies();
  test_gym_member_creation_unknown_branch_denies();
  test_gym_member_creation_valid_branch_allows();

  /* policy: branch name validity */
  test_branch_name_is_valid_blank_denies();
  test_branch_name_is_valid_null_denies();
  test_branch_name_is_valid_unknown_denies();
  test_branch_name_is_valid_known_allows();

  /* policy: branch listing */
  test_branch_listing_sysadmin_allows();
  test_branch_listing_manager_allows();
  test_branch_listing_member_allows();
  test_branch_listing_inactive_session_denies();

  /* policy: member listing */
  test_member_listing_sysadmin_any_branch_allows();
  test_member_listing_sysadmin_empty_branch_allows();
  test_member_listing_manager_own_branch_allows();
  test_member_listing_manager_other_branch_denies();
  test_member_listing_trainer_own_branch_allows();
  test_member_listing_trainer_other_branch_denies();
  test_member_listing_inactive_session_denies();
  test_member_listing_blank_branch_non_sysadmin_denies();

  /* policy: lost and found view */
  test_lost_found_view_sysadmin_any_branch_allows();
  test_lost_found_view_manager_own_branch_allows();
  test_lost_found_view_manager_other_branch_denies();
  test_lost_found_view_trainer_own_branch_allows();
  test_lost_found_view_trainer_other_branch_denies();
  test_lost_found_view_inactive_session_denies();
  test_lost_found_view_blank_branch_non_sysadmin_denies();

  /* policy: lost and found report */
  test_lost_found_report_sysadmin_any_branch_allows();
  test_lost_found_report_manager_own_branch_allows();
  test_lost_found_report_manager_other_branch_denies();
  test_lost_found_report_member_own_branch_allows();
  test_lost_found_report_member_other_branch_denies();
  test_lost_found_report_inactive_session_denies();
  test_lost_found_report_blank_branch_denies();

  printf("\n\nAll tests passed.\n\n");
  return 0;
}
