#include "pts.h"

void oussh_open_pt(int flags, int* fdm, int* fds)
{
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


struct termios orig_external_term_settings; // Saved terminal settings
void setup_external_terminal()
{
    struct termios new_term_settings; // Current terminal settings

    tcgetattr(0, &orig_external_term_settings);
    new_term_settings = orig_external_term_settings;
    cfmakeraw(&new_term_settings);
    tcsetattr(0, TCSANOW, &new_term_settings);
}

void setup_master_pt(int fd)
{
  (void) fd;
  //As of now, no special setup required
}

void setup_slave_pt(int fd)
{
    //We must not make our pts raw, it MUST be in cooked mode
    /*struct termios term_settings;

    // Save the default parameters of the slave side of the PTY
    ioerr = tcgetattr(fd, &term_settings);

    //cfmakeraw (&new_term_settings);
    tcsetattr(fd, TCSANOW, &term_settings);
    */

    // Make fd our input and output
    close(0); // Close standard input (current terminal)
    close(1); // Close standard output (current terminal)
    close(2); // Close standard error (current terminal)

    dup(fd); // PTY becomes standard input (0)
    dup(fd); // PTY becomes standard output (1)
    dup(fd); // PTY becomes standard error (2)

    setsid(); // Put our process as a session
    ioctl(0, TIOCSCTTY,1); // Set our controlling terminal
}
