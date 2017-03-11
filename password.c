#define _XOPEN_SOURCE
#define _GNU_SOURCE
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <err.h>
#include <crypt.h>

static char* get_crypt_hash(char* pwd, char* salt) 
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

char* pwd_get_hash(char* username, char* clear_pwd)
{
  size_t username_len = 0;
  char* salt = NULL; // used with crypt()
  int errval = -1; // error value from sprintf
  char* hash = NULL; // returned value

  username_len = strlen(username);
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

int pwd_is_hash_equals(char* hash1, char* hash2)
{
  size_t hash1_len = strlen(hash1);
  size_t hash2_len = strlen(hash2);

  if (hash1_len != hash2_len)
  {
    return 0;
  }

  return !(strncmp(hash1, hash2, hash1_len));
}

int main()
{
  char* hash1 = pwd_get_hash("Max", "test1");
  char* hash2 = pwd_get_hash("Max", "test23");
  warnx("hash1 = %p", hash1);
  warnx("hash1 = %s", hash1);
  warnx("hash2 = %p", hash2);
  warnx("hash2 = %s", hash2);

  warnx("equal ? : %d", pwd_is_hash_equals(hash1, hash1));
  warnx("equal ? : %d", pwd_is_hash_equals(hash1, hash2));

  free(hash2);
  free(hash1);
  return 0;
}
