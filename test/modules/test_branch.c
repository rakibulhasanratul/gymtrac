#include "../test_settings.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "../../src/modules/branch.h"
#include "../../src/settings.h"
#include "../../src/utils/file_util.h"
#include "test_branch.h"

/**
 * Removes the test branches file to start with a clean slate.
 */
static void cleanup_branches_file()
{
  char path[PATH_BUFFER_SIZE];
  build_file_path(GYM_BRANCHES_FILENAME, path, PATH_BUFFER_SIZE);
  remove(path);
}

/**
 * Verifies that add_branch adds a branch and branch_exists finds it.
 */
static void test_add_branch_and_exists()
{
  cleanup_branches_file();
  load_branches();

  assert(add_branch("Dhanmondi") == true);
  assert(branch_exists("Dhanmondi") == true);

  cleanup_branches_file();
}

/**
 * Verifies that add_branch rejects a duplicate branch name.
 */
static void test_add_branch_rejects_duplicate()
{
  cleanup_branches_file();
  load_branches();

  assert(add_branch("Gulshan") == true);
  assert(add_branch("Gulshan") == false);

  cleanup_branches_file();
}

/**
 * Verifies that add_branch rejects an empty string.
 */
static void test_add_branch_rejects_empty()
{
  cleanup_branches_file();
  load_branches();

  assert(add_branch("") == false);

  cleanup_branches_file();
}

/**
 * Verifies that add_branch rejects a NULL input.
 */
static void test_add_branch_rejects_null()
{
  cleanup_branches_file();
  load_branches();

  assert(add_branch(NULL) == false);

  cleanup_branches_file();
}

/**
 * Verifies that branch_exists returns false for a non-existent branch.
 */
static void test_branch_exists_returns_false_for_missing()
{
  cleanup_branches_file();
  load_branches();

  assert(add_branch("Uttara") == true);
  assert(branch_exists("NonExistent") == false);

  cleanup_branches_file();
}

/**
 * Verifies that branch_exists returns false for NULL input.
 */
static void test_branch_exists_returns_false_for_null()
{
  cleanup_branches_file();
  load_branches();

  assert(branch_exists(NULL) == false);

  cleanup_branches_file();
}

/**
 * Verifies that load_branches loads all added branches.
 */
static void test_load_branches()
{
  cleanup_branches_file();
  load_branches();

  add_branch("Banani");
  add_branch("Mirpur");
  add_branch("Bashundhara");

  load_branches();

  assert(get_branch_count() == 3);
  assert(strcmp(get_branch_name(0), "Banani") == 0);
  assert(strcmp(get_branch_name(1), "Mirpur") == 0);
  assert(strcmp(get_branch_name(2), "Bashundhara") == 0);

  cleanup_branches_file();
}

/**
 * Verifies that load_branches returns 0 when no branches exist.
 */
static void test_load_branches_returns_zero_when_empty()
{
  cleanup_branches_file();
  load_branches();

  assert(get_branch_count() == 0);

  cleanup_branches_file();
}

/**
 * Verifies that add_branch respects the maximum branch count.
 */
static void test_add_branch_respects_max_count()
{
  cleanup_branches_file();
  load_branches();

  char name[BRANCH_NAME_BUFFER_SIZE];
  for (int i = 0; i < BRANCH_COUNT_MAX; i++)
  {
    snprintf(name, BRANCH_NAME_BUFFER_SIZE, "Branch%d", i);
    assert(add_branch(name) == true);
  }

  assert(add_branch("OverflowBranch") == false);

  cleanup_branches_file();
}

/**
 * Verifies that branch names are stored and compared exactly.
 */
static void test_branch_names_are_case_sensitive()
{
  cleanup_branches_file();
  load_branches();

  add_branch("Dhanmondi");
  assert(branch_exists("Dhanmondi") == true);
  assert(branch_exists("dhanmondi") == false);
  assert(branch_exists("DHANMONDI") == false);

  cleanup_branches_file();
}

/**
 * Verifies that get_branch_count returns the correct count.
 */
static void test_get_branch_count()
{
  cleanup_branches_file();
  load_branches();

  assert(get_branch_count() == 0);

  add_branch("One");
  assert(get_branch_count() == 1);

  add_branch("Two");
  assert(get_branch_count() == 2);

  cleanup_branches_file();
}

/**
 * Verifies that get_branch_name returns NULL for invalid indices.
 */
static void test_get_branch_name()
{
  cleanup_branches_file();
  load_branches();

  add_branch("Alpha");
  add_branch("Beta");

  assert(get_branch_name(0) != NULL);
  assert(strcmp(get_branch_name(0), "Alpha") == 0);
  assert(get_branch_name(1) != NULL);
  assert(strcmp(get_branch_name(1), "Beta") == 0);
  assert(get_branch_name(2) == NULL);
  assert(get_branch_name(-1) == NULL);

  cleanup_branches_file();
}

/**
 * Runs every branch module unit test, aborting on the first failure.
 */
void run_all_branch_tests()
{
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
}
