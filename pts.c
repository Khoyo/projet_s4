#define _DEFAULT_SOURCE
#define _XOPEN_SOURCE 600
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <err.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/select.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <signal.h>
#include <errno.h>

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

void oussh_openPt(int flags, int* fdm, int* fds) {

  *fdm = -1;
  *fds = -1;
  int errval = -1;

  *fdm = posix_openpt(O_RDWR);
  if (*fdm < 0) { errx(EXIT_FAILURE, "oussh_openpt : openpt failed"); }

  errval = grantpt(*fdm);
  if (errval < 0) { errx(EXIT_FAILURE, "oussh_openpt : grantpt failed"); }

  errval = unlockpt(*fdm);
  if (errval < 0) { errx(EXIT_FAILURE, "oussh_openpt : unlockpt failed"); }

  *fds = open(ptsname(*fdm), flags);
  if (*fds < 0) { errx(EXIT_FAILURE, "oussh_openpt : open fds failed"); }
}

int main() {

  //SET THE PARENT TERMINAL TO RAW AND NOECHO

  int log_fd;
  int fds = -1; // slave's file descriptor

  oussh_openPt(O_RDWR, &fdm, &fds);

  pid_t pid = fork();
  struct winsize ws;
  ioctl(0, TIOCGWINSZ, &ws);
  ioctl(fdm, TIOCSWINSZ, &ws);

  if (pid < 0) {

    errx(EXIT_FAILURE, "main : fork failed");
  }
  else if (pid != 0) {

    // FATHER - MASTER
    struct termios master_orig_term_settings; // Saved terminal settings
    struct termios new_term_settings; // Current terminal settings
    char buffer[BUFF_SIZE];
    ssize_t ioerr = -1;

    reg_winchange_handler();

    log_fd = open("log.txt", O_APPEND|O_CREAT|O_RDWR);

    // Save the default parameters of the slave side of the PTY
    ioerr = tcgetattr(0, &master_orig_term_settings);

    // Set raw mode on the slave side of the PTY
    new_term_settings = master_orig_term_settings;
    cfmakeraw(&new_term_settings);
    tcsetattr(0, TCSANOW, &new_term_settings);

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
            tcsetattr(0, TCSANOW, &master_orig_term_settings);
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

    struct termios slave_orig_term_settings; // Saved terminal settings
    struct termios new_term_settings; // Current terminal settings

    // CHILD - SLAVE
    char buffer[BUFF_SIZE];
    ssize_t ioerr = -1;


    // Save the default parameters of the slave side of the PTY
    ioerr = tcgetattr(fds, &slave_orig_term_settings);

    // Set raw mode on the slave side of the PTY
    new_term_settings = slave_orig_term_settings;
    //cfmakeraw (&new_term_settings);
    tcsetattr (fds, TCSANOW, &new_term_settings);

    close(fdm);

    close(0); // Close standard input (current terminal)
    close(1); // Close standard output (current terminal)
    close(2); // Close standard error (current terminal)

    dup(fds); // PTY becomes standard input (0)
    dup(fds); // PTY becomes standard output (1)
    dup(fds); // PTY becomes standard error (2)


    setsid();
    ioctl(0, TIOCSCTTY,1);
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
