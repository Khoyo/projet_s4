#include "pts.h"

#define BUFF_SIZE 1024

int fdm = -1; // master's file descriptor

static void sigwinch_handler(int signum)
{
  struct winsize ws;
  ioctl(0, TIOCGWINSZ, &ws);
  ioctl(fdm, TIOCSWINSZ, &ws);
}

void reg_winchange_handler()
{
  struct sigaction sa;
  sa.sa_handler = sigwinch_handler;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART; /* Restart functions if
                               interrupted by handler */
  if (sigaction(SIGWINCH, &sa, NULL) == -1)
    err(5, "Can't set signal handler");

}

int main() {

  //SET THE PARENT TERMINAL TO RAW AND NOECHO

  int log_fd;
  int fds = -1; // slave's file descriptor

  oussh_open_pt(O_RDWR, &fdm, &fds);

  pid_t pid = fork();
  set_term_size(fdm, get_term_size(0));

  if (pid < 0) {

    errx(EXIT_FAILURE, "main : fork failed");
  }
  else if (pid != 0) {

    // FATHER - MASTER
    char buffer[BUFF_SIZE];
    ssize_t ioerr = -1;

    setup_master_pt(fdm);
    setup_external_terminal();

    reg_winchange_handler();

    log_fd = open("log.txt", O_APPEND|O_CREAT|O_RDWR);


    // Set raw mode on the slave side of the PTY
    close(fds);
    fd_set fd_in;

    while (1) {
      FD_ZERO(&fd_in);
      FD_SET(0, &fd_in);
      FD_SET(fdm, &fd_in);

      if(select(fdm+1, &fd_in, NULL, NULL, NULL) == -1)
      {
        if(errno != ERESTART && errno != EINTR)
          err(3, "I/O error");
      }
      else
      {
        if(FD_ISSET(fdm, &fd_in))
        {
          //write(STDOUT_FILENO, " $ READ $\n", sizeof("Input : "));
          ioerr = read(fdm, buffer, BUFF_SIZE - 1);

          if (ioerr < 0)
          {
            tcsetattr(0, TCSANOW, &orig_external_term_settings);
            //err(EXIT_FAILURE, "main : read failed");
            printf("oussh: Exiting\n");
            exit(0);
          }

          buffer[ioerr] = '\0';
          fprintf(stderr,"%s", buffer);
        }
        if(FD_ISSET(0, &fd_in))
        {
          ioerr = read(STDIN_FILENO, buffer, BUFF_SIZE);
          if (ioerr < 0) { errx(EXIT_FAILURE, "main : read failed"); }

          write(log_fd, buffer, ioerr);
          fsync(log_fd);
          ioerr = write(fdm, buffer, ioerr);
          if (ioerr < 0) { errx(EXIT_FAILURE, "main : write failed"); }
        }
      }
    }


  }
  else {
    // CHILD - SLAVE
    char buffer[BUFF_SIZE];
    ssize_t ioerr = -1;

    close(fdm);

    setup_slave_pt(fds);
    close(fds);
    execlp("bash", NULL);

    while (1) {

      ioerr = read(fds, buffer, BUFF_SIZE - 1);
      if (ioerr < 0) { errx(EXIT_FAILURE, "main : read failed"); }
      buffer[ioerr] = '\0';

      printf("Child receve : %s", buffer);
    }
  }

  close(fdm);
  close(fds);

  system("ls -l /dev/pts");

  return 0;
}
