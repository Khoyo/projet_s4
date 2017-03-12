#include "pts.h"

#include <err.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include "packet.h"

#define BUFF_SIZE sizeof(struct oussh_packet)

int socket_fd;
void send_ws_packet(int fd)
{
  struct winsize ws = get_term_size(1);
  struct oussh_packet p;
  p.type = OUSSH_WINDOW_CHANGE;
  p.window_change_packet.ws = ws;
  write(fd, &p, sizeof(p));
}

static void sigwinch_handler(int signum)
{
  send_ws_packet(socket_fd);
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

void sigpipe_handler(int signum){
  tcsetattr(0, TCSANOW, &orig_external_term_settings);
  //err(EXIT_FAILURE, "main : read failed");
  fprintf(stderr, "oussh: sigppipe: exiting\n");
  fflush(stderr);
  exit(0);
}

void init_ws()
{
  static int  did_it = 0;
  if(!did_it)
  {
      send_ws_packet(socket_fd);
      did_it = 1;
  }
}

int main()
{
  signal(SIGPIPE, sigpipe_handler);
  int fd = socket(AF_UNIX, SOCK_STREAM, 0);
  socket_fd = fd;

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

  //write(fd, msg, sizeof(msg));
  setup_external_terminal();
  fd_set fd_in;
  reg_winchange_handler();
  //send_ws_packet(fd);
  //kill(getpid(), SIGWINCH);
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
      struct oussh_packet p;
      ioerr = read(fd, &p, sizeof(p));
      if (ioerr < 0)
      {
        tcsetattr(0, TCSANOW, &orig_external_term_settings);
        //err(EXIT_FAILURE, "main : read failed");
        printf("oussh: can't read socket: exiting\n");
        exit(0);
      }

      if(p.type == OUSSH_IO)
      {
        p.io_packet.payload[p.io_packet.size] = '\0';
        printf("%s", p.io_packet.payload);
        fflush(stdout);
      }
      if(p.type == OUSSH_DISCONNECT)
      {
        tcsetattr(0, TCSANOW, &orig_external_term_settings);
        fprintf(stderr, "oussh: exiting gracefully\n");
        exit(0);
      }
    }
    if(FD_ISSET(0, &fd_in))
    {
      struct oussh_packet p;
      p.type = OUSSH_IO;
      ioerr = read(STDIN_FILENO, p.io_packet.payload, OUSSH_IO_PAYLOAD_SIZE -1);
      if (ioerr < 0) { errx(EXIT_FAILURE, "main : read failed"); }
      p.io_packet.size = ioerr;

      //write(log_fd, buffer, ioerr);
      //fsync(log_fd);
      //ioerr = write(1, "rcv", 4);
      ioerr = write(fd, &p, sizeof(p));
      if (ioerr < 0) { errx(EXIT_FAILURE, "main : write failed"); }
      init_ws();
    }
  }

}
