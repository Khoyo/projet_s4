#define _XOPEN_SOURCE
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char* argv[]) {

  if (argc > 3) {
    printf("You must enter 2 arguments : \n- Your username\n- Your password\n");
    return 1;
  }

  char* pwd = argv[2];
  char* salt = malloc(sizeof(char) * strlen(argv[1]) + 5);
  char* hash = NULL;

  sprintf(salt, "$6$%s$", argv[1]);
  
  char* old_hash = "$6$max$pqC9cKzGf9GnGLGdj2ZMMzUqtvr2fTFLlORBAvALAfAcPsvsGSEy2r21CVWbBuLzjWLx87xAykql59ERUDMqu0%";

  hash = crypt(pwd, salt);

  

  if (hash) 
  {
    if (strlen(hash) == strlen(old_hash) && strcmp(hash, old_hash))
      printf("you logged in :DDD");
    else
      printf("you didnt logged in DDD:");
  }
  else
    return 1;

  return 0;
}
