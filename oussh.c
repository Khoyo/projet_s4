#include "pts.h"

#include <err.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#define BUFF_SIZE 256

void sigpipe_handler(int signum){
  tcsetattr(0, TCSANOW, &orig_external_term_settings);
  //err(EXIT_FAILURE, "main : read failed");
  fprintf(stderr, "oussh: sigppipe Exiting\n");
  flush(stderr);
  exit(0);
}

int main()
{
  signal(SIGPIPE, sigpipe_handler);
  int fd = socket(AF_UNIX, SOCK_STREAM, 0);

  struct sockaddr_un addr;
  char buffer[256];

  char socket_path[] = "unix.sock";

  memset(&addr, 0, sizeof(addr));
  addr.sun_family = AF_UNIX;
  if (*socket_path == '\0') {
    *addr.sun_path = '\0';
    strncpy(addr.sun_path+1, socket_path+1, sizeof(addr.sun_path)-2);
  } else {
    strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path)-1);
  }

  if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
    perror("connect error");
    exit(-1);
  }

  char msg[] = "echo toto\n";
  //write(fd, msg, sizeof(msg));
  setup_external_terminal();
  fd_set fd_in;
  while(1)
  {
    FD_ZERO(&fd_in);
    FD_SET(0, &fd_in);
    FD_SET(fd, &fd_in);
    int ioerr;
    if(select(fd+1, &fd_in, NULL, NULL, NULL) == -1)
    {
      if(errno != ERESTART && errno != EINTR)
        err(3, "I/O error");
      else
        continue;
    }

    if(FD_ISSET(fd, &fd_in))
    {
      //write(STDOUT_FILENO, " $ READ $\n", sizeof("Input : "));
      ioerr = read(fd, buffer, BUFF_SIZE - 1);

      if (ioerr < 0)
      {
        tcsetattr(0, TCSANOW, &orig_external_term_settings);
        //err(EXIT_FAILURE, "main : read failed");
        printf("oussh: Exiting\n");
        exit(0);
      }

      buffer[ioerr] = '\0';
      printf("%s", buffer);
      fflush(stdout);
      //printf("\n\n END RECV");
    }
    if(FD_ISSET(0, &fd_in))
    {
      ioerr = read(STDIN_FILENO, buffer, BUFF_SIZE);
      if (ioerr < 0) { errx(EXIT_FAILURE, "main : read failed"); }

      //write(log_fd, buffer, ioerr);
      //fsync(log_fd);
      //ioerr = write(1, "rcv", 4);
      ioerr = write(fd, buffer, ioerr);
      if (ioerr < 0) { errx(EXIT_FAILURE, "main : write failed"); }
    }
  }

}
