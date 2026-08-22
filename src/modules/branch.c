#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "../settings.h"
#include "../utils/file_util.h"
#include "branch.h"
#include "user.h"

static char branches[BRANCH_COUNT_MAX][BRANCH_NAME_BUFFER_SIZE];
static int branch_count;

// Removes the branch at index from memory by shifting later names left.
static void remove_branch_at(int index)
{
  for (int i = index; i < branch_count - 1; i++)
    strcpy(branches[i], branches[i + 1]);

  branch_count--;
}

// Rewrites the branches file from memory, skipping the branch at index.
//
// Used by delete_branch so the persisted file never contains the removed
// branch, matching the file-first ordering of add_branch.
static bool rewrite_branches_file_without(int index)
{
  FILE *file = fopen(GYM_BRANCHES_FILE_PATH, "w");
  if (file == NULL)
  {
    LOG_ERROR("Error: Failed to open branches file for writing.");
    return false;
  }

  for (int i = 0; i < branch_count; i++)
  {
    if (i != index && !write_line_to_file(file, branches[i]))
    {
      fclose(file);
      LOG_ERROR("Error: Failed to write branch record.");
      return false;
    }
  }

  fclose(file);
  return true;
}

int load_branches()
{
  branch_count = 0;

  FILE *file = fopen(GYM_BRANCHES_FILE_PATH, "r");
  if (file == NULL)
    return 0;

  char line[BRANCH_NAME_BUFFER_SIZE];
  while (branch_count < BRANCH_COUNT_MAX && read_line_from_file(file, line, BRANCH_NAME_BUFFER_SIZE))
  {
    if (strlen(line) > 0)
    {
      strcpy(branches[branch_count], line);
      branch_count++;
    }
  }

  fclose(file);
  return branch_count;
}

bool branch_exists(const char branch_name[])
{
  if (branch_name == NULL || strlen(branch_name) == 0)
    return false;

  for (int i = 0; i < branch_count; i++)
  {
    if (strcmp(branches[i], branch_name) == 0)
      return true;
  }

  return false;
}

bool add_branch(const char branch_name[])
{
  if (branch_name == NULL || strlen(branch_name) == 0)
  {
    LOG_ERROR("Error: Branch name cannot be empty.");
    return false;
  }

  if (branch_exists(branch_name))
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
  if (branch_name == NULL || strlen(branch_name) == 0)
    return false;

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

  if (!branch_exists(branch_name))
  {
    LOG_ERROR("Error: Branch '%s' does not exist.", branch_name);
    return false;
  }

  if (!ensure_branch_has_no_users(branch_name))
  {
    LOG_ERROR("Error: Branch '%s' still has assigned users.", branch_name);
    return false;
  }

  int index = -1;
  for (int i = 0; i < branch_count; i++)
  {
    if (strcmp(branches[i], branch_name) == 0)
    {
      index = i;
      break;
    }
  }

  if (index < 0)
  {
    LOG_ERROR("Error: Branch '%s' not found in memory.", branch_name);
    return false;
  }

  if (!rewrite_branches_file_without(index))
  {
    LOG_ERROR("Error: Failed to rewrite branches file.");
    return false;
  }

  remove_branch_at(index);
  return true;
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
