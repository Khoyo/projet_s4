#define _DEFAULT_SOURCE
#define _XOPEN_SOURCE 600
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>

extern struct termios orig_external_term_settings; // Saved terminal settings

void oussh_open_pt(int flags, int* fdm, int* fds);

void setup_master_pt(int fd);
void setup_slave_pt(int fd);
void setup_external_terminal();

static inline struct winsize get_term_size(int fd)
{
  struct winsize ws;
  ioctl(fd, TIOCGWINSZ, &ws);
  return ws;
}
static inline void set_term_size(int fd, struct winsize ws)
{
  ioctl(fd, TIOCSWINSZ, &ws);
}
