#define _XOPEN_SOURCE
#define _GNU_SOURCE
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <err.h>
#include <crypt.h>
#include <errno.h>

#define SALT_SIZE 17

static char* get_crypt_hash(const char* pwd, const char* salt) 
{
  char* tmp = NULL;
  size_t tmp_len = 0;
  char* hash = NULL;

  tmp = crypt(pwd, salt);
  if (tmp == NULL)
  {
    warnx("crypt failed");
    return NULL;
  }

  tmp_len = strlen(tmp);

  hash = malloc((tmp_len + 1) * sizeof(char));
  if (hash == NULL)
  {
    warnx("malloc failed");
    return NULL;
  }

  hash = strncpy(hash, tmp, tmp_len);
  hash[tmp_len] = '\0';

  return hash;
}

// USERNAME MUST NOT CONTAINS SPACES 
// (or other char used as separator in the hash_file)
char* pwd_get_hash(const char* username, const char* clear_pwd)
{
  size_t username_len = 0;
  char* salt = NULL; // used with crypt()
  int errval = -1; // error value from sprintf
  char* hash = NULL; // returned value

  username_len = strlen(username);
  //TODO : verify that username does not contains spaces 
  //(or other separator used in hash_file)
  if (username_len > 16)
  {
    warnx("The username must not have more than 16 char");
    return NULL;
  }

  salt = malloc(sizeof(char) * (username_len + 5)); // "$6$...$\0" = 5
  if (salt == NULL)
  {
    warnx("malloc failed");
    return NULL;
  }

  errval = sprintf(salt, "$6$%s$", username);
  if (errval < 0)
  {
    warnx("sprintf failed");
    free(salt);
    return NULL;
  }

  hash = get_crypt_hash(clear_pwd, salt);
  if (hash == NULL)
  {
    warnx("get_crypt_hash failed");
    free(salt);
    return NULL;
  }

  free(salt);
  return hash;
}

static int pwd_is_str_equals(const char* hash1, const char* hash2)
{
  size_t hash1_len = strlen(hash1);
  size_t hash2_len = strlen(hash2);

  if (hash1_len != hash2_len)
  {
    return 0;
  }

  return !(strncmp(hash1, hash2, hash1_len));
}

int pwd_set_new_hash_in_file(const char* hash_file_path, char* new_hash)
{
  FILE* hash_file = NULL;
  int errval = -1;

  hash_file = fopen(hash_file_path, "a+");
  if (hash_file == NULL)
  {
    warnx("fopen failed");
    return -1;
  }

  errval = fprintf(hash_file, "%s\n", new_hash); 
  if (errval < 0)
  {
    warnx("fprintf failed");
    fclose(hash_file);
    return -1;
  }

  fclose(hash_file);
  return 0;
}

static void fill_salt(char salt[SALT_SIZE], char* hash, size_t hash_size)
{
  size_t i = 0;
  while (i < SALT_SIZE - 1 && i < hash_size && hash[i + 3] != '$') 
  {
    salt[i] = hash[i + 3];
    i++;
  }

  salt[i] = '\0';
}

char* pwd_get_hash_from_file(const char* hash_file_path, const char* username)
{
  FILE* hash_file = fopen(hash_file_path, "a+");
  char* line = NULL;
  size_t line_len = 0;
  ssize_t errval = -1;
  char salt[SALT_SIZE];

  if (hash_file == NULL)
  {
    warnx("fopen failed");
    return NULL;
  }

  errval = getline(&line, &line_len, hash_file);

  while (errval != -1)
  {
    fill_salt(salt, line, line_len);

    if (pwd_is_str_equals(salt, username))
    {
      fclose(hash_file);
      return line;
    }

    free(line);
    line = NULL;
    errval = getline(&line, &line_len, hash_file);
  }
  

  fclose(hash_file);
  return NULL;
}

int main()
{
  char* hash1 = pwd_get_hash("Max", "test1");
  char* hash2 = pwd_get_hash("Max", "test23");
  warnx("hash1 = %p", hash1);
  warnx("hash1 = %s", hash1);
  warnx("hash2 = %p", hash2);
  warnx("hash2 = %s", hash2);

  pwd_set_new_hash_in_file("hash_file", hash1);
  warnx("hash in file : %s", pwd_get_hash_from_file("hash_file", "Mx"));

  free(hash2);
  free(hash1);
  return 0;
}
