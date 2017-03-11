#define _XOPEN_SOURCE
#define _GNU_SOURCE
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <err.h>
#include <crypt.h>

/* pwd_create_hash()
 * IN  username : name of the user, must be 16 char or less
 * IN  clear_password : password of the user, not encrypted yet
 *
 * return : NULL if error
 *          the pointer to the result hash else */
char* pwd_create_hash(char* username, char* clear_password)
{
  int errval = -1; // error value
  size_t username_len = 0;
  char* salt = NULL;
  char* tmp_hash = NULL; // tmp hash if crypt fail
  size_t tmp_len = 0;
  char* hash = NULL; // result of the function

  username_len = strlen(username);
  if (username_len > 16)
  {
    warnx("The username must not have more than 16 charracters.");
    return NULL;
  }

  salt = malloc(sizeof(char) * (username_len + 5));
  if (salt == NULL)
  {
    return NULL;
  }

  errval = sprintf(salt, "$6$%s$", username);
  if (errval < 0)
  {
    warnx("sprintf failed");
    free(salt);
    return NULL;
  }
  
  tmp_hash = crypt(clear_password, salt);
  if (tmp_hash == NULL)
  {
    warnx("crypt failed");
    return NULL;
  }

  hash = malloc(tmp_len * sizeof(char));
  strncpy(hash, tmp_hash, tmp_len);
  free(salt);
  return hash;
}

/* pwd_compare_pwd_to_hash()
 * IN username : username bind to the password
 * IN clear_password : password to match with the hash
 * IN hash : hash of a password
 *
 * return : -1 if error
 *          0 if the pwd don't match with the hash
 *          else 1 */
int pwd_compare_pwd_to_hash(char* username, 
                            char* clear_password, 
                            char* old_hash) 
{
  char* new_hash = NULL;

  new_hash = pwd_create_hash(username, clear_password);
  if (new_hash == NULL)
  {
    warnx("pwd_create_hash failed");
    return -1;
  }

  int d1 = (strlen(new_hash) == strlen(old_hash));
  int d2 = strncmp(new_hash, old_hash, strlen(old_hash));
  
  if (d1 && d2 == 0)
  {
    free(new_hash);
    return 1;
  }
  else
  {
    free(new_hash);
    return 0;
  }
}

int main() 
{
  char* true_hash = pwd_create_hash("Max", "kek");
  int res = pwd_compare_pwd_to_hash("Max", "jpp", true_hash);
  warnx("res = %d", res);

  return 0;
}
