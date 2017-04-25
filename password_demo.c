#define _XOPEN_SOURCE
#define _GNU_SOURCE
#include "password.h"
#include <stdio.h>


/* pwd_demo()
 * Ask in stdin username and password, search for a matching username in
 * hashfile, and compare the hash of the given pwd to the hash in the file
 *
 * return value : -1 if error
 *                 0 if login failed
 *                 1 if login success */
int pwd_demo() 
{
  char username[SALT_SIZE + 1];
  char password[PWD_SIZE + 1];

  printf("Please enter your username :\n");
  fgets(username, SALT_SIZE + 1, stdin);
  if (username == NULL)
  {
    warnx("scanf failed for username");
    return -1;
  }
  char* p = strchr(username, '\n');
  if (p != NULL)
  {
    *p = '\0';
  }

  printf("Please enter your password :\n");
  fgets(password, PWD_SIZE + 1, stdin);
  if (password == NULL)
  {
    warnx("scanf failed for password");
    return -1;
  }
  p = strchr(password, '\n');
  if (p != NULL)
  {
    *p = '\0';
  }

  char* current_hash = pwd_get_hash(username, password);
  if (current_hash == NULL)
  {
    printf("Incorrect input : hash failed\nClosing...\n");
    return -1;
  }

  char* file_hash = pwd_get_hash_from_file("hash_file", username);
  if (file_hash == NULL)
  {
    printf("Username not found : have you created a account ?\nClosing...\n");
    free(current_hash);
    return -1;
  }

  warnx("fh = %s", file_hash);
  warnx("ch = %s", current_hash);
  if (pwd_is_str_equals(file_hash, current_hash))
  {
    printf("You successfully logged in !\n");
    free(file_hash);
    free(current_hash);
    return 1;
  }

  free(file_hash);
  free(current_hash);
  printf("Login failed");
  return 0;
}

int main()
{
  char* hash = pwd_get_hash("root", "toor");
  pwd_add_new_hash_in_file("hash_file", hash);
  if (pwd_demo() == 1)
  {
    return 0;
  }

  return 1;
}
