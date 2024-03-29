#define _GNU_SOURCE
#define _DEFAULT_SOURCE
#define _XOPEN_SOURCE 600
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
#include "tea.h" 

#define BUFF_SIZE sizeof(struct oussh_packet)
#define KEY (sample_key)

uint32_t sample_key[4] = {1,2,3,456};

int socket_fd;
void send_ws_packet(int fd)
{
  struct winsize ws = get_term_size(1);
  struct oussh_packet p;
  p.type = OUSSH_WINDOW_CHANGE;
  p.window_change_packet.ws = ws;
  write_crypted_packet(fd, p, KEY);
}

static void sigwinch_handler(int signum)
{
  (void)signum;
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
  (void)signum;
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

void try_auth(int fd)
{
  char username[17];

  struct oussh_packet packet;

  read(fd, &packet, sizeof(packet));
  printf("Server fingerprint is %s\n", packet.banner.fingerprint + 4);

  packet.type = OUSSH_PWD_AUTH;

  printf("username: ");
  fgets(packet.pwd_auth.username, 17, stdin);
  if (username == NULL)
  {
    err(3, "scanf failed for username");
  }
  char* p = strchr(packet.pwd_auth.username, '\n');
  if (p != NULL)
  {
    *p = '\0';
  }

  char* password = getpass("password: ");
  if (password == NULL)
  {
    err(3, "scanf failed for password");
  }
  strncpy(packet.pwd_auth.password, password, 100);
  
  p = strchr(packet.pwd_auth.password, '\n');
  if (p != NULL)
  {
    *p = '\0';
  }
  // Send the packet
  write(fd, &packet, sizeof(packet));
  // Read the reply back
  read(fd, &packet, sizeof(packet));
  if (packet.type != OUSSH_PWD_REPLY)
  {
    errx(1, "bad packet received");
  }

  if (!packet.pwd_reply.accepted)
    errx(1, "authentification failed");
  
}

int main()
{
  signal(SIGPIPE, sigpipe_handler);
  int fd = socket(AF_UNIX, SOCK_STREAM, 0);
  socket_fd = fd;

  struct sockaddr_un addr;

  char socket_path[] = "/tmp/oussh.sock";

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

  try_auth(fd);
  //write(fd, msg, sizeof(msg));
  setup_external_terminal();
  fd_set fd_in;
  reg_winchange_handler();
  //send_ws_packet(fd);
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
      ioerr = read_crypted_packet(fd, &p, KEY);
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
        p.io_packet.size = 0;
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
      ioerr = write_crypted_packet(fd, p, KEY);
      if (ioerr < 0) { errx(EXIT_FAILURE, "main : write failed"); }
      init_ws();
    }
  }

}
