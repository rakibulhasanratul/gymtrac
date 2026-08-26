#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "../settings.h"
#include "../utils/file_util.h"
#include "branch.h"
#include "user.h"

static char branches[BRANCH_COUNT_MAX][BRANCH_NAME_BUFFER_SIZE];
static int branch_count;

// Persists the in-memory branch list to the branches file, one name per line.
static bool save_branches_to_file()
{
  // Map rows to pointers because write_lines_to_file reads through char pointers.
  const char *line_maps[branch_count];
  for (int i = 0; i < branch_count; i++) line_maps[i] = branches[i];

  if (!write_lines_to_file(GYM_BRANCHES_FILE_PATH, line_maps, branch_count))
  {
    LOG_ERROR("Error: Failed to rewrite branches file.");
    return false;
  }

  return true;
}

int load_branches()
{
  // Map rows to pointers because read_lines_from_file fills through char pointers.
  char *line_maps[BRANCH_COUNT_MAX];
  for (int i = 0; i < BRANCH_COUNT_MAX; i++) line_maps[i] = branches[i];

  branch_count = read_lines_from_file(GYM_BRANCHES_FILE_PATH, line_maps, BRANCH_COUNT_MAX, BRANCH_NAME_BUFFER_SIZE);
  return branch_count;
}

int find_branch(const char branch_name[])
{
  if (branch_name == NULL || strlen(branch_name) == 0) return -1;

  for (int i = 0; i < branch_count; i++)
  {
    if (strcmp(branches[i], branch_name) == 0) return i;
  }

  return -1;
}

bool add_branch(const char branch_name[])
{
  if (branch_name == NULL || strlen(branch_name) == 0)
  {
    LOG_ERROR("Error: Branch name cannot be empty.");
    return false;
  }

  if (find_branch(branch_name) != -1)
  {
    LOG_ERROR("Error: Branch '%s' already exists.", branch_name);
    return false;
  }

  if (branch_count >= BRANCH_COUNT_MAX)
  {
    LOG_ERROR("Error: Maximum branch count reached.");
    return false;
  }

  FILE *file = fopen(GYM_BRANCHES_FILE_PATH, "a");
  if (file == NULL)
  {
    LOG_ERROR("Error: Failed to open branches file for appending.");
    return false;
  }

  bool success = write_line_to_file(file, branch_name);
  fclose(file);

  if (success)
  {
    strcpy(branches[branch_count], branch_name);
    branch_count++;
  }

  return success;
}

bool ensure_branch_has_no_users(const char branch_name[])
{
  if (branch_name == NULL || strlen(branch_name) == 0) return false;

  return branch_manager_count(branch_name) == 0 && branch_trainer_count(branch_name) == 0 &&
         branch_member_count(branch_name) == 0;
}

bool delete_branch(const char branch_name[])
{
  if (branch_name == NULL || strlen(branch_name) == 0)
  {
    LOG_ERROR("Error: Branch name cannot be empty.");
    return false;
  }

  int index = find_branch(branch_name);
  if (index < 0)
  {
    LOG_ERROR("Error: Branch '%s' does not exist.", branch_name);
    return false;
  }

  if (!ensure_branch_has_no_users(branch_name))
  {
    LOG_ERROR("Error: Branch '%s' still has assigned users.", branch_name);
    return false;
  }

  // Shift every later name one slot left over the removed branch.
  for (int i = index; i < branch_count - 1; i++) strcpy(branches[i], branches[i + 1]);

  // Blank the vacated tail slot and shrink the count.
  branches[branch_count - 1][0] = '\0';
  branch_count--;

  return save_branches_to_file();
}

bool update_branch_name(const char old_branch_name[], const char new_branch_name[])
{
  if (old_branch_name == NULL || strlen(old_branch_name) == 0)
  {
    LOG_ERROR("Error: Old branch name cannot be empty.");
    return false;
  }

  if (new_branch_name == NULL || strlen(new_branch_name) == 0)
  {
    LOG_ERROR("Error: New branch name cannot be empty.");
    return false;
  }

  // Find the match up front so the rename reuses the index without rescanning.
  int index = find_branch(old_branch_name);
  if (index < 0)
  {
    LOG_ERROR("Error: Branch '%s' does not exist.", old_branch_name);
    return false;
  }

  // Any taken new name is rejected here, including a rename to the same name.
  if (find_branch(new_branch_name) != -1)
  {
    LOG_ERROR("Error: Branch '%s' already exists.", new_branch_name);
    return false;
  }

  // Cascade the rename to staff and member records before changing the branch list.
  if (!rename_branch_for_all_users(old_branch_name, new_branch_name)) return false;

  strcpy(branches[index], new_branch_name);

  return save_branches_to_file();
}

int get_branch_count()
{
  return branch_count;
}

const char *get_branch_name(int index)
{
  if (index < 0 || index >= branch_count)
  {
    LOG_ERROR("Error: Branch index %d is out of range.", index);
    return NULL;
  }

  return branches[index];
}
