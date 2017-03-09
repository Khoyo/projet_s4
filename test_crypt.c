#define _XOPEN_SOURCE
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <err.h>

/* pwd_create_hash()
 * IN  username : name of the user, must be 16 char or less
 * IN  clear_password : password of the user, not encrypted yet
 * OUT hash : pointer to the hash of the password
 *
 * return int : if error : -1, else 0 */
int pwd_create_hash(char* username, char* clear_password, char** hash)
{
  int errval = -1; // error value
  size_t username_len = 0;
  char* salt = NULL;
  char* tmp_hash = NULL; // tmp hash if crypt fail

  username_len = strlen(username);
  if (username_len > 16)
  {
    warnx("The username must not have more than 16 charracters.");
    return -1;
  }

  salt = malloc(sizeof(char) * (username_len + 5));
  if (salt == NULL)
  {
    warnx("Malloc failed");
    return -1;
  }

  errval = sprintf(salt, "$6$%s$", username);
  if (errval < 0)
  {
    warnx("sprintf failed");
    return -1;
  }
  
  tmp_hash = crypt(clear_password, salt);
  if (tmp_hash == NULL)
  {
    warnx("crypt failed");
    return -1;
  }
  else // crypt() didnt fail, so we can modify "hash"
  {
    *hash = tmp_hash;
  }

  free(salt);
  return 0;
}

int main() 
{
  char* username = "Max";
  char* clear_password = "kek";
  char* hash = NULL;

  if ( pwd_create_hash(username, clear_password, &hash) == -1 )
  {
    printf("failed D:");
    return 1;
  }

  printf("Hash : %s", hash);
  return 0;
}

