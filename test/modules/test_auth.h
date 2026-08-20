#ifndef GYMTRAC_TEST_AUTH_H
#define GYMTRAC_TEST_AUTH_H

void test_hash_password_valid_format();
void test_hash_password_unique_salts();
void test_verify_password_correct();
void test_verify_password_wrong();
void test_verify_password_empty_vs_nonempty();
void test_verify_password_empty_password();
void test_verify_password_null_returns_false();
void test_hash_password_null_is_safe();

#endif
