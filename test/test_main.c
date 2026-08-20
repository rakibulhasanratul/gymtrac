#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "../src/utils/rng.h"
#include "modules/test_auth.h"
#include "modules/test_branch.h"
#include "modules/test_user.h"
#include "utils/test_date_util.h"
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
  test_build_file_path_joins_correctly();
  test_build_file_path_null_inputs_are_safe();
  test_build_file_path_overflow_produces_empty();

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

  /* date_util */
  test_time_t_to_string_formats_date();
  test_time_t_to_string_invalid_input();
  test_string_to_time_t_parses_valid_dates();
  test_string_to_time_t_rejects_invalid();
  test_date_round_trip();
  test_leap_year_dates();
  test_add_months_clamps_day();
  test_add_months_year_boundary();
  test_add_months_preserves_day();
  test_days_between_calculates_difference();
  test_add_months_zero_returns_same();

  /* branch */
  test_add_branch_and_exists();
  test_add_branch_rejects_duplicate();
  test_add_branch_rejects_empty();
  test_add_branch_rejects_null();
  test_branch_exists_returns_false_for_missing();
  test_branch_exists_returns_false_for_null();
  test_load_branches();
  test_load_branches_returns_zero_when_empty();
  test_add_branch_respects_max_count();
  test_branch_names_are_case_sensitive();
  test_get_branch_count();
  test_get_branch_name();

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

  printf("All tests passed.\n");
  return 0;
}
