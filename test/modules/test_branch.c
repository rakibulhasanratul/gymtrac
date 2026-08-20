#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "../../src/modules/branch.h"
#include "../../src/settings.h"
#include "../../src/utils/file_util.h"
#include "test_branch.h"

/**
 * Removes the test branches file from the test_data directory.
 * Called once at startup from test_main, not between individual tests.
 */
void cleanup_branches_file()
{
  char path[PATH_BUFFER_SIZE];
  build_file_path(GYM_BRANCHES_FILENAME, path, PATH_BUFFER_SIZE);
  remove(path);
}

/**
 * Verifies that add_branch adds a branch and branch_exists finds it.
 */
void test_add_branch_and_exists()
{
  cleanup_branches_file();
  load_branches();

  assert(add_branch("Dhanmondi") == true);
  assert(branch_exists("Dhanmondi") == true);
}

/**
 * Verifies that add_branch rejects a duplicate branch name.
 */
void test_add_branch_rejects_duplicate()
{
  assert(add_branch("Dhanmondi") == false);
}

/**
 * Verifies that add_branch rejects an empty string.
 */
void test_add_branch_rejects_empty()
{
  assert(add_branch("") == false);
}

/**
 * Verifies that add_branch rejects a NULL input.
 */
void test_add_branch_rejects_null()
{
  assert(add_branch(NULL) == false);
}

/**
 * Verifies that branch_exists returns false for a non-existent branch.
 */
void test_branch_exists_returns_false_for_missing()
{
  assert(branch_exists("NonExistent") == false);
}

/**
 * Verifies that branch_exists returns false for NULL input.
 */
void test_branch_exists_returns_false_for_null()
{
  assert(branch_exists(NULL) == false);
}

/**
 * Verifies that load_branches loads all added branches.
 */
void test_load_branches()
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
}

/**
 * Verifies that load_branches returns 0 when no branches exist.
 */
void test_load_branches_returns_zero_when_empty()
{
  cleanup_branches_file();
  load_branches();

  assert(get_branch_count() == 0);
}

/**
 * Verifies that add_branch respects the maximum branch count.
 */
void test_add_branch_respects_max_count()
{
  cleanup_branches_file();
  load_branches();

  char name[BRANCH_NAME_BUFFER_SIZE];
  for (int index = 0; index < BRANCH_COUNT_MAX; index++)
  {
    snprintf(name, BRANCH_NAME_BUFFER_SIZE, "Branch%d", index);
    assert(add_branch(name) == true);
  }

  assert(add_branch("OverflowBranch") == false);
}

/**
 * Verifies that branch names are stored and compared exactly.
 */
void test_branch_names_are_case_sensitive()
{
  cleanup_branches_file();
  load_branches();

  add_branch("Dhanmondi");
  assert(branch_exists("Dhanmondi") == true);
  assert(branch_exists("dhanmondi") == false);
  assert(branch_exists("DHANMONDI") == false);
}

/**
 * Verifies that get_branch_count returns the correct count.
 */
void test_get_branch_count()
{
  cleanup_branches_file();
  load_branches();

  assert(get_branch_count() == 0);

  add_branch("One");
  assert(get_branch_count() == 1);

  add_branch("Two");
  assert(get_branch_count() == 2);
}

/**
 * Verifies that get_branch_name returns NULL for invalid indices.
 */
void test_get_branch_name()
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
}
