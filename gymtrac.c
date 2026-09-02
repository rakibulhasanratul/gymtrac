#include <ctype.h>
#include <limits.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

// settings like macros those are used in main.c
#define PASSWORD_HASH_BUFFER_SIZE 128 // salt (15 chars) + polynomial hash decimal + null + overflow buffer
// Credentials for the sysadmin account seeded on first run.
#define DEFAULT_SYSADMIN_USERNAME "admin"
#define DEFAULT_SYSADMIN_PASSWORD "admin123"
// main verifies this is exactly one character
#define FIELD_DELIMITER_STRING "|"
#define FIELD_DELIMITER FIELD_DELIMITER_STRING[0]

// function prototypes
void seed_rng(unsigned int seed);
int load_branches();
int load_branch_staff();
int load_gym_members();
int load_suspensions();
int load_payments();
int load_lost_and_found_records();
int load_sysadmins();
void hash_password(const char password[], char *destination);
unsigned long int create_sysadmin(const char username[], const char password_hash[]);
int auto_suspend_overdue_members();
void run_main_menu();

int main()
{
  if (strlen(FIELD_DELIMITER_STRING) != 1)
  {
    printf("FIELD_DELIMITER_STRING must be exactly one character long.");
    return 1;
  }

  seed_rng((unsigned int)time(NULL));

  load_branches();
  load_branch_staff();
  load_gym_members();
  load_suspensions();
  load_payments();
  load_lost_and_found_records();

  int sysadmin_count = load_sysadmins();
  if (sysadmin_count == 0)
  {
    char hashed_password[PASSWORD_HASH_BUFFER_SIZE];
    hash_password(DEFAULT_SYSADMIN_PASSWORD, hashed_password);
    create_sysadmin(DEFAULT_SYSADMIN_USERNAME, hashed_password);
  }

  int auto_suspended = auto_suspend_overdue_members();
  printf("Boot complete. Auto-suspended %d overdue member(s).\n", auto_suspended);

  run_main_menu();
  return 0;
}

// Rest of the Settings-like macros

// Buffer sizes for fixed-size char array fields.
#define FULL_NAME_BUFFER_SIZE 64
#define USERNAME_BUFFER_SIZE 32

#define EMAIL_BUFFER_SIZE 64
#define PHONE_BUFFER_SIZE 11         // phone number pattern: 01XXXXXXXXX
#define BRANCH_NAME_BUFFER_SIZE 128  // should allow detailed location names
#define REASON_BUFFER_SIZE 1024      // suspension/request reason field
#define DESCRIPTION_BUFFER_SIZE 2048 // description field for lost and found
#define TRX_ID_BUFFER_SIZE 64        // transaction id field
#define DATETIME_BUFFER_SIZE 20      // yyyy-mm-dd hh:mm:ss + null terminator
#define TIMEZONE_OFFSET_HOURS 6      // hours added to UTC; Bangladesh Standard Time is UTC+6
#define FIELD_BUFFER_SIZE 256        // buffer for a single field when splitting a pipe-delimited record line
#define LINE_BUFFER_SIZE 4096        // one full record line. Why long? I was even getting overflow warnings for 2048!

// Hashing internals.
#define SALT_BUFFER_SIZE 16        // 15 printable chars + null
#define HASH_STRING_BUFFER_SIZE 32 // decimal representation of unsigned long + null
#define POLYNOMIAL_MULTIPLIER 31   // multiplier matching Java's String.hashCode()
#define SALT_CHARSET "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789"

// Capacity limits: branch count, per-branch caps, and derived global caps.
#define BRANCH_COUNT_MAX 32
#define MAX_MANAGERS_PER_BRANCH 1
#define MAX_TRAINERS_PER_BRANCH 5
#define MAX_MEMBERS_PER_BRANCH 100
#define MAX_SYSTEM_ADMINS 1
#define MAX_BRANCH_MANAGERS (BRANCH_COUNT_MAX * MAX_MANAGERS_PER_BRANCH)
#define MAX_TRAINERS (BRANCH_COUNT_MAX * MAX_TRAINERS_PER_BRANCH)
#define MAX_GYM_MEMBERS (BRANCH_COUNT_MAX * MAX_MEMBERS_PER_BRANCH)
#define MAX_SUSPENSION_RECORDS (MAX_GYM_MEMBERS * 2) // a member may be suspended more than once
#define MAX_PAYMENT_RECORDS (MAX_GYM_MEMBERS * 4)    // a member pays once per interval, many times over
#define MAX_LOST_FOUND_RECORDS (MAX_GYM_MEMBERS * 2) // a member may report items many times over

// Member economics: default plan assigned on approval and dues grace period.
#define DEFAULT_PLAN_AMOUNT 1000      // default plan payable amount in whole Taka
#define DEFAULT_PLAN_INTERVAL_DAYS 30 // default plan payment interval in days
#define MAX_UNPAID_DAYS 90            // days past the due date before auto-suspension
#define AUTO_SUSPENSION_REASON "Auto: unpaid dues"

// File storage.
#define GYM_BRANCHES_FILE_PATH "branches.txt"
#define SYSADMINS_FILE_PATH "sysadmins.txt"
#define BRANCH_STAFF_FILE_PATH "branch_staff.txt"
#define GYM_MEMBERS_FILE_PATH "gym_members.txt"
#define SUSPENSIONS_FILE_PATH "suspensions.txt"
#define PAYMENTS_FILE_PATH "payments.txt"
#define LOST_FOUND_FILE_PATH "lost_found.txt"
#define MAX_RECORD_FIELDS 14 // maximum number of fields in any pipe-delimited record

// type definitions

// Common numeric id type shared by every record.
typedef unsigned long int id_t;

// Calendar timestamp with second precision.
typedef struct
{
  int year;   // full year, e.g. 2026; must be at least EPOCH_YEAR
  int month;  // 1-12
  int day;    // 1-31, valid for the month
  int hour;   // 0-23
  int minute; // 0-59
  int second; // 0-59
} datetime_t;

// Datetime meaning "nothing recorded yet"; an open suspension uses it until
// unsuspension. Matches epoch 0, the value an unrecorded datetime stores in
// data files, so the sentinel survives save/load round-trips.
#define EMPTY_DATETIME ((datetime_t){1970, 1, 1, 0, 0, 0})

// System administrator record, seeded on first run.
typedef struct
{
  id_t id;
  char username[USERNAME_BUFFER_SIZE];
  char password_hash[PASSWORD_HASH_BUFFER_SIZE];
} sysadmin_t;

// Staff role distinguishing a branch trainer from a branch manager.
typedef unsigned char staff_role_t;
#define TRAINER 0
#define BRANCH_MANAGER 1

// Branch staff record covering both trainers and branch managers.
typedef struct
{
  id_t id;
  char full_name[FULL_NAME_BUFFER_SIZE];
  char email[EMAIL_BUFFER_SIZE];
  char phone_number[PHONE_BUFFER_SIZE];
  char gym_branch[BRANCH_NAME_BUFFER_SIZE];
  char username[USERNAME_BUFFER_SIZE];
  char password_hash[PASSWORD_HASH_BUFFER_SIZE];
  datetime_t joined_at;
  staff_role_t role;
} branch_staff_t;

// Subscription plan with fee amount and payment interval.
typedef struct
{
  unsigned int payable_amount;
  unsigned int interval_days;
} subscription_plan_t;

// Lifecycle status of a gym member's membership.
typedef unsigned char membership_status_t;
#define MEMBERSHIP_ON_HOLD 0
#define MEMBERSHIP_ACTIVE 1
#define MEMBERSHIP_SUSPENDED 2
#define MEMBERSHIP_CANCELLED 3

// Gym member record with plan, dues, and membership status.
typedef struct
{
  id_t id;
  char full_name[FULL_NAME_BUFFER_SIZE];
  char email[EMAIL_BUFFER_SIZE];
  char phone_number[PHONE_BUFFER_SIZE];
  char gym_branch[BRANCH_NAME_BUFFER_SIZE];
  char username[USERNAME_BUFFER_SIZE];
  char password_hash[PASSWORD_HASH_BUFFER_SIZE];
  datetime_t joined_at;
  datetime_t last_payment_date;
  unsigned int due_amount;
  subscription_plan_t plan;
  membership_status_t status;
} gym_member_t;

// Payment method: cash or digital.
typedef unsigned char transaction_t;
#define CASH_TRANSACTION 0
#define DIGITAL_TRANSACTION 1

// Lifecycle status of a payment.
typedef unsigned char payment_status_t;
#define PAYMENT_PENDING 0
#define PAYMENT_COMPLETED 1
#define PAYMENT_FAILED 2
#define PAYMENT_INVALID 3

// Payment made by a gym member; each belongs to exactly one member.
typedef struct
{
  id_t id;
  id_t gym_member_id;
  unsigned int amount;
  datetime_t transaction_time;
  transaction_t transaction_type;
  char transaction_id[TRX_ID_BUFFER_SIZE];
  payment_status_t status;
} payment_t;

// Arguments carried to record a digital payment.
typedef struct
{
  id_t request_id;
  id_t gym_member_id;
  unsigned int amount;
  datetime_t transaction_time;
  char transaction_id[TRX_ID_BUFFER_SIZE];
  payment_status_t status;
} digital_payment_request_t;

// Read-only profile view of a gym member.
typedef struct
{
  id_t id;
  char full_name[FULL_NAME_BUFFER_SIZE];
  char email[EMAIL_BUFFER_SIZE];
  char phone_number[PHONE_BUFFER_SIZE];
  char gym_branch[BRANCH_NAME_BUFFER_SIZE];
  char username[USERNAME_BUFFER_SIZE];
  datetime_t joined_at;
  subscription_plan_t plan;
} gym_member_profile_t;

// Record of a member suspension with reason and dates.
typedef struct
{
  id_t id;
  id_t gym_member_id;
  char reason[REASON_BUFFER_SIZE];
  datetime_t suspension_date;
  datetime_t unsuspension_date;
} suspension_record_t;

// Lost or found item report, resolved by a manager or the system administrator.
typedef struct
{
  id_t id;
  char description[DESCRIPTION_BUFFER_SIZE];
  char reporter_username[USERNAME_BUFFER_SIZE];
  char gym_branch[BRANCH_NAME_BUFFER_SIZE];
  datetime_t reported_at;
  char resolver_username[USERNAME_BUFFER_SIZE];
} lost_and_found_record_t;

// Top-level role of a logged-in user.
typedef unsigned char user_role_t;
#define USER_ROLE_SYSADMIN 0
#define USER_ROLE_BRANCH_MANAGER 1
#define USER_ROLE_TRAINER 2
#define USER_ROLE_MEMBER 3

// Context of the logged-in user; gates access by role and branch.
typedef struct
{
  user_role_t role;
  id_t user_id;
  char username[USERNAME_BUFFER_SIZE];
  char branch_name[BRANCH_NAME_BUFFER_SIZE];
} session_t;

// ===================== string_util.c =====================

bool is_blank_string(const char text[]);

void trim(const char text[], char *destination, int destination_capacity)
{
  if (destination == NULL || text == NULL || destination_capacity < 2) return;

  int start_index = 0;
  while (text[start_index] != '\0' && isspace((unsigned char)text[start_index])) start_index++;

  int end_index = start_index;
  while (text[end_index] != '\0') end_index++;

  while (end_index > start_index && isspace((unsigned char)text[end_index - 1])) end_index--;

  int write_index = 0;
  while (start_index < end_index && write_index < destination_capacity - 1)
  {
    destination[write_index] = text[start_index];
    write_index++;
    start_index++;
  }
  destination[write_index] = '\0';
}

int split(const char text[], char delimiter, char destination[][FIELD_BUFFER_SIZE], int destination_capacity)
{
  if (text == NULL || destination == NULL || destination_capacity < 1) return 0;

  int part_count = 0;
  int cursor_index = 0;
  while (part_count < destination_capacity)
  {
    int field_index = 0;
    while (text[cursor_index] != '\0' && text[cursor_index] != delimiter)
    {
      if (field_index < FIELD_BUFFER_SIZE - 1)
      {
        destination[part_count][field_index] = text[cursor_index];
        field_index++;
      }
      cursor_index++;
    }
    destination[part_count][field_index] = '\0';
    part_count++;
    if (text[cursor_index] == '\0') break;
    cursor_index++;
  }

  return part_count;
}

unsigned int string_to_unsigned_int(const char text[])
{
  if (is_blank_string(text)) return 0;

  unsigned int accumulated = 0;
  for (int i = 0; text[i] != '\0'; i++)
  {
    if (!isdigit((unsigned char)text[i])) return 0;
    unsigned int digit = (unsigned int)(text[i] - '0');
    if (accumulated > (UINT_MAX - digit) / 10u) return 0;
    accumulated = accumulated * 10u + digit;
  }

  return accumulated;
}

unsigned long int string_to_unsigned_long_int(const char text[])
{
  if (is_blank_string(text)) return 0;

  unsigned long int accumulated = 0;
  for (int i = 0; text[i] != '\0'; i++)
  {
    if (!isdigit((unsigned char)text[i])) return 0;
    unsigned long int digit = (unsigned long int)(text[i] - '0');
    if (accumulated > (ULONG_MAX - digit) / 10ul) return 0;
    accumulated = accumulated * 10ul + digit;
  }

  return accumulated;
}

char *to_lowercase(char text[])
{
  if (text == NULL) return NULL;

  for (int i = 0; text[i] != '\0'; i++) text[i] = (char)tolower((unsigned char)text[i]);

  return text;
}

char *to_uppercase(char text[])
{
  if (text == NULL) return NULL;

  for (int i = 0; text[i] != '\0'; i++) text[i] = (char)toupper((unsigned char)text[i]);

  return text;
}

bool sanitize_field(const char text[], char *destination, int destination_capacity)
{
  if (text == NULL || destination == NULL || destination_capacity < 2) return false;

  int write_index = 0;
  for (int i = 0; text[i] != '\0'; i++)
  {
    unsigned char ch = (unsigned char)text[i];
    if (ch == (unsigned char)FIELD_DELIMITER || iscntrl(ch)) continue;
    if (write_index < destination_capacity - 1)
    {
      destination[write_index] = text[i];
      write_index++;
    }
  }
  destination[write_index] = '\0';

  return true;
}

bool is_blank_string(const char text[])
{
  if (text == NULL) return true;
  if (strlen(text) == 0) return true;
  for (int i = 0; text[i] != '\0'; i++)
    if (!isspace((unsigned char)text[i])) return false;
  return true;
}

// ===================== file_util.c =====================

bool read_line_from_file(FILE *file, char *destination, int destination_capacity)
{
  if (file == NULL || destination == NULL || destination_capacity < 2) return false;

  if (fgets(destination, destination_capacity, file) == NULL) return false;

  int length = (int)strlen(destination);
  if (length > 0 && destination[length - 1] == '\n')
  {
    destination[length - 1] = '\0';
    if (length > 1 && destination[length - 2] == '\r') destination[length - 2] = '\0';
    return true;
  }

  if (feof(file)) return true;

  while (!feof(file) && !ferror(file))
  {
    if (fgetc(file) == '\n') break;
  }
  return false;
}

bool write_line_to_file(FILE *file, const char line[])
{
  if (file == NULL || line == NULL) return false;
  if (fputs(line, file) == EOF) return false;
  if (fputc('\n', file) == EOF) return false;
  return true;
}

int read_lines_from_file(const char file_path[], char *destination[], int max_lines, int line_capacity)
{
  if (file_path == NULL || destination == NULL || max_lines < 1 || line_capacity < 2) return 0;

  FILE *file = fopen(file_path, "r");
  if (file == NULL) return 0;

  int line_count = 0;
  while (line_count < max_lines && read_line_from_file(file, destination[line_count], line_capacity))
  {
    if (!is_blank_string(destination[line_count])) line_count++;
  }

  fclose(file);
  return line_count;
}

bool write_lines_to_file(const char file_path[], const char *lines[], int line_count)
{
  if (file_path == NULL || lines == NULL || line_count < 0) return false;

  FILE *file = fopen(file_path, "w");
  if (file == NULL) return false;

  for (int i = 0; i < line_count; i++)
  {
    if (lines[i] == NULL || !write_line_to_file(file, lines[i]))
    {
      fclose(file);
      return false;
    }
  }

  fclose(file);
  return true;
}

// ===================== input.c =====================

bool input_positive_int(int *destination);

static inline void discard_remaining_input()
{
  int ch;
  while ((ch = getchar()) != '\n' && ch != EOF);
}

bool input_string(char *destination, int destination_capacity)
{
  if (destination == NULL || destination_capacity < 2) return false;

  if (fgets(destination, destination_capacity, stdin) == NULL) return false;

  int length = (int)strlen(destination);
  if (length > 0 && destination[length - 1] == '\n')
  {
    destination[length - 1] = '\0';
    if (length > 1 && destination[length - 2] == '\r') destination[length - 2] = '\0';
    return true;
  }

  discard_remaining_input();
  return true;
}

bool input_integer(int *destination)
{
  if (destination == NULL) return false;

  int matched = scanf("%d", destination);
  if (matched != 1)
  {
    discard_remaining_input();
    return false;
  }
  discard_remaining_input();
  return true;
}

bool input_positive_int(int *destination)
{
  if (!input_integer(destination)) return false;
  if (*destination <= 0) return false;
  return true;
}

// ===================== rng.c =====================

static unsigned int state[4];

static inline unsigned int power_of_2_to_unsigned_int(unsigned int n)
{
  return (unsigned int)pow(2.0, (double)n);
}

static inline unsigned int xor_32(unsigned int a, unsigned int b)
{
  unsigned int result = 0;
  unsigned int place = 1;
  while (a > 0 || b > 0)
  {
    unsigned int bit_a = a % 2;
    unsigned int bit_b = b % 2;
    if (bit_a != bit_b) result += place;
    a /= 2;
    b /= 2;
    place *= 2;
  }
  return result;
}

static inline unsigned int rotate_left_32(unsigned int x, unsigned int k)
{
  return x * power_of_2_to_unsigned_int(k) + x / power_of_2_to_unsigned_int(32 - k);
}

static inline void xoshiro128_next(void)
{
  unsigned int t = state[1] * power_of_2_to_unsigned_int(9);

  state[2] = xor_32(state[2], state[0]);
  state[3] = xor_32(state[3], state[1]);
  state[1] = xor_32(state[1], state[2]);
  state[0] = xor_32(state[0], state[3]);

  state[2] = xor_32(state[2], t);

  state[3] = rotate_left_32(state[3], 11);
}

void seed_rng(unsigned int seed)
{
  unsigned int mixed = seed + 2654435769;
  mixed = mixed * 2654435761 + 3266489917;

  state[0] = mixed;
  state[1] = mixed + power_of_2_to_unsigned_int(8);
  state[2] = mixed + power_of_2_to_unsigned_int(16);
  state[3] = mixed + power_of_2_to_unsigned_int(24);
}

unsigned int random_number(void)
{
  xoshiro128_next();
  return rotate_left_32(state[1] * 5, 7) * 9;
}

// ===================== hash.c =====================

typedef unsigned long hash_t;

void generate_salt(char *destination)
{
  if (destination == NULL) return;

  int charset_size = (int)strlen(SALT_CHARSET);
  for (int i = 0; i < SALT_BUFFER_SIZE - 1; i++) destination[i] = SALT_CHARSET[random_number() % charset_size];
  destination[SALT_BUFFER_SIZE - 1] = '\0';
}

void mix_salt(const char password[], const char salt[], char *destination)
{
  if (password == NULL || salt == NULL || destination == NULL) return;

  int write_index = 0;

  for (int i = 0; i < 7 && salt[i] != '\0'; i++) destination[write_index++] = salt[i];

  for (int i = 0; password[i] != '\0'; i++) destination[write_index++] = password[i];

  for (int i = 8; i < 15 && salt[i] != '\0'; i++) destination[write_index++] = salt[i];

  destination[write_index] = '\0';
}

hash_t create_hash(const char text[])
{
  if (text == NULL) return 0;

  hash_t hash_value = 0;
  for (int i = 0; text[i] != '\0'; i++) hash_value = POLYNOMIAL_MULTIPLIER * hash_value + (unsigned char)text[i];

  return hash_value;
}

bool compare_hash(hash_t stored, hash_t computed)
{
  return stored == computed;
}

void hash_value_to_string(hash_t value, char *destination)
{
  if (destination == NULL) return;

  sprintf(destination, "%lu", value);
}

hash_t parse_hash_value(const char text[])
{
  return string_to_unsigned_long_int(text);
}

// ===================== datetime_utils.c =====================

#define SECONDS_PER_HOUR 3600
#define SECONDS_PER_DAY (3600 * 24)
#define EPOCH_YEAR 1970

static const int DAYS_IN_MONTH[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

static inline bool is_leap_year(int year)
{
  if (year % 400 == 0) return true;
  if (year % 100 == 0) return false;
  if (year % 4 == 0) return true;
  return false;
}

static inline int days_in_month(int year, int month)
{
  if (month == 2 && is_leap_year(year)) return 29;
  return DAYS_IN_MONTH[month - 1];
}

static inline int days_in_year(int year)
{
  if (is_leap_year(year)) return 366;
  return 365;
}

datetime_t datetime_from_seconds(long long seconds_since_epoch);

datetime_t now_datetime()
{
  long long utc_seconds = (long long)time(NULL);
  return datetime_from_seconds(utc_seconds + (long long)TIMEZONE_OFFSET_HOURS * SECONDS_PER_HOUR);
}

long long datetime_to_seconds(const datetime_t datetime_payload)
{
  long long total_seconds = 0;

  for (int year = EPOCH_YEAR; year < datetime_payload.year; year++)
    total_seconds += (long long)days_in_year(year) * SECONDS_PER_DAY;

  for (int month = 1; month < datetime_payload.month; month++)
    total_seconds += (long long)days_in_month(datetime_payload.year, month) * SECONDS_PER_DAY;

  total_seconds += (long long)(datetime_payload.day - 1) * SECONDS_PER_DAY;
  total_seconds += (long long)datetime_payload.hour * SECONDS_PER_HOUR;
  total_seconds += (long long)datetime_payload.minute * 60;

  return total_seconds + datetime_payload.second;
}

datetime_t datetime_from_seconds(long long seconds_since_epoch)
{
  long long day_count = seconds_since_epoch / SECONDS_PER_DAY;
  long long remaining_seconds = seconds_since_epoch % SECONDS_PER_DAY;

  datetime_t result;
  result.year = EPOCH_YEAR;
  result.month = 1;
  result.day = 1;
  result.hour = (int)(remaining_seconds / SECONDS_PER_HOUR);
  result.minute = (int)(remaining_seconds % SECONDS_PER_HOUR / 60);
  result.second = (int)(remaining_seconds % 60);

  while (day_count >= days_in_year(result.year))
  {
    day_count -= days_in_year(result.year);
    result.year++;
  }

  while (day_count >= days_in_month(result.year, result.month))
  {
    day_count -= days_in_month(result.year, result.month);
    result.month++;
  }

  result.day += (int)day_count;
  return result;
}

bool format_datetime(const datetime_t datetime_payload, char *destination, int destination_capacity)
{
  if (destination == NULL || destination_capacity < DATETIME_BUFFER_SIZE) return false;

  snprintf(
    destination, (size_t)destination_capacity, "%04d-%02d-%02d %02d:%02d:%02d", datetime_payload.year,
    datetime_payload.month, datetime_payload.day, datetime_payload.hour, datetime_payload.minute,
    datetime_payload.second
  );
  return true;
}

bool parse_datetime(const char datetime_text[], datetime_t *destination)
{
  if (datetime_text == NULL || destination == NULL) return false;

  if (strlen(datetime_text) != DATETIME_BUFFER_SIZE - 1) return false;
  if (datetime_text[4] != '-' || datetime_text[7] != '-' || datetime_text[10] != ' ') return false;
  if (datetime_text[13] != ':' || datetime_text[16] != ':') return false;
  for (int i = 0; i < DATETIME_BUFFER_SIZE - 1; i++)
  {
    if (i == 4 || i == 7 || i == 10 || i == 13 || i == 16) continue;
    if (!isdigit((unsigned char)datetime_text[i])) return false;
  }

  int year = 0;
  int month = 0;
  int day = 0;
  int hour = 0;
  int minute = 0;
  int second = 0;
  if (sscanf(datetime_text, "%4d-%2d-%2d %2d:%2d:%2d", &year, &month, &day, &hour, &minute, &second) != 6) return false;

  if (year < EPOCH_YEAR || month < 1 || month > 12) return false;
  if (day < 1 || day > days_in_month(year, month)) return false;
  if (hour < 0 || hour > 23) return false;
  if (minute < 0 || minute > 59) return false;
  if (second < 0 || second > 59) return false;

  destination->year = year;
  destination->month = month;
  destination->day = day;
  destination->hour = hour;
  destination->minute = minute;
  destination->second = second;
  return true;
}

datetime_t add_days(const datetime_t date_payload, int days)
{
  long long shifted_seconds = datetime_to_seconds(date_payload) + (long long)days * SECONDS_PER_DAY;
  return datetime_from_seconds(shifted_seconds);
}

datetime_t add_months(const datetime_t date_payload, int months)
{
  datetime_t result = date_payload;
  result.month += months;

  while (result.month > 12)
  {
    result.month -= 12;
    result.year++;
  }
  while (result.month < 1)
  {
    result.month += 12;
    result.year--;
  }

  int last_day_of_month = days_in_month(result.year, result.month);
  if (result.day > last_day_of_month) result.day = last_day_of_month;

  return result;
}

int compare_datetime(const datetime_t left_payload, const datetime_t right_payload)
{
  long long left_seconds = datetime_to_seconds(left_payload);
  long long right_seconds = datetime_to_seconds(right_payload);
  if (left_seconds == right_seconds) return 0;
  return left_seconds < right_seconds ? -1 : 1;
}

int days_between(const datetime_t earlier_payload, const datetime_t later_payload)
{
  long long difference = datetime_to_seconds(later_payload) - datetime_to_seconds(earlier_payload);
  return (int)(difference / SECONDS_PER_DAY);
}

bool is_empty_datetime(const datetime_t datetime_payload)
{
  return compare_datetime(datetime_payload, EMPTY_DATETIME) == 0;
}

// ===================== session.c =====================

static session_t _session;

void initialize_session()
{
  _session.role = 0;
  _session.user_id = 0;
  _session.username[0] = '\0';
  _session.branch_name[0] = '\0';
}

void set_session_context(user_role_t role, id_t user_id, const char username[], const char branch_name[])
{
  _session.role = role;
  _session.user_id = user_id;

  if (username != NULL)
    strcpy(_session.username, username);
  else
    _session.username[0] = '\0';

  if (branch_name != NULL)
    strcpy(_session.branch_name, branch_name);
  else
    _session.branch_name[0] = '\0';
}

void clear_session_context()
{
  initialize_session();
}

bool session_is_active()
{
  return _session.user_id != 0;
}

bool session_is_sysadmin()
{
  return session_is_active() && _session.role == USER_ROLE_SYSADMIN;
}

bool session_is_branch_manager()
{
  return session_is_active() && _session.role == USER_ROLE_BRANCH_MANAGER;
}

bool session_is_trainer()
{
  return session_is_active() && _session.role == USER_ROLE_TRAINER;
}

bool session_is_member()
{
  return session_is_active() && _session.role == USER_ROLE_MEMBER;
}

bool session_belongs_to_branch(const char branch_name[])
{
  if (!session_is_active()) return false;

  if (is_blank_string(branch_name)) return false;

  if (_session.role == USER_ROLE_SYSADMIN) return true;

  return strcmp(_session.branch_name, branch_name) == 0;
}

user_role_t get_role_from_session()
{
  return _session.role;
}

id_t get_user_id_from_session()
{
  return _session.user_id;
}

const char *get_username_from_session()
{
  return _session.username;
}

const char *get_branch_name_from_session()
{
  return _session.branch_name;
}

// ===================== user.c =====================

bool username_exists(const char username[]);
int branch_manager_count(const char branch_name[]);
int branch_trainer_count(const char branch_name[]);
int branch_member_count(const char branch_name[]);

#define SEP FIELD_DELIMITER_STRING

static sysadmin_t sysadmins[MAX_SYSTEM_ADMINS];
static int sysadmin_count;
static id_t next_sysadmin_id;

static branch_staff_t branch_staffs[MAX_BRANCH_MANAGERS + MAX_TRAINERS];
static int branch_staff_count;
static id_t next_branch_staff_id;

static gym_member_t gym_members[MAX_GYM_MEMBERS];
static int gym_member_count;
static id_t next_gym_member_id;

static inline void format_sysadmin_line(const sysadmin_t record_payload, char *destination)
{
  snprintf(
    destination, LINE_BUFFER_SIZE, "%lu" SEP "%s" SEP "%s", (unsigned long)record_payload.id, record_payload.username,
    record_payload.password_hash
  );
}

static bool persist_sysadmin(const sysadmin_t record_payload)
{
  FILE *file = fopen(SYSADMINS_FILE_PATH, "a");
  if (file == NULL)
  {
    printf("Error: Failed to open sysadmin data file.");
    return false;
  }

  char line[LINE_BUFFER_SIZE];
  format_sysadmin_line(record_payload, line);

  bool success = write_line_to_file(file, line);
  fclose(file);
  return success;
}

static inline void format_branch_staff_line(const branch_staff_t record_payload, char *destination)
{
  snprintf(
    destination, LINE_BUFFER_SIZE, "%lu" SEP "%s" SEP "%s" SEP "%s" SEP "%s" SEP "%s" SEP "%s" SEP "%lld" SEP "%d",
    (unsigned long)record_payload.id, record_payload.full_name, record_payload.email, record_payload.phone_number,
    record_payload.gym_branch, record_payload.username, record_payload.password_hash,
    datetime_to_seconds(record_payload.joined_at), (int)record_payload.role
  );
}

static inline void format_gym_member_line(const gym_member_t record_payload, char *destination)
{
  snprintf(
    destination, LINE_BUFFER_SIZE,
    "%lu" SEP "%s" SEP "%s" SEP "%s" SEP "%s" SEP "%s" SEP "%s" SEP "%lld" SEP "%lld" SEP "%u" SEP "%u" SEP "%u" SEP
    "%d",
    (unsigned long)record_payload.id, record_payload.full_name, record_payload.email, record_payload.phone_number,
    record_payload.gym_branch, record_payload.username, record_payload.password_hash,
    datetime_to_seconds(record_payload.joined_at), datetime_to_seconds(record_payload.last_payment_date),
    record_payload.due_amount, record_payload.plan.payable_amount, record_payload.plan.interval_days,
    (int)record_payload.status
  );
}

static bool persist_branch_staff(const branch_staff_t record_payload)
{
  FILE *file = fopen(BRANCH_STAFF_FILE_PATH, "a");
  if (file == NULL)
  {
    printf("Error: Failed to open branch staff data file.");
    return false;
  }

  char line[LINE_BUFFER_SIZE];
  format_branch_staff_line(record_payload, line);

  bool success = write_line_to_file(file, line);
  fclose(file);
  return success;
}

static bool persist_gym_member(const gym_member_t record_payload)
{
  FILE *file = fopen(GYM_MEMBERS_FILE_PATH, "a");
  if (file == NULL)
  {
    printf("Error: Failed to open gym member data file.");
    return false;
  }

  char line[LINE_BUFFER_SIZE];
  format_gym_member_line(record_payload, line);

  bool success = write_line_to_file(file, line);
  fclose(file);
  return success;
}

static inline void remove_branch_staff_at(int index)
{
  for (int i = index; i < branch_staff_count - 1; i++) branch_staffs[i] = branch_staffs[i + 1];

  branch_staff_count--;
}

static inline void remove_gym_member_at(int index)
{
  for (int i = index; i < gym_member_count - 1; i++) gym_members[i] = gym_members[i + 1];

  gym_member_count--;
}

static bool rewrite_all_branch_staff_to_file_without_index(int index)
{
  FILE *file = fopen(BRANCH_STAFF_FILE_PATH, "w");
  if (file == NULL)
  {
    printf("Error: Failed to open branch staff file for writing.");
    return false;
  }

  char line[LINE_BUFFER_SIZE];
  for (int i = 0; i < branch_staff_count; i++)
  {
    if (i == index) continue;

    format_branch_staff_line(branch_staffs[i], line);
    if (!write_line_to_file(file, line))
    {
      fclose(file);
      printf("Error: Failed to write branch staff record.");
      return false;
    }
  }

  fclose(file);
  return true;
}

static bool rewrite_all_gym_members_to_file_without_index(int index)
{
  FILE *file = fopen(GYM_MEMBERS_FILE_PATH, "w");
  if (file == NULL)
  {
    printf("Error: Failed to open gym members file for writing.");
    return false;
  }

  char line[LINE_BUFFER_SIZE];
  for (int i = 0; i < gym_member_count; i++)
  {
    if (i == index) continue;

    format_gym_member_line(gym_members[i], line);
    if (!write_line_to_file(file, line))
    {
      fclose(file);
      printf("Error: Failed to write gym member record.");
      return false;
    }
  }

  fclose(file);
  return true;
}

static bool rewrite_all_branch_staffs_to_file()
{
  FILE *file = fopen(BRANCH_STAFF_FILE_PATH, "w");
  if (file == NULL)
  {
    printf("Error: Failed to open branch staff file for writing.");
    return false;
  }

  char line[LINE_BUFFER_SIZE];
  for (int i = 0; i < branch_staff_count; i++)
  {
    format_branch_staff_line(branch_staffs[i], line);
    if (!write_line_to_file(file, line))
    {
      fclose(file);
      printf("Error: Failed to write branch staff record.");
      return false;
    }
  }

  fclose(file);
  return true;
}

static bool rewrite_all_gym_members_to_file()
{
  FILE *file = fopen(GYM_MEMBERS_FILE_PATH, "w");
  if (file == NULL)
  {
    printf("Error: Failed to open gym members file for writing.");
    return false;
  }

  char line[LINE_BUFFER_SIZE];
  for (int i = 0; i < gym_member_count; i++)
  {
    format_gym_member_line(gym_members[i], line);
    if (!write_line_to_file(file, line))
    {
      fclose(file);
      printf("Error: Failed to write gym member record.");
      return false;
    }
  }

  fclose(file);
  return true;
}

static inline int split_record_line(const char line[], char destination[][FIELD_BUFFER_SIZE])
{
  return split(line, FIELD_DELIMITER, destination, MAX_RECORD_FIELDS);
}

static inline bool parse_sysadmin_line(const char line[], sysadmin_t *destination)
{
  char parts[MAX_RECORD_FIELDS][FIELD_BUFFER_SIZE];
  int count = split_record_line(line, parts);
  if (count != 3) return false;

  destination->id = string_to_unsigned_long_int(parts[0]);
  strcpy(destination->username, parts[1]);
  strcpy(destination->password_hash, parts[2]);
  return true;
}

static inline bool parse_branch_staff_line(const char line[], branch_staff_t *destination)
{
  char parts[MAX_RECORD_FIELDS][FIELD_BUFFER_SIZE];
  int count = split_record_line(line, parts);
  if (count != 9) return false;

  destination->id = string_to_unsigned_long_int(parts[0]);
  strcpy(destination->full_name, parts[1]);
  strcpy(destination->email, parts[2]);
  strcpy(destination->phone_number, parts[3]);
  strcpy(destination->gym_branch, parts[4]);
  strcpy(destination->username, parts[5]);
  strcpy(destination->password_hash, parts[6]);
  destination->joined_at = datetime_from_seconds((long long)string_to_unsigned_long_int(parts[7]));
  destination->role = (staff_role_t)string_to_unsigned_int(parts[8]);
  return true;
}

static inline bool parse_gym_member_line(const char line[], gym_member_t *destination)
{
  char parts[MAX_RECORD_FIELDS][FIELD_BUFFER_SIZE];
  int count = split_record_line(line, parts);
  if (count != 13) return false;

  destination->id = string_to_unsigned_long_int(parts[0]);
  strcpy(destination->full_name, parts[1]);
  strcpy(destination->email, parts[2]);
  strcpy(destination->phone_number, parts[3]);
  strcpy(destination->gym_branch, parts[4]);
  strcpy(destination->username, parts[5]);
  strcpy(destination->password_hash, parts[6]);
  destination->joined_at = datetime_from_seconds((long long)string_to_unsigned_long_int(parts[7]));
  destination->last_payment_date = datetime_from_seconds((long long)string_to_unsigned_long_int(parts[8]));
  destination->due_amount = string_to_unsigned_int(parts[9]);
  destination->plan.payable_amount = string_to_unsigned_int(parts[10]);
  destination->plan.interval_days = string_to_unsigned_int(parts[11]);
  destination->status = (membership_status_t)string_to_unsigned_int(parts[12]);
  return true;
}

int load_sysadmins()
{
  sysadmin_count = 0;

  FILE *file = fopen(SYSADMINS_FILE_PATH, "r");
  if (file == NULL)
  {
    next_sysadmin_id = 1;
    return 0;
  }

  char line[LINE_BUFFER_SIZE];
  while (sysadmin_count < MAX_SYSTEM_ADMINS && read_line_from_file(file, line, LINE_BUFFER_SIZE))
  {
    if (!is_blank_string(line) && parse_sysadmin_line(line, &sysadmins[sysadmin_count])) sysadmin_count++;
  }

  fclose(file);
  next_sysadmin_id = sysadmin_count > 0 ? sysadmins[sysadmin_count - 1].id + 1 : 1;
  return sysadmin_count;
}

int load_branch_staff()
{
  branch_staff_count = 0;

  FILE *file = fopen(BRANCH_STAFF_FILE_PATH, "r");
  if (file == NULL)
  {
    next_branch_staff_id = 1;
    return 0;
  }

  char line[LINE_BUFFER_SIZE];
  int capacity = MAX_BRANCH_MANAGERS + MAX_TRAINERS;
  while (branch_staff_count < capacity && read_line_from_file(file, line, LINE_BUFFER_SIZE))
  {
    if (!is_blank_string(line) && parse_branch_staff_line(line, &branch_staffs[branch_staff_count]))
      branch_staff_count++;
  }

  fclose(file);
  next_branch_staff_id = branch_staff_count > 0 ? branch_staffs[branch_staff_count - 1].id + 1 : 1;
  return branch_staff_count;
}

int load_gym_members()
{
  gym_member_count = 0;

  FILE *file = fopen(GYM_MEMBERS_FILE_PATH, "r");
  if (file == NULL)
  {
    next_gym_member_id = 1;
    return 0;
  }

  char line[LINE_BUFFER_SIZE];
  while (gym_member_count < MAX_GYM_MEMBERS && read_line_from_file(file, line, LINE_BUFFER_SIZE))
  {
    if (!is_blank_string(line) && parse_gym_member_line(line, &gym_members[gym_member_count])) gym_member_count++;
  }

  fclose(file);
  next_gym_member_id = gym_member_count > 0 ? gym_members[gym_member_count - 1].id + 1 : 1;
  return gym_member_count;
}

id_t create_sysadmin(const char username[], const char password_hash[])
{
  if (is_blank_string(username))
  {
    printf("Error: Username cannot be empty.");
    return 0;
  }

  if (is_blank_string(password_hash))
  {
    printf("Error: Password hash cannot be empty.");
    return 0;
  }

  if (sysadmin_count >= MAX_SYSTEM_ADMINS)
  {
    printf("Error: Maximum sysadmin count reached.");
    return 0;
  }

  if (username_exists(username))
  {
    printf("Error: Username '%s' already exists.", username);
    return 0;
  }

  sysadmin_t record;
  record.id = next_sysadmin_id++;
  strcpy(record.username, username);
  strcpy(record.password_hash, password_hash);

  if (!persist_sysadmin(record))
  {
    printf("Error: Failed to persist sysadmin record.");
    return 0;
  }

  sysadmins[sysadmin_count] = record;
  sysadmin_count++;
  return record.id;
}

id_t create_branch_staff(
  const char full_name[],
  const char email[],
  const char phone_number[],
  const char gym_branch[],
  const char username[],
  const char password_hash[],
  staff_role_t role
)
{
  if (is_blank_string(full_name))
  {
    printf("Error: Full name cannot be empty.");
    return 0;
  }

  if (is_blank_string(email))
  {
    printf("Error: Email cannot be empty.");
    return 0;
  }

  if (is_blank_string(phone_number))
  {
    printf("Error: Phone number cannot be empty.");
    return 0;
  }

  if (is_blank_string(gym_branch))
  {
    printf("Error: Branch name cannot be empty.");
    return 0;
  }

  if (is_blank_string(username))
  {
    printf("Error: Username cannot be empty.");
    return 0;
  }

  if (is_blank_string(password_hash))
  {
    printf("Error: Password hash cannot be empty.");
    return 0;
  }

  if (username_exists(username))
  {
    printf("Error: Username '%s' already exists.", username);
    return 0;
  }

  int capacity = MAX_BRANCH_MANAGERS + MAX_TRAINERS;
  if (branch_staff_count >= capacity)
  {
    printf("Error: Maximum branch staff count reached.");
    return 0;
  }

  branch_staff_t record;
  record.id = next_branch_staff_id++;

  strcpy(record.full_name, full_name);
  strcpy(record.email, email);
  strcpy(record.phone_number, phone_number);
  strcpy(record.gym_branch, gym_branch);
  strcpy(record.username, username);
  strcpy(record.password_hash, password_hash);
  record.joined_at = now_datetime();
  record.role = role;

  if (!persist_branch_staff(record))
  {
    printf("Error: Failed to persist branch staff record.");
    return 0;
  }

  branch_staffs[branch_staff_count] = record;
  branch_staff_count++;
  return record.id;
}

id_t create_gym_member(
  const char full_name[],
  const char email[],
  const char phone_number[],
  const char gym_branch[],
  const char username[],
  const char password_hash[],
  subscription_plan_t plan_payload,
  membership_status_t status
)
{
  if (is_blank_string(full_name))
  {
    printf("Error: Full name cannot be empty.");
    return 0;
  }

  if (is_blank_string(email))
  {
    printf("Error: Email cannot be empty.");
    return 0;
  }

  if (is_blank_string(phone_number))
  {
    printf("Error: Phone number cannot be empty.");
    return 0;
  }

  if (is_blank_string(gym_branch))
  {
    printf("Error: Branch name cannot be empty.");
    return 0;
  }

  if (is_blank_string(username))
  {
    printf("Error: Username cannot be empty.");
    return 0;
  }

  if (is_blank_string(password_hash))
  {
    printf("Error: Password hash cannot be empty.");
    return 0;
  }

  if (username_exists(username))
  {
    printf("Error: Username '%s' already exists.", username);
    return 0;
  }

  if (gym_member_count >= MAX_GYM_MEMBERS)
  {
    printf("Error: Maximum gym member count reached.");
    return 0;
  }

  gym_member_t record;
  record.id = next_gym_member_id++;

  strcpy(record.full_name, full_name);
  strcpy(record.email, email);
  strcpy(record.phone_number, phone_number);
  strcpy(record.gym_branch, gym_branch);
  strcpy(record.username, username);
  strcpy(record.password_hash, password_hash);
  record.joined_at = now_datetime();
  record.last_payment_date = EMPTY_DATETIME;
  record.due_amount = 0;
  record.plan = plan_payload;
  record.status = status;

  if (!persist_gym_member(record))
  {
    printf("Error: Failed to persist gym member record.");
    return 0;
  }

  gym_members[gym_member_count] = record;
  gym_member_count++;
  return record.id;
}

bool delete_branch_staff(id_t id)
{
  int index = -1;
  for (int i = 0; i < branch_staff_count; i++)
  {
    if (branch_staffs[i].id == id)
    {
      index = i;
      break;
    }
  }

  if (index < 0)
  {
    printf("Error: No staff member found with id %lu.", (unsigned long)id);
    return false;
  }

  if (!rewrite_all_branch_staff_to_file_without_index(index))
  {
    printf("Error: Failed to rewrite branch staff file.");
    return false;
  }

  remove_branch_staff_at(index);
  return true;
}

bool delete_gym_member(id_t id)
{
  int index = -1;
  for (int i = 0; i < gym_member_count; i++)
  {
    if (gym_members[i].id == id)
    {
      index = i;
      break;
    }
  }

  if (index < 0)
  {
    printf("Error: No gym member found with id %lu.", (unsigned long)id);
    return false;
  }

  if (gym_members[index].due_amount != 0)
  {
    printf("Error: Member with id %lu has outstanding dues.", (unsigned long)id);
    return false;
  }

  if (!rewrite_all_gym_members_to_file_without_index(index))
  {
    printf("Error: Failed to rewrite gym members file.");
    return false;
  }

  remove_gym_member_at(index);
  return true;
}

bool username_exists(const char username[])
{
  if (is_blank_string(username)) return false;

  for (int i = 0; i < sysadmin_count; i++)
  {
    if (strcmp(sysadmins[i].username, username) == 0) return true;
  }

  for (int i = 0; i < branch_staff_count; i++)
  {
    if (strcmp(branch_staffs[i].username, username) == 0) return true;
  }

  for (int i = 0; i < gym_member_count; i++)
  {
    if (strcmp(gym_members[i].username, username) == 0) return true;
  }

  return false;
}

int branch_manager_count(const char branch_name[])
{
  if (is_blank_string(branch_name)) return 0;

  int count = 0;
  for (int i = 0; i < branch_staff_count; i++)
  {
    if (branch_staffs[i].role == BRANCH_MANAGER && strcmp(branch_staffs[i].gym_branch, branch_name) == 0) count++;
  }
  return count;
}

int branch_trainer_count(const char branch_name[])
{
  if (is_blank_string(branch_name)) return 0;

  int count = 0;
  for (int i = 0; i < branch_staff_count; i++)
  {
    if (branch_staffs[i].role == TRAINER && strcmp(branch_staffs[i].gym_branch, branch_name) == 0) count++;
  }
  return count;
}

int branch_member_count(const char branch_name[])
{
  if (is_blank_string(branch_name)) return 0;

  int count = 0;
  for (int i = 0; i < gym_member_count; i++)
  {
    if (strcmp(gym_members[i].gym_branch, branch_name) == 0) count++;
  }
  return count;
}

bool get_sysadmin_by_id(id_t id, sysadmin_t *destination)
{
  if (destination == NULL) return false;

  for (int i = 0; i < sysadmin_count; i++)
  {
    if (sysadmins[i].id == id)
    {
      *destination = sysadmins[i];
      return true;
    }
  }
  return false;
}

bool get_sysadmin_by_username(const char username[], sysadmin_t *destination)
{
  if (is_blank_string(username) || destination == NULL) return false;

  for (int i = 0; i < sysadmin_count; i++)
  {
    if (strcmp(sysadmins[i].username, username) == 0)
    {
      *destination = sysadmins[i];
      return true;
    }
  }
  return false;
}

bool get_branch_staff_by_id(id_t id, branch_staff_t *destination)
{
  if (destination == NULL) return false;

  for (int i = 0; i < branch_staff_count; i++)
  {
    if (branch_staffs[i].id == id)
    {
      *destination = branch_staffs[i];
      return true;
    }
  }
  return false;
}

bool get_branch_staff_by_username(const char username[], branch_staff_t *destination)
{
  if (is_blank_string(username) || destination == NULL) return false;

  for (int i = 0; i < branch_staff_count; i++)
  {
    if (strcmp(branch_staffs[i].username, username) == 0)
    {
      *destination = branch_staffs[i];
      return true;
    }
  }
  return false;
}

bool get_gym_member_by_id(id_t id, gym_member_t *destination)
{
  if (destination == NULL) return false;

  for (int i = 0; i < gym_member_count; i++)
  {
    if (gym_members[i].id == id)
    {
      *destination = gym_members[i];
      return true;
    }
  }
  return false;
}

bool get_gym_member_by_username(const char username[], gym_member_t *destination)
{
  if (is_blank_string(username) || destination == NULL) return false;

  for (int i = 0; i < gym_member_count; i++)
  {
    if (strcmp(gym_members[i].username, username) == 0)
    {
      *destination = gym_members[i];
      return true;
    }
  }
  return false;
}

bool update_branch_staff(id_t id, const char full_name[], const char email[], const char phone_number[])
{
  if (is_blank_string(full_name))
  {
    printf("Error: Full name cannot be empty.");
    return false;
  }

  if (is_blank_string(email))
  {
    printf("Error: Email cannot be empty.");
    return false;
  }

  if (is_blank_string(phone_number))
  {
    printf("Error: Phone number cannot be empty.");
    return false;
  }

  int index = -1;
  for (int i = 0; i < branch_staff_count; i++)
  {
    if (branch_staffs[i].id == id)
    {
      index = i;
      break;
    }
  }

  if (index < 0)
  {
    printf("Error: No staff member found with id %lu.", (unsigned long)id);
    return false;
  }

  strcpy(branch_staffs[index].full_name, full_name);
  strcpy(branch_staffs[index].email, email);
  strcpy(branch_staffs[index].phone_number, phone_number);

  if (!rewrite_all_branch_staffs_to_file())
  {
    printf("Error: Failed to persist branch staff update.");
    return false;
  }

  return true;
}

bool update_gym_member(
  id_t id,
  const char full_name[],
  const char email[],
  const char phone_number[],
  const char gym_branch[],
  const char username[]
)
{
  if (is_blank_string(full_name))
  {
    printf("Error: Full name cannot be empty.");
    return false;
  }

  if (is_blank_string(email))
  {
    printf("Error: Email cannot be empty.");
    return false;
  }

  if (is_blank_string(phone_number))
  {
    printf("Error: Phone number cannot be empty.");
    return false;
  }

  if (is_blank_string(gym_branch))
  {
    printf("Error: Branch name cannot be empty.");
    return false;
  }

  if (is_blank_string(username))
  {
    printf("Error: Username cannot be empty.");
    return false;
  }

  int index = -1;
  for (int i = 0; i < gym_member_count; i++)
  {
    if (gym_members[i].id == id)
    {
      index = i;
      break;
    }
  }

  if (index < 0)
  {
    printf("Error: No gym member found with id %lu.", (unsigned long)id);
    return false;
  }

  if (strcmp(gym_members[index].username, username) != 0 && username_exists(username))
  {
    printf("Error: Username '%s' already exists.", username);
    return false;
  }

  strcpy(gym_members[index].full_name, full_name);
  strcpy(gym_members[index].email, email);
  strcpy(gym_members[index].phone_number, phone_number);
  strcpy(gym_members[index].gym_branch, gym_branch);
  strcpy(gym_members[index].username, username);

  if (!rewrite_all_gym_members_to_file())
  {
    printf("Error: Failed to persist gym member update.");
    return false;
  }

  return true;
}

bool update_gym_member_status(id_t id, membership_status_t status)
{
  int index = -1;
  for (int i = 0; i < gym_member_count; i++)
  {
    if (gym_members[i].id == id)
    {
      index = i;
      break;
    }
  }

  if (index < 0)
  {
    printf("Error: No gym member found with id %lu.", (unsigned long)id);
    return false;
  }

  gym_members[index].status = status;

  if (!rewrite_all_gym_members_to_file())
  {
    printf("Error: Failed to persist gym member status update.");
    return false;
  }

  return true;
}

bool update_gym_member_billing(id_t id, const datetime_t last_payment_date_payload, unsigned int paid_amount)
{
  int index = -1;
  for (int i = 0; i < gym_member_count; i++)
  {
    if (gym_members[i].id == id)
    {
      index = i;
      break;
    }
  }

  if (index < 0)
  {
    printf("Error: No gym member found with id %lu.", (unsigned long)id);
    return false;
  }

  gym_members[index].last_payment_date = last_payment_date_payload;

  if (gym_members[index].due_amount > paid_amount)
    gym_members[index].due_amount -= paid_amount;
  else
    gym_members[index].due_amount = 0;

  if (!rewrite_all_gym_members_to_file())
  {
    printf("Error: Failed to persist gym member billing update.");
    return false;
  }

  return true;
}

bool update_gym_member_lifecycle(
  id_t id,
  subscription_plan_t plan_payload,
  const datetime_t last_payment_date_payload,
  unsigned int due_amount,
  membership_status_t status
)
{
  int index = -1;
  for (int i = 0; i < gym_member_count; i++)
  {
    if (gym_members[i].id == id)
    {
      index = i;
      break;
    }
  }

  if (index < 0)
  {
    printf("Error: No gym member found with id %lu.", (unsigned long)id);
    return false;
  }

  gym_members[index].plan = plan_payload;
  gym_members[index].last_payment_date = last_payment_date_payload;
  gym_members[index].due_amount = due_amount;
  gym_members[index].status = status;

  if (!rewrite_all_gym_members_to_file())
  {
    printf("Error: Failed to persist gym member lifecycle update.");
    return false;
  }

  return true;
}

static bool rename_branch_for_branch_staffs(const char old_branch_name[], const char new_branch_name[])
{
  bool renamed = false;
  for (int i = 0; i < branch_staff_count; i++)
  {
    if (strcmp(branch_staffs[i].gym_branch, old_branch_name) == 0)
    {
      strcpy(branch_staffs[i].gym_branch, new_branch_name);
      renamed = true;
    }
  }

  if (!renamed) return true;

  if (!rewrite_all_branch_staffs_to_file())
  {
    printf("Error: Failed to persist staff branch rename.");
    return false;
  }

  return true;
}

static bool rename_branch_for_gym_members(const char old_branch_name[], const char new_branch_name[])
{
  bool renamed = false;
  for (int i = 0; i < gym_member_count; i++)
  {
    if (strcmp(gym_members[i].gym_branch, old_branch_name) == 0)
    {
      strcpy(gym_members[i].gym_branch, new_branch_name);
      renamed = true;
    }
  }

  if (!renamed) return true;

  if (!rewrite_all_gym_members_to_file())
  {
    printf("Error: Failed to persist member branch rename.");
    return false;
  }

  return true;
}

bool rename_branch_for_all_users(const char old_branch_name[], const char new_branch_name[])
{
  if (is_blank_string(old_branch_name))
  {
    printf("Error: Old branch name cannot be empty.");
    return false;
  }

  if (is_blank_string(new_branch_name))
  {
    printf("Error: New branch name cannot be empty.");
    return false;
  }

  if (!rename_branch_for_branch_staffs(old_branch_name, new_branch_name))
  {
    printf("Error: Failed to move branch staff records to '%s'.", new_branch_name);
    return false;
  }

  if (!rename_branch_for_gym_members(old_branch_name, new_branch_name))
  {
    printf("Error: Failed to move gym member records to '%s'.", new_branch_name);
    return false;
  }

  return true;
}

int get_branch_staff_count()
{
  return branch_staff_count;
}

bool get_branch_staff_at(int index, branch_staff_t *destination)
{
  if (destination == NULL) return false;
  if (index < 0 || index >= branch_staff_count) return false;
  *destination = branch_staffs[index];
  return true;
}

int get_gym_member_ids_by_status(membership_status_t status, id_t *destination, int destination_capacity)
{
  if (destination == NULL || destination_capacity <= 0) return 0;

  int copied_count = 0;
  for (int i = 0; i < gym_member_count && copied_count < destination_capacity; i++)
  {
    if (gym_members[i].status == status)
    {
      destination[copied_count] = gym_members[i].id;
      copied_count++;
    }
  }

  return copied_count;
}

// ===================== branch.c =====================

static char branches[BRANCH_COUNT_MAX][BRANCH_NAME_BUFFER_SIZE];
static int branch_count;

static bool save_branches_to_file()
{
  const char *line_maps[branch_count];
  for (int i = 0; i < branch_count; i++) line_maps[i] = branches[i];

  if (!write_lines_to_file(GYM_BRANCHES_FILE_PATH, line_maps, branch_count))
  {
    printf("Error: Failed to rewrite branches file.");
    return false;
  }

  return true;
}

int load_branches()
{
  char *line_maps[BRANCH_COUNT_MAX];
  for (int i = 0; i < BRANCH_COUNT_MAX; i++) line_maps[i] = branches[i];

  branch_count = read_lines_from_file(GYM_BRANCHES_FILE_PATH, line_maps, BRANCH_COUNT_MAX, BRANCH_NAME_BUFFER_SIZE);
  return branch_count;
}

int find_branch(const char branch_name[])
{
  if (is_blank_string(branch_name)) return -1;

  for (int i = 0; i < branch_count; i++)
  {
    if (strcmp(branches[i], branch_name) == 0) return i;
  }

  return -1;
}

bool add_branch(const char branch_name[])
{
  if (is_blank_string(branch_name))
  {
    printf("Error: Branch name cannot be empty.");
    return false;
  }

  if (find_branch(branch_name) != -1)
  {
    printf("Error: Branch '%s' already exists.", branch_name);
    return false;
  }

  if (branch_count >= BRANCH_COUNT_MAX)
  {
    printf("Error: Maximum branch count reached.");
    return false;
  }

  FILE *file = fopen(GYM_BRANCHES_FILE_PATH, "a");
  if (file == NULL)
  {
    printf("Error: Failed to open branches file for appending.");
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
  if (is_blank_string(branch_name)) return false;

  return branch_manager_count(branch_name) == 0 && branch_trainer_count(branch_name) == 0 &&
         branch_member_count(branch_name) == 0;
}

bool delete_branch(const char branch_name[])
{
  if (is_blank_string(branch_name))
  {
    printf("Error: Branch name cannot be empty.");
    return false;
  }

  int index = find_branch(branch_name);
  if (index < 0)
  {
    printf("Error: Branch '%s' does not exist.", branch_name);
    return false;
  }

  if (!ensure_branch_has_no_users(branch_name))
  {
    printf("Error: Branch '%s' still has assigned users.", branch_name);
    return false;
  }

  for (int i = index; i < branch_count - 1; i++) strcpy(branches[i], branches[i + 1]);

  branches[branch_count - 1][0] = '\0';
  branch_count--;

  return save_branches_to_file();
}

bool update_branch_name(const char old_branch_name[], const char new_branch_name[])
{
  if (is_blank_string(old_branch_name))
  {
    printf("Error: Old branch name cannot be empty.");
    return false;
  }

  if (is_blank_string(new_branch_name))
  {
    printf("Error: New branch name cannot be empty.");
    return false;
  }

  int index = find_branch(old_branch_name);
  if (index < 0)
  {
    printf("Error: Branch '%s' does not exist.", old_branch_name);
    return false;
  }

  if (find_branch(new_branch_name) != -1)
  {
    printf("Error: Branch '%s' already exists.", new_branch_name);
    return false;
  }

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
    printf("Error: Branch index %d is out of range.", index);
    return NULL;
  }

  return branches[index];
}

// ===================== auth.c =====================

#define MIXED_BUFFER_SIZE PASSWORD_HASH_BUFFER_SIZE

void hash_password(const char password[], char *destination)
{
  if (password == NULL || destination == NULL) return;

  char salt[SALT_BUFFER_SIZE];
  generate_salt(salt);

  char mixed[MIXED_BUFFER_SIZE];
  mix_salt(password, salt, mixed);

  hash_t hash_value = create_hash(mixed);

  char hash_str[HASH_STRING_BUFFER_SIZE];
  hash_value_to_string(hash_value, hash_str);

  int write_index = 0;

  for (int i = 0; i < SALT_BUFFER_SIZE - 1 && salt[i] != '\0'; i++) destination[write_index++] = salt[i];

  for (int i = 0; hash_str[i] != '\0'; i++) destination[write_index++] = hash_str[i];

  destination[write_index] = '\0';
}

bool verify_password(const char password[], const char stored_hash[])
{
  if (password == NULL || stored_hash == NULL) return false;

  char salt[SALT_BUFFER_SIZE];
  int i = 0;
  for (; i < SALT_BUFFER_SIZE - 1 && stored_hash[i] != '\0'; i++) salt[i] = stored_hash[i];
  salt[i] = '\0';

  char mixed[MIXED_BUFFER_SIZE];
  mix_salt(password, salt, mixed);

  hash_t computed = create_hash(mixed);
  hash_t stored = parse_hash_value(stored_hash + SALT_BUFFER_SIZE - 1);

  return compare_hash(stored, computed);
}

bool auth_login(const char username[], const char password[], user_role_t *destination)
{
  if (username == NULL || password == NULL || destination == NULL) return false;

  sysadmin_t sysadmin;
  if (get_sysadmin_by_username(username, &sysadmin))
  {
    if (!verify_password(password, sysadmin.password_hash)) return false;

    *destination = USER_ROLE_SYSADMIN;
    set_session_context(USER_ROLE_SYSADMIN, sysadmin.id, sysadmin.username, "");
    return true;
  }

  branch_staff_t staff;
  if (get_branch_staff_by_username(username, &staff))
  {
    if (!verify_password(password, staff.password_hash)) return false;

    switch (staff.role)
    {
    case BRANCH_MANAGER:
      *destination = USER_ROLE_BRANCH_MANAGER;
      break;
    default:
      *destination = USER_ROLE_TRAINER;
      break;
    }

    set_session_context(*destination, staff.id, staff.username, staff.gym_branch);
    return true;
  }

  gym_member_t member;
  if (get_gym_member_by_username(username, &member))
  {
    if (!verify_password(password, member.password_hash)) return false;

    *destination = USER_ROLE_MEMBER;
    set_session_context(USER_ROLE_MEMBER, member.id, member.username, member.gym_branch);
    return true;
  }

  return false;
}

void auth_logout()
{
  clear_session_context();
}

// ===================== member.c =====================

static suspension_record_t suspension_records[MAX_SUSPENSION_RECORDS];
static int suspension_count;
static id_t next_suspension_id;

static inline void format_suspension_line(const suspension_record_t record_payload, char *destination)
{
  snprintf(
    destination, LINE_BUFFER_SIZE, "%lu" SEP "%lu" SEP "%s" SEP "%lld" SEP "%lld", (unsigned long)record_payload.id,
    (unsigned long)record_payload.gym_member_id, record_payload.reason,
    datetime_to_seconds(record_payload.suspension_date), datetime_to_seconds(record_payload.unsuspension_date)
  );
}

static bool persist_suspension(const suspension_record_t record_payload)
{
  FILE *file = fopen(SUSPENSIONS_FILE_PATH, "a");
  if (file == NULL)
  {
    printf("Error: Failed to open suspensions data file.");
    return false;
  }

  char line[LINE_BUFFER_SIZE];
  format_suspension_line(record_payload, line);

  bool success = write_line_to_file(file, line);
  fclose(file);
  return success;
}

static bool rewrite_all_suspension_to_file()
{
  FILE *file = fopen(SUSPENSIONS_FILE_PATH, "w");
  if (file == NULL)
  {
    printf("Error: Failed to open suspensions file for writing.");
    return false;
  }

  char line[LINE_BUFFER_SIZE];
  for (int i = 0; i < suspension_count; i++)
  {
    format_suspension_line(suspension_records[i], line);
    if (!write_line_to_file(file, line))
    {
      fclose(file);
      printf("Error: Failed to write suspension record.");
      return false;
    }
  }

  fclose(file);
  return true;
}

static inline bool parse_suspension_line(const char line[], suspension_record_t *destination)
{
  char parts[MAX_RECORD_FIELDS][FIELD_BUFFER_SIZE];

  int count = split(line, FIELD_DELIMITER, parts, MAX_RECORD_FIELDS);
  if (count != 5) return false;

  destination->id = string_to_unsigned_long_int(parts[0]);
  destination->gym_member_id = string_to_unsigned_long_int(parts[1]);
  strcpy(destination->reason, parts[2]);
  destination->suspension_date = datetime_from_seconds((long long)string_to_unsigned_long_int(parts[3]));
  destination->unsuspension_date = datetime_from_seconds((long long)string_to_unsigned_long_int(parts[4]));
  return true;
}

int load_suspensions()
{
  suspension_count = 0;

  FILE *file = fopen(SUSPENSIONS_FILE_PATH, "r");
  if (file == NULL)
  {
    next_suspension_id = 1;
    return 0;
  }

  char line[LINE_BUFFER_SIZE];
  while (suspension_count < MAX_SUSPENSION_RECORDS && read_line_from_file(file, line, LINE_BUFFER_SIZE))
  {
    if (!is_blank_string(line) && parse_suspension_line(line, &suspension_records[suspension_count]))
      suspension_count++;
  }

  fclose(file);
  next_suspension_id = suspension_count > 0 ? suspension_records[suspension_count - 1].id + 1 : 1;
  return suspension_count;
}

bool approve_gym_member(id_t member_id)
{
  gym_member_t member;
  if (!get_gym_member_by_id(member_id, &member))
  {
    printf("Error: No gym member found with id %lu.", (unsigned long)member_id);
    return false;
  }

  if (member.status != MEMBERSHIP_ON_HOLD)
  {
    printf("Error: Member with id %lu is not awaiting approval.", (unsigned long)member_id);
    return false;
  }

  subscription_plan_t default_plan = {
    .payable_amount = DEFAULT_PLAN_AMOUNT, .interval_days = DEFAULT_PLAN_INTERVAL_DAYS
  };
  datetime_t approval_date = now_datetime();

  if (!update_gym_member_lifecycle(
        member_id, default_plan, approval_date, default_plan.payable_amount, MEMBERSHIP_ACTIVE
      ))
  {
    printf("Error: Failed to persist approval of member %lu.", (unsigned long)member_id);
    return false;
  }

  return true;
}

bool suspend_gym_member(id_t member_id, const char reason[])
{
  if (is_blank_string(reason))
  {
    printf("Error: Suspension reason cannot be empty.");
    return false;
  }

  if (strlen(reason) >= REASON_BUFFER_SIZE)
  {
    printf("Error: Suspension reason exceeds %d characters.", REASON_BUFFER_SIZE - 1);
    return false;
  }

  gym_member_t member;
  if (!get_gym_member_by_id(member_id, &member))
  {
    printf("Error: No gym member found with id %lu.", (unsigned long)member_id);
    return false;
  }

  if (member.status != MEMBERSHIP_ACTIVE)
  {
    printf("Error: Member with id %lu is not active and cannot be suspended.", (unsigned long)member_id);
    return false;
  }

  if (suspension_count >= MAX_SUSPENSION_RECORDS)
  {
    printf("Error: Maximum suspension record count reached.");
    return false;
  }

  datetime_t suspension_date = now_datetime();

  suspension_record_t record;
  record.id = next_suspension_id++;
  record.gym_member_id = member.id;
  strcpy(record.reason, reason);
  record.suspension_date = suspension_date;
  record.unsuspension_date = EMPTY_DATETIME;

  if (!persist_suspension(record))
  {
    printf("Error: Failed to persist suspension record.");
    return false;
  }

  suspension_records[suspension_count] = record;
  suspension_count++;

  if (!update_gym_member_status(member_id, MEMBERSHIP_SUSPENDED))
  {
    printf("Error: Failed to persist suspension of member %lu.", (unsigned long)member_id);
    return false;
  }

  return true;
}

bool unsuspend_gym_member(id_t member_id)
{
  gym_member_t member;
  if (!get_gym_member_by_id(member_id, &member))
  {
    printf("Error: No gym member found with id %lu.", (unsigned long)member_id);
    return false;
  }

  if (member.status != MEMBERSHIP_SUSPENDED)
  {
    printf("Error: Member with id %lu is not suspended.", (unsigned long)member_id);
    return false;
  }

  if (member.due_amount != 0)
  {
    printf("Error: Member with id %lu still owes dues and cannot be unsuspended.", (unsigned long)member_id);
    return false;
  }

  int open_index = -1;
  for (int i = 0; i < suspension_count; i++)
  {
    if (suspension_records[i].gym_member_id == member_id && is_empty_datetime(suspension_records[i].unsuspension_date))
    {
      open_index = i;
      break;
    }
  }

  if (open_index < 0)
  {
    printf("Error: No open suspension record found for member %lu.", (unsigned long)member_id);
    return false;
  }

  datetime_t unsuspension_date = now_datetime();

  datetime_t previous_unsuspension_date = suspension_records[open_index].unsuspension_date;
  suspension_records[open_index].unsuspension_date = unsuspension_date;

  if (!rewrite_all_suspension_to_file())
  {
    suspension_records[open_index].unsuspension_date = previous_unsuspension_date;
    printf("Error: Failed to persist suspension record update.");
    return false;
  }

  if (!update_gym_member_status(member_id, MEMBERSHIP_ACTIVE))
  {
    printf("Error: Failed to persist unsuspension of member %lu.", (unsigned long)member_id);
    return false;
  }

  return true;
}

int auto_suspend_overdue_members()
{
  datetime_t today = now_datetime();

  id_t active_ids[MAX_GYM_MEMBERS];
  int active_count = get_gym_member_ids_by_status(MEMBERSHIP_ACTIVE, active_ids, MAX_GYM_MEMBERS);

  int suspended_count = 0;
  for (int i = 0; i < active_count; i++)
  {
    gym_member_t member;
    if (!get_gym_member_by_id(active_ids[i], &member)) continue;

    datetime_t due_date = add_days(member.last_payment_date, (int)member.plan.interval_days);
    if (days_between(due_date, today) < MAX_UNPAID_DAYS) continue;

    if (!suspend_gym_member(active_ids[i], AUTO_SUSPENSION_REASON))
    {
      printf("Error: Failed to auto-suspend overdue member %lu.", (unsigned long)active_ids[i]);
      continue;
    }

    suspended_count++;
  }

  return suspended_count;
}

int get_suspensions_for_member(id_t gym_member_id, suspension_record_t *destination, int destination_capacity)
{
  if (destination == NULL || destination_capacity <= 0) return 0;

  int copied_count = 0;
  for (int i = 0; i < suspension_count && copied_count < destination_capacity; i++)
  {
    if (suspension_records[i].gym_member_id == gym_member_id)
    {
      destination[copied_count] = suspension_records[i];
      copied_count++;
    }
  }

  return copied_count;
}

// ===================== payment.c =====================

static payment_t payments[MAX_PAYMENT_RECORDS];
static int payment_count;
static id_t next_payment_id;

static inline void format_payment_line(const payment_t record_payload, char *destination)
{
  snprintf(
    destination, LINE_BUFFER_SIZE, "%lu" SEP "%lu" SEP "%u" SEP "%lld" SEP "%d" SEP "%s" SEP "%d",
    (unsigned long)record_payload.id, (unsigned long)record_payload.gym_member_id, record_payload.amount,
    datetime_to_seconds(record_payload.transaction_time), (int)record_payload.transaction_type,
    record_payload.transaction_id, (int)record_payload.status
  );
}

static inline bool parse_payment_line(const char line[], payment_t *destination)
{
  char parts[MAX_RECORD_FIELDS][FIELD_BUFFER_SIZE];

  int count = split(line, FIELD_DELIMITER, parts, MAX_RECORD_FIELDS);
  if (count != 7) return false;

  destination->id = string_to_unsigned_long_int(parts[0]);
  destination->gym_member_id = string_to_unsigned_long_int(parts[1]);
  destination->amount = string_to_unsigned_int(parts[2]);
  destination->transaction_time = datetime_from_seconds((long long)string_to_unsigned_long_int(parts[3]));
  destination->transaction_type = (transaction_t)string_to_unsigned_int(parts[4]);
  strcpy(destination->transaction_id, parts[5]);
  destination->status = (payment_status_t)string_to_unsigned_int(parts[6]);
  return true;
}

static bool persist_payment(const payment_t record_payload)
{
  FILE *file = fopen(PAYMENTS_FILE_PATH, "a");
  if (file == NULL)
  {
    printf("Error: Failed to open payments data file.");
    return false;
  }

  char line[LINE_BUFFER_SIZE];
  format_payment_line(record_payload, line);

  bool success = write_line_to_file(file, line);
  fclose(file);
  return success;
}

static inline bool ensure_member_can_pay(id_t gym_member_id)
{
  gym_member_t member;
  if (!get_gym_member_by_id(gym_member_id, &member)) return false;

  switch (member.status)
  {
  case MEMBERSHIP_ACTIVE:
  case MEMBERSHIP_SUSPENDED:
    return true;
  default:
    return false;
  }
}

static bool settle_payment(const payment_t payment_payload)
{
  if (!persist_payment(payment_payload))
  {
    printf("Error: Failed to persist payment.");
    return false;
  }

  payments[payment_count] = payment_payload;
  payment_count++;

  if (payment_payload.status != PAYMENT_COMPLETED) return true;

  if (!update_gym_member_billing(
        payment_payload.gym_member_id, payment_payload.transaction_time, payment_payload.amount
      ))
  {
    printf("Error: Failed to settle dues for member %lu.", (unsigned long)payment_payload.gym_member_id);
    return false;
  }

  return true;
}

int load_payments()
{
  payment_count = 0;

  FILE *file = fopen(PAYMENTS_FILE_PATH, "r");
  if (file == NULL)
  {
    next_payment_id = 1;
    return 0;
  }

  char line[LINE_BUFFER_SIZE];
  while (payment_count < MAX_PAYMENT_RECORDS && read_line_from_file(file, line, LINE_BUFFER_SIZE))
  {
    if (!is_blank_string(line) && parse_payment_line(line, &payments[payment_count])) payment_count++;
  }

  fclose(file);
  next_payment_id = payment_count > 0 ? payments[payment_count - 1].id + 1 : 1;
  return payment_count;
}

bool record_digital_payment(const digital_payment_request_t request_payload)
{
  if (request_payload.amount == 0)
  {
    printf("Error: Payment amount cannot be zero.");
    return false;
  }

  if (is_blank_string(request_payload.transaction_id))
  {
    printf("Error: Digital payment requires a transaction id.");
    return false;
  }

  switch (request_payload.status)
  {
  case PAYMENT_PENDING:
  case PAYMENT_COMPLETED:
  case PAYMENT_FAILED:
  case PAYMENT_INVALID:
    break;
  default:
    printf("Error: Unknown payment status %d.", (int)request_payload.status);
    return false;
  }

  if (request_payload.status == PAYMENT_COMPLETED && is_empty_datetime(request_payload.transaction_time))
  {
    printf("Error: Completed payment requires a transaction time.");
    return false;
  }

  if (!ensure_member_can_pay(request_payload.gym_member_id))
  {
    printf("Error: Member %lu cannot receive payments.", (unsigned long)request_payload.gym_member_id);
    return false;
  }

  if (payment_count >= MAX_PAYMENT_RECORDS)
  {
    printf("Error: Maximum payment record count reached.");
    return false;
  }

  payment_t payment;
  payment.id = next_payment_id++;
  payment.gym_member_id = request_payload.gym_member_id;
  payment.amount = request_payload.amount;
  payment.transaction_time = request_payload.transaction_time;
  payment.transaction_type = DIGITAL_TRANSACTION;
  strcpy(payment.transaction_id, request_payload.transaction_id);
  payment.status = request_payload.status;

  return settle_payment(payment);
}

bool record_cash_payment(id_t gym_member_id, unsigned int amount)
{
  if (amount == 0)
  {
    printf("Error: Payment amount cannot be zero.");
    return false;
  }

  if (!ensure_member_can_pay(gym_member_id))
  {
    printf("Error: Member %lu cannot receive payments.", (unsigned long)gym_member_id);
    return false;
  }

  if (payment_count >= MAX_PAYMENT_RECORDS)
  {
    printf("Error: Maximum payment record count reached.");
    return false;
  }

  payment_t payment;
  payment.id = next_payment_id++;
  payment.gym_member_id = gym_member_id;
  payment.amount = amount;
  payment.transaction_time = now_datetime();
  payment.transaction_type = CASH_TRANSACTION;
  payment.transaction_id[0] = '\0';
  payment.status = PAYMENT_COMPLETED;

  return settle_payment(payment);
}

int get_payments_for_member(id_t gym_member_id, payment_t *destination, int destination_capacity)
{
  if (destination == NULL || destination_capacity <= 0) return 0;

  int count = 0;
  for (int i = 0; i < payment_count && count < destination_capacity; i++)
  {
    if (payments[i].gym_member_id == gym_member_id)
    {
      destination[count] = payments[i];
      count++;
    }
  }

  return count;
}

// ===================== lost_found.c =====================

static lost_and_found_record_t lost_and_found_records[MAX_LOST_FOUND_RECORDS];
static int lost_and_found_count;
static id_t next_lost_and_found_id;

static inline void format_lost_and_found_line(const lost_and_found_record_t record_payload, char *destination)
{
  snprintf(
    destination, LINE_BUFFER_SIZE, "%lu" SEP "%s" SEP "%s" SEP "%s" SEP "%lld" SEP "%s",
    (unsigned long)record_payload.id, record_payload.description, record_payload.reporter_username,
    record_payload.gym_branch, datetime_to_seconds(record_payload.reported_at), record_payload.resolver_username
  );
}

static inline bool parse_lost_and_found_line(const char line[], lost_and_found_record_t *destination)
{
  char parts[MAX_RECORD_FIELDS][FIELD_BUFFER_SIZE];

  int count = split(line, FIELD_DELIMITER, parts, MAX_RECORD_FIELDS);
  if (count != 6) return false;

  destination->id = string_to_unsigned_long_int(parts[0]);
  strcpy(destination->description, parts[1]);
  strcpy(destination->reporter_username, parts[2]);
  strcpy(destination->gym_branch, parts[3]);
  destination->reported_at = datetime_from_seconds((long long)string_to_unsigned_long_int(parts[4]));
  strcpy(destination->resolver_username, parts[5]);
  return true;
}

static bool persist_lost_and_found(const lost_and_found_record_t record_payload)
{
  FILE *file = fopen(LOST_FOUND_FILE_PATH, "a");
  if (file == NULL)
  {
    printf("Error: Failed to open lost and found data file.");
    return false;
  }

  char line[LINE_BUFFER_SIZE];
  format_lost_and_found_line(record_payload, line);

  bool success = write_line_to_file(file, line);
  fclose(file);
  return success;
}

static bool rewrite_all_lost_and_found_to_file()
{
  FILE *file = fopen(LOST_FOUND_FILE_PATH, "w");
  if (file == NULL)
  {
    printf("Error: Failed to open lost and found file for writing.");
    return false;
  }

  char line[LINE_BUFFER_SIZE];
  for (int i = 0; i < lost_and_found_count; i++)
  {
    format_lost_and_found_line(lost_and_found_records[i], line);
    if (!write_line_to_file(file, line))
    {
      fclose(file);
      printf("Error: Failed to write lost and found record.");
      return false;
    }
  }

  fclose(file);
  return true;
}

static inline bool ensure_resolver_can_resolve(const char resolver_username[])
{
  sysadmin_t sysadmin;
  if (get_sysadmin_by_username(resolver_username, &sysadmin)) return true;

  branch_staff_t staff;
  if (get_branch_staff_by_username(resolver_username, &staff) && staff.role == BRANCH_MANAGER) return true;

  return false;
}

int load_lost_and_found_records()
{
  lost_and_found_count = 0;

  FILE *file = fopen(LOST_FOUND_FILE_PATH, "r");
  if (file == NULL)
  {
    next_lost_and_found_id = 1;
    return 0;
  }

  char line[LINE_BUFFER_SIZE];
  while (lost_and_found_count < MAX_LOST_FOUND_RECORDS && read_line_from_file(file, line, LINE_BUFFER_SIZE))
  {
    if (!is_blank_string(line) && parse_lost_and_found_line(line, &lost_and_found_records[lost_and_found_count]))
      lost_and_found_count++;
  }

  fclose(file);
  next_lost_and_found_id = lost_and_found_count > 0 ? lost_and_found_records[lost_and_found_count - 1].id + 1 : 1;
  return lost_and_found_count;
}

bool report_lost_item(const char reporter_username[], const char gym_branch[], const char description[])
{
  if (is_blank_string(reporter_username))
  {
    printf("Error: Reporter username cannot be empty.");
    return false;
  }

  if (is_blank_string(gym_branch))
  {
    printf("Error: Branch name cannot be empty.");
    return false;
  }

  if (is_blank_string(description))
  {
    printf("Error: Item description cannot be empty.");
    return false;
  }

  if (strlen(description) >= DESCRIPTION_BUFFER_SIZE)
  {
    printf("Error: Item description exceeds %d characters.", DESCRIPTION_BUFFER_SIZE - 1);
    return false;
  }

  if (!username_exists(reporter_username))
  {
    printf("Error: No user found with username '%s'.", reporter_username);
    return false;
  }

  if (lost_and_found_count >= MAX_LOST_FOUND_RECORDS)
  {
    printf("Error: Maximum lost and found record count reached.");
    return false;
  }

  lost_and_found_record_t record;
  record.id = next_lost_and_found_id++;
  strcpy(record.description, description);
  strcpy(record.reporter_username, reporter_username);
  strcpy(record.gym_branch, gym_branch);
  record.reported_at = now_datetime();
  record.resolver_username[0] = '\0';

  if (!persist_lost_and_found(record))
  {
    printf("Error: Failed to persist lost and found record.");
    return false;
  }

  lost_and_found_records[lost_and_found_count] = record;
  lost_and_found_count++;
  return true;
}

bool resolve_lost_item(id_t record_id, const char resolver_username[])
{
  if (is_blank_string(resolver_username))
  {
    printf("Error: Resolver username cannot be empty.");
    return false;
  }

  int record_index = -1;
  for (int i = 0; i < lost_and_found_count; i++)
  {
    if (lost_and_found_records[i].id == record_id)
    {
      record_index = i;
      break;
    }
  }

  if (record_index < 0)
  {
    printf("Error: No lost and found record found with id %lu.", (unsigned long)record_id);
    return false;
  }

  if (!is_blank_string(lost_and_found_records[record_index].resolver_username))
  {
    printf("Error: Lost and found record %lu is already resolved.", (unsigned long)record_id);
    return false;
  }

  if (!ensure_resolver_can_resolve(resolver_username))
  {
    printf("Error: User '%s' is not authorized to resolve lost and found reports.", resolver_username);
    return false;
  }

  char previous_resolver[USERNAME_BUFFER_SIZE];
  strcpy(previous_resolver, lost_and_found_records[record_index].resolver_username);
  strcpy(lost_and_found_records[record_index].resolver_username, resolver_username);

  if (!rewrite_all_lost_and_found_to_file())
  {
    strcpy(lost_and_found_records[record_index].resolver_username, previous_resolver);
    printf("Error: Failed to persist lost and found record update.");
    return false;
  }

  return true;
}

int get_lost_and_found_for_branch(
  const char branch_name[], lost_and_found_record_t *destination, int destination_capacity
)
{
  if (branch_name == NULL || destination == NULL || destination_capacity <= 0) return 0;

  int copied_count = 0;
  for (int i = 0; i < lost_and_found_count && copied_count < destination_capacity; i++)
  {
    if (strcmp(lost_and_found_records[i].gym_branch, branch_name) == 0)
    {
      destination[copied_count] = lost_and_found_records[i];
      copied_count++;
    }
  }

  return copied_count;
}

int get_lost_and_found_for_reporter(
  const char reporter_username[], lost_and_found_record_t *destination, int destination_capacity
)
{
  if (reporter_username == NULL || destination == NULL || destination_capacity <= 0) return 0;

  int copied_count = 0;
  for (int i = 0; i < lost_and_found_count && copied_count < destination_capacity; i++)
  {
    if (strcmp(lost_and_found_records[i].reporter_username, reporter_username) == 0)
    {
      destination[copied_count] = lost_and_found_records[i];
      copied_count++;
    }
  }

  return copied_count;
}

int get_lost_and_found_for_resolver(
  const char resolver_username[], lost_and_found_record_t *destination, int destination_capacity
)
{
  if (resolver_username == NULL || destination == NULL || destination_capacity <= 0) return 0;

  int copied_count = 0;
  for (int i = 0; i < lost_and_found_count && copied_count < destination_capacity; i++)
  {
    if (strcmp(lost_and_found_records[i].resolver_username, resolver_username) == 0)
    {
      destination[copied_count] = lost_and_found_records[i];
      copied_count++;
    }
  }

  return copied_count;
}

bool get_lost_and_found_by_id(id_t id, lost_and_found_record_t *destination)
{
  if (destination == NULL) return false;
  for (int i = 0; i < lost_and_found_count; i++)
  {
    if (lost_and_found_records[i].id == id)
    {
      *destination = lost_and_found_records[i];
      return true;
    }
  }
  return false;
}

// ===================== policy.c =====================

bool ensure_membership_approval_is_allowed(id_t gym_member_id)
{
  if (session_is_sysadmin()) return true;
  if (!session_is_branch_manager()) return false;
  gym_member_t member;
  if (!get_gym_member_by_id(gym_member_id, &member)) return false;
  return session_belongs_to_branch(member.gym_branch);
}

bool ensure_membership_suspension_is_allowed(id_t gym_member_id)
{
  if (session_is_sysadmin()) return true;
  if (!session_is_branch_manager()) return false;

  gym_member_t member;
  if (!get_gym_member_by_id(gym_member_id, &member)) return false;
  return session_belongs_to_branch(member.gym_branch);
}

bool ensure_membership_unsuspension_is_allowed(id_t gym_member_id)
{
  if (session_is_sysadmin()) return true;
  if (!session_is_branch_manager()) return false;
  gym_member_t member;
  if (!get_gym_member_by_id(gym_member_id, &member)) return false;
  return session_belongs_to_branch(member.gym_branch);
}

bool ensure_digital_payment_is_allowed(id_t gym_member_id)
{
  if (session_is_sysadmin()) return true;
  if (!session_is_member()) return false;
  return get_user_id_from_session() == gym_member_id;
}

bool ensure_cash_payment_recording_is_allowed(id_t gym_member_id)
{
  if (session_is_sysadmin()) return true;
  if (session_is_member()) return false;
  gym_member_t member;
  if (!get_gym_member_by_id(gym_member_id, &member)) return false;
  return session_belongs_to_branch(member.gym_branch);
}

bool ensure_payment_view_allowed(id_t gym_member_id)
{
  if (session_is_sysadmin()) return true;
  if (session_is_member()) return get_user_id_from_session() == gym_member_id;
  gym_member_t member;
  if (!get_gym_member_by_id(gym_member_id, &member)) return false;
  return session_belongs_to_branch(member.gym_branch);
}

bool ensure_member_profile_view_allowed(id_t gym_member_id)
{
  if (session_is_sysadmin()) return true;
  if (session_is_member()) return get_user_id_from_session() == gym_member_id;
  gym_member_t member;
  if (!get_gym_member_by_id(gym_member_id, &member)) return false;
  return session_belongs_to_branch(member.gym_branch);
}

bool ensure_lost_found_resolution_is_allowed(const lost_and_found_record_t item_payload)
{
  if (session_is_sysadmin()) return true;
  if (!session_is_branch_manager()) return false;
  return session_belongs_to_branch(item_payload.gym_branch);
}

bool ensure_branch_deletion_is_allowed(const char branch_name[])
{
  if (is_blank_string(branch_name)) return false;
  if (session_is_sysadmin()) return true;
  return false;
}

bool ensure_member_deletion_is_allowed(id_t gym_member_id)
{
  if (session_is_sysadmin()) return true;
  if (!session_is_branch_manager()) return false;
  gym_member_t member;
  if (!get_gym_member_by_id(gym_member_id, &member)) return false;
  if (!session_belongs_to_branch(member.gym_branch)) return false;
  return member.due_amount == 0;
}

bool ensure_staff_deletion_is_allowed(id_t staff_id)
{
  if (session_is_sysadmin()) return true;
  if (!session_is_branch_manager()) return false;
  branch_staff_t staff;
  if (!get_branch_staff_by_id(staff_id, &staff)) return false;
  if (!session_belongs_to_branch(staff.gym_branch)) return false;
  return staff.role == TRAINER;
}

bool ensure_branch_creation_is_allowed()
{
  if (!session_is_sysadmin()) return false;
  if (get_branch_count() >= BRANCH_COUNT_MAX) return false;
  return true;
}

bool ensure_branch_rename_is_allowed()
{
  return session_is_sysadmin();
}

bool ensure_staff_creation_is_allowed(const char branch_name[], staff_role_t role)
{
  if (is_blank_string(branch_name)) return false;
  if (find_branch(branch_name) == -1) return false;
  switch (role)
  {
  case BRANCH_MANAGER:
    if (branch_manager_count(branch_name) >= MAX_MANAGERS_PER_BRANCH) return false;
    break;
  case TRAINER:
    if (branch_trainer_count(branch_name) >= MAX_TRAINERS_PER_BRANCH) return false;
    break;
  default:
    return false;
  }
  if (session_is_sysadmin()) return true;
  if (role == TRAINER && session_is_branch_manager() && session_belongs_to_branch(branch_name)) return true;
  return false;
}

bool ensure_gym_member_creation_is_allowed(const char branch_name[])
{
  if (is_blank_string(branch_name)) return false;
  if (find_branch(branch_name) == -1) return false;
  if (branch_member_count(branch_name) >= MAX_MEMBERS_PER_BRANCH) return false;
  return true;
}

bool ensure_branch_name_is_valid(const char branch_name[])
{
  if (is_blank_string(branch_name)) return false;
  if (find_branch(branch_name) == -1) return false;
  return true;
}

bool ensure_branch_listing_is_allowed()
{
  return session_is_active();
}

bool ensure_member_listing_is_allowed(const char branch_name[])
{
  if (!session_is_active()) return false;
  if (session_is_sysadmin()) return true;
  if (is_blank_string(branch_name)) return false;
  return session_belongs_to_branch(branch_name);
}

bool ensure_lost_found_view_is_allowed(const char branch_name[])
{
  if (!session_is_active()) return false;
  if (session_is_sysadmin()) return true;
  if (is_blank_string(branch_name)) return false;
  return session_belongs_to_branch(branch_name);
}

bool ensure_lost_found_report_is_allowed(const char branch_name[])
{
  if (!session_is_active()) return false;
  if (is_blank_string(branch_name)) return false;
  if (session_is_sysadmin()) return true;
  return session_belongs_to_branch(branch_name);
}

bool ensure_staff_listing_is_allowed(const char branch_name[])
{
  if (!session_is_active()) return false;
  if (session_is_sysadmin()) return true;
  if (!session_is_branch_manager()) return false;
  if (is_blank_string(branch_name)) return false;
  return session_belongs_to_branch(branch_name);
}

// ===================== menu.c =====================

static bool prompt_string_input(const char prompt[], char *destination, int destination_capacity)
{
  printf("%s ", prompt);
  char raw[LINE_BUFFER_SIZE];
  if (!input_string(raw, LINE_BUFFER_SIZE))
  {
    printf("Error: Failed to read input for '%s'.", prompt);
    return false;
  }
  char trimmed[LINE_BUFFER_SIZE];
  trim(raw, trimmed, LINE_BUFFER_SIZE);
  if (!sanitize_field(trimmed, destination, destination_capacity))
  {
    printf("Error: Failed to sanitize input for '%s'.", prompt);
    return false;
  }
  return true;
}

static bool prompt_integer_input(const char prompt[], int *destination)
{
  printf("%s ", prompt);
  if (!input_integer(destination))
  {
    printf("Error: Invalid integer input for '%s'.", prompt);
    return false;
  }
  return true;
}

static bool prompt_positive_input(const char prompt[], int *destination)
{
  printf("%s ", prompt);
  if (!input_positive_int(destination))
  {
    printf("Error: Invalid positive integer input for '%s'.", prompt);
    return false;
  }
  return true;
}

static inline const char *membership_status_text(membership_status_t status)
{
  switch (status)
  {
  case MEMBERSHIP_ON_HOLD:
    return "on_hold";
  case MEMBERSHIP_ACTIVE:
    return "active";
  case MEMBERSHIP_SUSPENDED:
    return "suspended";
  case MEMBERSHIP_CANCELLED:
    return "cancelled";
  default:
    return "unknown";
  }
}

static inline const char *payment_status_text(payment_status_t status)
{
  switch (status)
  {
  case PAYMENT_PENDING:
    return "pending";
  case PAYMENT_COMPLETED:
    return "completed";
  case PAYMENT_FAILED:
    return "failed";
  case PAYMENT_INVALID:
    return "invalid";
  default:
    return "unknown";
  }
}

static inline void print_datetime(const datetime_t datetime_payload)
{
  char buffer[DATETIME_BUFFER_SIZE];
  if (!format_datetime(datetime_payload, buffer, DATETIME_BUFFER_SIZE))
    printf("invalid-datetime");
  else
    printf("%s", buffer);
}

static inline void print_member_line(const gym_member_t member_payload)
{
  printf(
    "  id=%lu name=%s username=%s branch=%s status=%s due=%u plan=%u/%u days\n", (unsigned long)member_payload.id,
    member_payload.full_name, member_payload.username, member_payload.gym_branch,
    membership_status_text(member_payload.status), member_payload.due_amount, member_payload.plan.payable_amount,
    member_payload.plan.interval_days
  );
}

static inline void print_member_profile(const gym_member_t member_payload)
{
  printf(
    "Profile id=%lu name=%s email=%s phone=%s branch=%s username=%s status=%s\n", (unsigned long)member_payload.id,
    member_payload.full_name, member_payload.email, member_payload.phone_number, member_payload.gym_branch,
    member_payload.username, membership_status_text(member_payload.status)
  );
  printf(
    "  plan %u/%u days due=%u joined=", member_payload.plan.payable_amount, member_payload.plan.interval_days,
    member_payload.due_amount
  );
  print_datetime(member_payload.joined_at);
  printf(" last_payment=");
  print_datetime(member_payload.last_payment_date);
  printf("\n");
}

static inline void print_suspension_line(const suspension_record_t record_payload)
{
  printf("  id=%lu reason=%s suspension=", (unsigned long)record_payload.id, record_payload.reason);
  print_datetime(record_payload.suspension_date);
  printf(" unsuspension=");
  if (is_empty_datetime(record_payload.unsuspension_date))
    printf("-");
  else
    print_datetime(record_payload.unsuspension_date);
  printf("\n");
}

static inline void print_payment_line(const payment_t payment_payload)
{
  printf(
    "  id=%lu member=%lu amount=%u type=%s status=%s time=", (unsigned long)payment_payload.id,
    (unsigned long)payment_payload.gym_member_id, payment_payload.amount,
    payment_payload.transaction_type == CASH_TRANSACTION ? "cash" : "digital",
    payment_status_text(payment_payload.status)
  );
  print_datetime(payment_payload.transaction_time);
  if (!is_blank_string(payment_payload.transaction_id)) printf(" trx=%s", payment_payload.transaction_id);
  printf("\n");
}

static inline void print_lost_found_line(const lost_and_found_record_t record_payload)
{
  printf(
    "  id=%lu branch=%s reporter=%s resolver=%s time=", (unsigned long)record_payload.id, record_payload.gym_branch,
    record_payload.reporter_username,
    is_blank_string(record_payload.resolver_username) ? "-" : record_payload.resolver_username
  );
  print_datetime(record_payload.reported_at);
  printf(" desc=%s\n", record_payload.description);
}

static inline void print_staff_line(const branch_staff_t staff_payload)
{
  printf(
    "  id=%lu name=%s username=%s branch=%s role=%s email=%s phone=%s\n", (unsigned long)staff_payload.id,
    staff_payload.full_name, staff_payload.username, staff_payload.gym_branch,
    staff_payload.role == BRANCH_MANAGER ? "manager" : "trainer", staff_payload.email, staff_payload.phone_number
  );
}

static inline void run_auto_suspend_sweep()
{
  int suspended = auto_suspend_overdue_members();
  if (suspended > 0) printf("Auto-suspended %d overdue member(s).\n", suspended);
}

static inline bool resolve_target_branch(const char prompt[], char *destination, int destination_capacity)
{
  if (session_is_sysadmin()) return prompt_string_input(prompt, destination, destination_capacity);
  const char *_branch = get_branch_name_from_session();
  if (_branch == NULL)
  {
    printf("Error: Branch name retrieval failed.");
    return false;
  }
  if (destination_capacity <= (int)strlen(_branch))
  {
    printf("Error: Destination buffer too tight for branch name.");
    return false;
  }
  strcpy(destination, _branch);
  return true;
}

static void handle_add_branch()
{
  if (!ensure_branch_creation_is_allowed()) return;
  char name[BRANCH_NAME_BUFFER_SIZE];
  if (!prompt_string_input("Enter branch name:", name, BRANCH_NAME_BUFFER_SIZE)) return;
  if (is_blank_string(name))
  {
    printf("Branch name cannot be empty.\n");
    return;
  }
  if (add_branch(name))
    printf("Branch '%s' added.\n", name);
  else
    printf("Failed to add branch '%s'.\n", name);
}

static void handle_delete_branch()
{
  char name[BRANCH_NAME_BUFFER_SIZE];
  if (!prompt_string_input("Enter branch name to delete:", name, BRANCH_NAME_BUFFER_SIZE)) return;
  if (!ensure_branch_deletion_is_allowed(name)) return;
  if (delete_branch(name))
    printf("Branch '%s' deleted.\n", name);
  else
    printf("Failed to delete branch '%s'.\n", name);
}

static void handle_rename_branch()
{
  if (!ensure_branch_rename_is_allowed()) return;
  char old_name[BRANCH_NAME_BUFFER_SIZE];
  char new_name[BRANCH_NAME_BUFFER_SIZE];
  if (!prompt_string_input("Enter current branch name:", old_name, BRANCH_NAME_BUFFER_SIZE)) return;
  if (!prompt_string_input("Enter new branch name:", new_name, BRANCH_NAME_BUFFER_SIZE)) return;
  if (update_branch_name(old_name, new_name))
    printf("Branch renamed to '%s'.\n", new_name);
  else
    printf("Failed to rename branch.\n");
}

static void handle_list_branches()
{
  if (!ensure_branch_listing_is_allowed()) return;
  int count = get_branch_count();
  if (count == 0)
  {
    printf("No branches.\n");
    return;
  }
  for (int i = 0; i < count; i++)
  {
    const char *name = get_branch_name(i);
    if (name != NULL) printf("  %d. %s\n", i + 1, name);
  }
}

static void handle_create_staff()
{
  char full_name[FULL_NAME_BUFFER_SIZE];
  char email[EMAIL_BUFFER_SIZE];
  char phone[PHONE_BUFFER_SIZE];
  char branch[BRANCH_NAME_BUFFER_SIZE];
  char username[USERNAME_BUFFER_SIZE];
  char password[USERNAME_BUFFER_SIZE];
  int role_choice = 0;
  if (!prompt_string_input("Enter full name:", full_name, FULL_NAME_BUFFER_SIZE)) return;
  if (!prompt_string_input("Enter email:", email, EMAIL_BUFFER_SIZE)) return;
  if (!prompt_string_input("Enter phone (01XXXXXXXXX):", phone, PHONE_BUFFER_SIZE)) return;
  if (!prompt_string_input("Enter branch name:", branch, BRANCH_NAME_BUFFER_SIZE)) return;
  if (!ensure_branch_name_is_valid(branch)) return;
  if (!prompt_string_input("Enter username:", username, USERNAME_BUFFER_SIZE)) return;
  if (!prompt_string_input("Enter password:", password, USERNAME_BUFFER_SIZE)) return;
  if (!prompt_integer_input("Choose role (1. Branch Manager  2. Trainer): ", &role_choice)) return;
  staff_role_t role;
  switch (role_choice)
  {
  case 1:
    role = BRANCH_MANAGER;
    break;
  case 2:
    role = TRAINER;
    break;
  default:
    printf("Invalid role. Choose 1 for Manager or 2 for Trainer.\n");
    return;
  }
  if (!ensure_staff_creation_is_allowed(branch, role)) return;
  char hash[PASSWORD_HASH_BUFFER_SIZE];
  hash_password(password, hash);
  id_t id = create_branch_staff(full_name, email, phone, branch, username, hash, role);
  if (id != 0)
    printf("Staff created with id %lu.\n", (unsigned long)id);
  else
    printf("Failed to create staff.\n");
}

static void handle_delete_staff()
{
  int staff_id = 0;
  if (!prompt_positive_input("Enter staff id to delete:", &staff_id)) return;
  if (!ensure_staff_deletion_is_allowed((id_t)staff_id)) return;
  if (delete_branch_staff((id_t)staff_id))
    printf("Staff %d deleted.\n", staff_id);
  else
    printf("Failed to delete staff %d.\n", staff_id);
}

static void handle_register_member()
{
  char full_name[FULL_NAME_BUFFER_SIZE];
  char email[EMAIL_BUFFER_SIZE];
  char phone[PHONE_BUFFER_SIZE];
  char branch[BRANCH_NAME_BUFFER_SIZE];
  char username[USERNAME_BUFFER_SIZE];
  char password[USERNAME_BUFFER_SIZE];
  if (!prompt_string_input("Enter full name:", full_name, FULL_NAME_BUFFER_SIZE)) return;
  if (!prompt_string_input("Enter email:", email, EMAIL_BUFFER_SIZE)) return;
  if (!prompt_string_input("Enter phone (01XXXXXXXXX):", phone, PHONE_BUFFER_SIZE)) return;
  if (!prompt_string_input("Enter branch name:", branch, BRANCH_NAME_BUFFER_SIZE)) return;
  if (!ensure_branch_name_is_valid(branch)) return;
  if (!ensure_gym_member_creation_is_allowed(branch)) return;
  if (!prompt_string_input("Enter username:", username, USERNAME_BUFFER_SIZE)) return;
  if (!prompt_string_input("Enter password:", password, USERNAME_BUFFER_SIZE)) return;
  subscription_plan_t plan = {DEFAULT_PLAN_AMOUNT, DEFAULT_PLAN_INTERVAL_DAYS};
  char hash[PASSWORD_HASH_BUFFER_SIZE];
  hash_password(password, hash);
  id_t id = create_gym_member(full_name, email, phone, branch, username, hash, plan, MEMBERSHIP_ON_HOLD);
  if (id != 0)
    printf("Member registered with id %lu (on_hold).\n", (unsigned long)id);
  else
    printf("Failed to register member.\n");
}

static void handle_approve_member()
{
  int member_id = 0;
  if (!prompt_positive_input("Enter member id to approve:", &member_id)) return;
  if (!ensure_membership_approval_is_allowed((id_t)member_id)) return;
  if (approve_gym_member((id_t)member_id))
    printf("Member %d approved.\n", member_id);
  else
    printf("Failed to approve member %d.\n", member_id);
}

static void handle_suspend_member()
{
  int member_id = 0;
  if (!prompt_positive_input("Enter member id to suspend:", &member_id)) return;
  if (!ensure_membership_suspension_is_allowed((id_t)member_id)) return;
  char reason[REASON_BUFFER_SIZE];
  if (!prompt_string_input("Enter suspension reason:", reason, REASON_BUFFER_SIZE)) return;
  if (is_blank_string(reason))
  {
    printf("Reason cannot be empty.\n");
    return;
  }
  if (suspend_gym_member((id_t)member_id, reason))
    printf("Member %d suspended.\n", member_id);
  else
    printf("Failed to suspend member %d.\n", member_id);
}

static void handle_unsuspend_member()
{
  int member_id = 0;
  if (!prompt_positive_input("Enter member id to unsuspend:", &member_id)) return;
  if (!ensure_membership_unsuspension_is_allowed((id_t)member_id)) return;
  if (unsuspend_gym_member((id_t)member_id))
    printf("Member %d unsuspended.\n", member_id);
  else
    printf("Failed to unsuspend member %d.\n", member_id);
}

static void handle_delete_member()
{
  int member_id = 0;
  if (!prompt_positive_input("Enter member id to delete:", &member_id)) return;
  if (!ensure_member_deletion_is_allowed((id_t)member_id)) return;
  if (delete_gym_member((id_t)member_id))
    printf("Member %d deleted.\n", member_id);
  else
    printf("Failed to delete member %d.\n", member_id);
}

static void handle_view_members_by_branch()
{
  char branch[BRANCH_NAME_BUFFER_SIZE];
  if (!resolve_target_branch("Enter branch name (empty for all):", branch, BRANCH_NAME_BUFFER_SIZE)) return;
  bool branch_is_blank = is_blank_string(branch);
  if (branch_is_blank && !session_is_sysadmin()) return;
  if (!branch_is_blank && !ensure_member_listing_is_allowed(branch)) return;
  int found = 0;
  id_t ids[64];
  gym_member_t member;
  for (membership_status_t s = MEMBERSHIP_ON_HOLD; s <= MEMBERSHIP_CANCELLED; s++)
  {
    int count = get_gym_member_ids_by_status(s, ids, 64);
    for (int i = 0; i < count; i++)
    {
      if (!get_gym_member_by_id(ids[i], &member)) continue;
      if (!branch_is_blank && strcmp(member.gym_branch, branch) != 0) continue;
      if (!session_is_sysadmin() && !session_belongs_to_branch(member.gym_branch)) continue;
      print_member_line(member);
      found++;
    }
  }
  if (found == 0) printf("No members found.\n");
}

static void handle_view_staffs_by_branch()
{
  char branch[BRANCH_NAME_BUFFER_SIZE];
  if (!resolve_target_branch("Enter branch name (empty for all):", branch, BRANCH_NAME_BUFFER_SIZE)) return;
  if (!ensure_staff_listing_is_allowed(branch)) return;
  bool branch_is_blank = is_blank_string(branch);
  int found = 0;
  int total = get_branch_staff_count();
  for (int i = 0; i < total; i++)
  {
    branch_staff_t staff;
    if (!get_branch_staff_at(i, &staff)) continue;
    if (!branch_is_blank && strcmp(staff.gym_branch, branch) != 0) continue;
    if (!session_is_sysadmin() && !session_belongs_to_branch(staff.gym_branch)) continue;
    if (session_is_branch_manager() && staff.role != TRAINER) continue;
    print_staff_line(staff);
    found++;
  }
  if (found == 0) printf("No staff found.\n");
}

static void handle_view_own_profile()
{
  if (!session_is_member()) return;
  id_t id = get_user_id_from_session();
  if (!ensure_member_profile_view_allowed(id)) return;
  gym_member_t member;
  if (!get_gym_member_by_id(id, &member)) return;
  print_member_profile(member);
}

static void handle_record_digital_payment()
{
  int member_id = 0;
  if (session_is_member())
    member_id = (int)get_user_id_from_session();
  else if (!prompt_positive_input("Enter member id:", &member_id))
    return;
  if (!ensure_digital_payment_is_allowed((id_t)member_id)) return;
  int amount = 0;
  if (!prompt_positive_input("Enter amount:", &amount)) return;
  char trx[TRX_ID_BUFFER_SIZE];
  if (!prompt_string_input("Enter transaction id:", trx, TRX_ID_BUFFER_SIZE)) return;
  char status_text[32];
  if (!prompt_string_input("Enter status (pending/completed/failed/invalid):", status_text, 32)) return;
  to_lowercase(status_text);
  payment_status_t status;
  if (strcmp(status_text, "pending") == 0)
    status = PAYMENT_PENDING;
  else if (strcmp(status_text, "completed") == 0)
    status = PAYMENT_COMPLETED;
  else if (strcmp(status_text, "failed") == 0)
    status = PAYMENT_FAILED;
  else if (strcmp(status_text, "invalid") == 0)
    status = PAYMENT_INVALID;
  else
  {
    printf("Invalid payment status.\n");
    return;
  }
  digital_payment_request_t req = {0, (id_t)member_id, (unsigned int)amount, now_datetime(), "", status};
  strcpy(req.transaction_id, trx);
  if (record_digital_payment(req))
    printf("Digital payment recorded.\n");
  else
    printf("Failed to record digital payment.\n");
}

static void handle_record_cash_payment()
{
  int member_id = 0;
  if (!prompt_positive_input("Enter member id:", &member_id)) return;
  if (!ensure_cash_payment_recording_is_allowed((id_t)member_id)) return;
  int amount = 0;
  if (!prompt_positive_input("Enter amount:", &amount)) return;
  if (record_cash_payment((id_t)member_id, (unsigned int)amount))
    printf("Cash payment recorded.\n");
  else
    printf("Failed to record cash payment.\n");
}

static void handle_view_payments()
{
  int member_id = 0;
  if (!prompt_positive_input("Enter member id to view:", &member_id)) return;
  if (!ensure_payment_view_allowed((id_t)member_id)) return;
  payment_t list[32];
  int count = get_payments_for_member((id_t)member_id, list, 32);
  if (count == 0)
  {
    printf("No payments for member %d.\n", member_id);
    return;
  }
  for (int i = 0; i < count; i++) print_payment_line(list[i]);
  if (count == 32) printf("  (showing first 32)\n");
}

static void handle_view_own_suspensions()
{
  id_t id = get_user_id_from_session();
  suspension_record_t list[16];
  int count = get_suspensions_for_member(id, list, 16);
  if (count == 0)
  {
    printf("No suspensions.\n");
    return;
  }
  for (int i = 0; i < count; i++) print_suspension_line(list[i]);
}

static void handle_report_lost_found()
{
  char branch[BRANCH_NAME_BUFFER_SIZE];
  if (!resolve_target_branch("Enter branch name:", branch, BRANCH_NAME_BUFFER_SIZE)) return;
  if (!ensure_branch_name_is_valid(branch)) return;
  if (!ensure_lost_found_report_is_allowed(branch)) return;
  char desc[DESCRIPTION_BUFFER_SIZE];
  if (!prompt_string_input("Enter description:", desc, DESCRIPTION_BUFFER_SIZE)) return;
  if (report_lost_item(get_username_from_session(), branch, desc))
    printf("Report submitted.\n");
  else
    printf("Failed to submit report.\n");
}

static void handle_view_lost_found()
{
  char branch[BRANCH_NAME_BUFFER_SIZE];
  if (!resolve_target_branch("Enter branch name:", branch, BRANCH_NAME_BUFFER_SIZE)) return;
  if (!ensure_lost_found_view_is_allowed(branch)) return;
  lost_and_found_record_t list[32];
  int count = get_lost_and_found_for_branch(branch, list, 32);
  if (count == 0)
  {
    printf("No reports for branch '%s'.\n", branch);
    return;
  }
  for (int i = 0; i < count; i++) print_lost_found_line(list[i]);
  if (count == 32) printf("  (showing first 32)\n");
}

static void handle_resolve_lost_found()
{
  int record_id = 0;
  if (!prompt_positive_input("Enter report id to resolve:", &record_id)) return;
  lost_and_found_record_t item;
  if (!get_lost_and_found_by_id((id_t)record_id, &item)) return;
  if (!ensure_lost_found_resolution_is_allowed(item)) return;
  if (resolve_lost_item((id_t)record_id, get_username_from_session()))
    printf("Report %d resolved.\n", record_id);
  else
    printf("Failed to resolve report %d.\n", record_id);
}

static void run_sysadmin_menu()
{
  while (session_is_sysadmin())
  {
    printf("\n-- Sysadmin Menu --\n");
    printf(" 1. Add branch\n");
    printf(" 2. Delete branch\n");
    printf(" 3. Rename branch\n");
    printf(" 4. List branches\n");
    printf(" 5. Create staff\n");
    printf(" 6. Delete staff\n");
    printf(" 7. Register member\n");
    printf(" 8. Approve member\n");
    printf(" 9. Suspend member\n");
    printf("10. Unsuspend member\n");
    printf("11. Delete member\n");
    printf("12. List members\n");
    printf("13. Digital pay\n");
    printf("14. Cash pay\n");
    printf("15. View payments\n");
    printf("16. Report Lost & Found\n");
    printf("17. View Lost & Found\n");
    printf("18. Resolve Lost & Found\n");
    printf("19. List staffs\n");
    printf(" 0. Logout\n");
    int choice = 0;
    if (!prompt_integer_input("\nChoose option:", &choice)) continue;
    printf("\n");
    switch (choice)
    {
    case 1:
      handle_add_branch();
      run_auto_suspend_sweep();
      break;
    case 2:
      handle_delete_branch();
      run_auto_suspend_sweep();
      break;
    case 3:
      handle_rename_branch();
      run_auto_suspend_sweep();
      break;
    case 4:
      handle_list_branches();
      run_auto_suspend_sweep();
      break;
    case 5:
      handle_create_staff();
      run_auto_suspend_sweep();
      break;
    case 6:
      handle_delete_staff();
      run_auto_suspend_sweep();
      break;
    case 7:
      handle_register_member();
      run_auto_suspend_sweep();
      break;
    case 8:
      handle_approve_member();
      run_auto_suspend_sweep();
      break;
    case 9:
      handle_suspend_member();
      run_auto_suspend_sweep();
      break;
    case 10:
      handle_unsuspend_member();
      run_auto_suspend_sweep();
      break;
    case 11:
      handle_delete_member();
      run_auto_suspend_sweep();
      break;
    case 12:
      handle_view_members_by_branch();
      run_auto_suspend_sweep();
      break;
    case 13:
      handle_record_digital_payment();
      run_auto_suspend_sweep();
      break;
    case 14:
      handle_record_cash_payment();
      run_auto_suspend_sweep();
      break;
    case 15:
      handle_view_payments();
      run_auto_suspend_sweep();
      break;
    case 16:
      handle_report_lost_found();
      run_auto_suspend_sweep();
      break;
    case 17:
      handle_view_lost_found();
      run_auto_suspend_sweep();
      break;
    case 18:
      handle_resolve_lost_found();
      run_auto_suspend_sweep();
      break;
    case 19:
      handle_view_staffs_by_branch();
      run_auto_suspend_sweep();
      break;
    case 0:
      auth_logout();
      printf("Logged out.\n");
      break;
    default:
      printf("Invalid option.\n");
      break;
    }
  }
}

static void run_branch_manager_menu()
{
  while (session_is_branch_manager())
  {
    printf("\n-- Branch Manager Menu (%s) --\n", get_branch_name_from_session());
    printf(" 1. List branches\n");
    printf(" 2. Approve member\n");
    printf(" 3. Suspend member\n");
    printf(" 4. Unsuspend member\n");
    printf(" 5. Delete member\n");
    printf(" 6. Delete trainer\n");
    printf(" 7. List members\n");
    printf(" 8. Cash pay\n");
    printf(" 9. View payments\n");
    printf("10. View Lost & Found\n");
    printf("11. Resolve Lost & Found\n");
    printf("12. Report Lost & Found\n");
    printf("13. Create trainer\n");
    printf("14. List trainers\n");
    printf(" 0. Logout\n");
    int choice = 0;
    if (!prompt_integer_input("\nChoose option:", &choice)) continue;
    printf("\n");
    switch (choice)
    {
    case 1:
      handle_list_branches();
      run_auto_suspend_sweep();
      break;
    case 2:
      handle_approve_member();
      run_auto_suspend_sweep();
      break;
    case 3:
      handle_suspend_member();
      run_auto_suspend_sweep();
      break;
    case 4:
      handle_unsuspend_member();
      run_auto_suspend_sweep();
      break;
    case 5:
      handle_delete_member();
      run_auto_suspend_sweep();
      break;
    case 6:
      handle_delete_staff();
      run_auto_suspend_sweep();
      break;
    case 7:
      handle_view_members_by_branch();
      run_auto_suspend_sweep();
      break;
    case 8:
      handle_record_cash_payment();
      run_auto_suspend_sweep();
      break;
    case 9:
      handle_view_payments();
      run_auto_suspend_sweep();
      break;
    case 10:
      handle_view_lost_found();
      run_auto_suspend_sweep();
      break;
    case 11:
      handle_resolve_lost_found();
      run_auto_suspend_sweep();
      break;
    case 12:
      handle_report_lost_found();
      run_auto_suspend_sweep();
      break;
    case 13:
      handle_create_staff();
      run_auto_suspend_sweep();
      break;
    case 14:
      handle_view_staffs_by_branch();
      run_auto_suspend_sweep();
      break;
    case 0:
      auth_logout();
      printf("Logged out.\n");
      break;
    default:
      printf("Invalid option.\n");
      break;
    }
  }
}

static void run_trainer_menu()
{
  while (session_is_trainer())
  {
    printf("\n-- Trainer Menu (%s) --\n", get_branch_name_from_session());
    printf(" 1. List members\n");
    printf(" 2. List payments\n");
    printf(" 3. Cash pay\n");
    printf(" 4. View Lost & Found\n");
    printf(" 5. Report Lost & Found\n");
    printf(" 0. Logout\n");
    int choice = 0;
    if (!prompt_integer_input("\nChoose option:", &choice)) continue;
    printf("\n");
    switch (choice)
    {
    case 1:
      handle_view_members_by_branch();
      run_auto_suspend_sweep();
      break;
    case 2:
      handle_view_payments();
      run_auto_suspend_sweep();
      break;
    case 3:
      handle_record_cash_payment();
      run_auto_suspend_sweep();
      break;
    case 4:
      handle_view_lost_found();
      run_auto_suspend_sweep();
      break;
    case 5:
      handle_report_lost_found();
      run_auto_suspend_sweep();
      break;
    case 0:
      auth_logout();
      printf("Logged out.\n");
      break;
    default:
      printf("Invalid option.\n");
      break;
    }
  }
}

static void run_member_menu()
{
  while (session_is_member())
  {
    printf("\n-- Member Menu (%s) --\n", get_branch_name_from_session());
    printf(" 1. View profile\n");
    printf(" 2. Digital pay\n");
    printf(" 3. View payments\n");
    printf(" 4. View suspensions\n");
    printf(" 5. Report Lost & Found\n");
    printf(" 6. View Lost & Found\n");
    printf(" 0. Logout\n");
    int choice = 0;
    if (!prompt_integer_input("\nChoose option:", &choice)) continue;
    printf("\n");
    switch (choice)
    {
    case 1:
      handle_view_own_profile();
      run_auto_suspend_sweep();
      break;
    case 2:
      handle_record_digital_payment();
      run_auto_suspend_sweep();
      break;
    case 3:
      handle_view_payments();
      run_auto_suspend_sweep();
      break;
    case 4:
      handle_view_own_suspensions();
      run_auto_suspend_sweep();
      break;
    case 5:
      handle_report_lost_found();
      run_auto_suspend_sweep();
      break;
    case 6:
      handle_view_lost_found();
      run_auto_suspend_sweep();
      break;
    case 0:
      auth_logout();
      printf("Logged out.\n");
      break;
    default:
      printf("Invalid option.\n");
      break;
    }
  }
}

static void handle_login()
{
  char username[USERNAME_BUFFER_SIZE];
  char password[USERNAME_BUFFER_SIZE];
  if (!prompt_string_input("Enter username:", username, USERNAME_BUFFER_SIZE)) return;
  if (!prompt_string_input("Enter password:", password, USERNAME_BUFFER_SIZE)) return;
  user_role_t role = 0;
  if (!auth_login(username, password, &role))
  {
    printf("Login failed.\n");
    return;
  }
  printf("Login successful as %s.\n", username);
  switch (role)
  {
  case USER_ROLE_SYSADMIN:
    run_sysadmin_menu();
    break;
  case USER_ROLE_BRANCH_MANAGER:
    run_branch_manager_menu();
    break;
  case USER_ROLE_TRAINER:
    run_trainer_menu();
    break;
  case USER_ROLE_MEMBER:
    run_member_menu();
    break;
  default:
    printf("Unknown role.\n");
    auth_logout();
    break;
  }
}

void run_main_menu()
{
  while (true)
  {
    printf("\n== Gymtrac ==\n");
    printf(" 1. Login\n");
    printf(" 2. Register as member\n");
    printf(" 0. Exit\n");
    int choice = 0;
    if (!prompt_integer_input("\nChoose option:", &choice)) continue;
    printf("\n");
    switch (choice)
    {
    case 1:
      handle_login();
      run_auto_suspend_sweep();
      break;
    case 2:
      handle_register_member();
      run_auto_suspend_sweep();
      break;
    case 0:
      printf("Goodbye.\n");
      return;
    default:
      printf("Invalid option.\n");
      break;
    }
  }
}
