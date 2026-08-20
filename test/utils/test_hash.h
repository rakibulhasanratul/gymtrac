#ifndef GYMTRAC_TEST_HASH_H
#define GYMTRAC_TEST_HASH_H

void test_create_hash_known_vectors();
void test_create_hash_null_returns_zero();
void test_generate_salt_length_and_charset();
void test_generate_salt_null_is_safe();
void test_mix_salt_sandwich_output();
void test_mix_salt_empty_password();
void test_mix_salt_null_is_safe();
void test_compare_hash_equal();
void test_compare_hash_different();
void test_hash_value_to_string_decimal();
void test_hash_value_to_string_null_is_safe();
void test_parse_hash_value_decimal();
void test_parse_hash_value_invalid_returns_zero();
void test_hash_value_to_string_parse_round_trip();
void test_create_hash_consistency();
void test_different_salts_different_hashes();

#endif
