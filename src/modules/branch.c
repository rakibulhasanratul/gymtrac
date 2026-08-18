#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "../settings.h"
#include "../utils/file_util.h"
#include "branch.h"

static char branches[BRANCH_COUNT_MAX][BRANCH_NAME_BUFFER_SIZE];
static int branch_count;

int load_branches()
{
  branch_count = 0;

  char path[PATH_BUFFER_SIZE];
  build_file_path(GYM_BRANCHES_FILENAME, path, PATH_BUFFER_SIZE);

  FILE *file = fopen(path, "r");
  if (file == NULL)
    return 0;

  char line[BRANCH_NAME_BUFFER_SIZE];
  while (branch_count < BRANCH_COUNT_MAX && read_line_from_file(file, line, BRANCH_NAME_BUFFER_SIZE))
  {
    if (strlen(line) > 0)
    {
      strncpy(branches[branch_count], line, BRANCH_NAME_BUFFER_SIZE - 1);
      branches[branch_count][BRANCH_NAME_BUFFER_SIZE - 1] = '\0';
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
    return false;

  if (branch_exists(branch_name))
    return false;

  if (branch_count >= BRANCH_COUNT_MAX)
    return false;

  char path[PATH_BUFFER_SIZE];
  build_file_path(GYM_BRANCHES_FILENAME, path, PATH_BUFFER_SIZE);

  FILE *file = fopen(path, "a");
  if (file == NULL)
    return false;

  bool success = write_line_to_file(file, branch_name);
  fclose(file);

  if (success)
  {
    strncpy(branches[branch_count], branch_name, BRANCH_NAME_BUFFER_SIZE - 1);
    branches[branch_count][BRANCH_NAME_BUFFER_SIZE - 1] = '\0';
    branch_count++;
  }

  return success;
}

int get_branch_count()
{
  return branch_count;
}

const char *get_branch_name(int index)
{
  if (index < 0 || index >= branch_count)
    return NULL;

  return branches[index];
}
