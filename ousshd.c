#define _GNU_SOURCE
#define _DEFAULT_SOURCE
#define _XOPEN_SOURCE 600
#include "pts.h"
#include "packet.h"
#include "password.h"
#include "tea.h"

#include <sys/socket.h>
#include <sys/un.h>

#define BUFF_SIZE sizeof(struct oussh_packet)
#define KEY (sample_key)

uint32_t sample_key[4] = {1,2,3,456};

int fdm = -1; // master's file descriptor



int bind_and_listen(char* socket_path)
{
  struct sockaddr_un addr;
  int fd;

  //if (argc > 1) socket_path=argv[1];

  if ( (fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
    perror("socket error");
    exit(-1);
  }

  memset(&addr, 0, sizeof(addr));
  addr.sun_family = AF_UNIX;
  if (*socket_path == '\0') {
    *addr.sun_path = '\0';
    strncpy(addr.sun_path+1, socket_path+1, sizeof(addr.sun_path)-2);
  } else {
    strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path)-1);
    unlink(socket_path);
  }

  if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
    perror("bind error");
    exit(-1);
  }

  if (listen(fd, 5) == -1) {
    perror("listen error");
    exit(-1);
  }
  return fd;
}

char* calc_key_fingerprint(char* filename)
{
  FILE* f = fopen(filename, "r");
  char* key = NULL;
  size_t n = 0;
  getdelim(&key, &n, 0, f);
  char* res = crypt(key, "$5$$");
  fprintf(stderr, "fingerprint sent: %s\n", res);
  free(key);
  return res;
}

void auth_client()
{
  struct oussh_packet p;

  p.type = OUSSH_BANNER;
  
  strncpy(p.banner.fingerprint, 
      calc_key_fingerprint("id_rsa.pub"),
      sizeof(p.banner.fingerprint));
  write(1, &p, sizeof(p));

  read(1, &p, sizeof(p));
  if(p.type != OUSSH_PWD_AUTH)
    err(3, "bad auth packet");

  char* current_hash = pwd_get_hash(p.pwd_auth.username, p.pwd_auth.password);
  if (current_hash == NULL)
  {
    p.type = OUSSH_PWD_REPLY;
    p.pwd_reply.accepted = 0;
    write(1, &p, sizeof(p));
    exit(0);
  }

  char* file_hash = pwd_get_hash_from_file("hash_file", p.pwd_auth.username);
  if (file_hash == NULL)
  {
    p.type = OUSSH_PWD_REPLY;
    p.pwd_reply.accepted = 0;
    write(1, &p, sizeof(p));
    free(current_hash);
    exit(0);
  }
  
  p.type = OUSSH_PWD_REPLY;
  warnx("fh = %s", file_hash);
  warnx("ch = %s", current_hash);
  p.pwd_reply.accepted = pwd_is_str_equals(file_hash, current_hash);
  write(1, &p, sizeof(p));
  free(file_hash);
  free(current_hash);
  if (!p.pwd_reply.accepted)
  {
    fprintf(stderr, "Bad password\n");
    exit(0);
  }
}

void handle_connection()
{
  int log_fd;
  int fds = -1; // slave's file descriptor

  auth_client();
  oussh_open_pt(O_RDWR, &fdm, &fds);

  pid_t pid = fork();
  set_term_size(fdm, get_term_size(0));

  if (pid < 0) {

    errx(EXIT_FAILURE, "main : fork failed");
  }
  else if (pid != 0) {

    // FATHER - MASTER
    ssize_t ioerr = -1;

    setup_master_pt(fdm);
    //reg_winchange_handler();

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
          struct oussh_packet p;
          p.type = OUSSH_IO;
          ioerr = read(fdm, p.io_packet.payload, OUSSH_IO_PAYLOAD_SIZE -1);
          p.io_packet.size = ioerr;

          if (ioerr < 0)
          {
            //tcsetattr(0, TCSANOW, &orig_external_term_settings);
            //err(EXIT_FAILURE, "main : read failed");
            p.type = OUSSH_DISCONNECT;
            write_crypted_packet(1, p, KEY);
            fprintf(stderr, "ousshd: Exiting\n");
            close(0);
            close(1);
            close(fdm);
            exit(0);
          }

          write_crypted_packet(1, p, KEY);
        }
        if(FD_ISSET(0, &fd_in))
        {
          struct oussh_packet p;
          ioerr = read_crypted_packet(STDIN_FILENO, &p, KEY);
          if (ioerr < 0) { errx(EXIT_FAILURE, "main : read failed"); }

          if(p.type == OUSSH_IO)
          {
            write(log_fd, p.io_packet.payload, p.io_packet.size);
            fsync(log_fd);
            ioerr = write(fdm, p.io_packet.payload, p.io_packet.size);
            if (ioerr < 0) { errx(EXIT_FAILURE, "main : write failed"); }
          }
          if(p.type == OUSSH_WINDOW_CHANGE)
          {
            struct winsize ws = p.window_change_packet.ws;
            fprintf(stderr, "Receive WS packet, %hu, %hu\n", ws.ws_row, ws.ws_col);
            set_term_size(fdm, p.window_change_packet.ws);
            ioctl(fdm, TIOCSWINSZ, &ws);
            //kill(getpid(), SIGWINCH);
            //kill(pid, SIGWINCH);
          }
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

    execlp("bash", "bash", NULL);

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

  exit(0);

}

int main() {

  int listen_sockfd = bind_and_listen("/tmp/oussh.sock");

  while(1)
  {
    int conn_fd = accept(listen_sockfd, NULL, NULL);
    if(conn_fd == -1)
      err(5,"couldn't accept connection");

    if(fork() == 0)
    {
      close(0);
      close(1);
      //close(2);
      dup(conn_fd);
      dup(conn_fd);
      //dup(conn_fd);
      close(conn_fd);
      handle_connection();
    }
    else
      close(conn_fd);

  }

}
