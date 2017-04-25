
#define OUSSH_IO_PAYLOAD_SIZE 1024

enum oussh_packet_type
{
  OUSSH_BANNER,
  OUSSH_IO,
  OUSSH_WINDOW_CHANGE,
  OUSSH_DISCONNECT,
  OUSSH_PWD_AUTH,
  OUSSH_PWD_REPLY
};

struct oussh_packet
{
  enum oussh_packet_type type;
  union {
    struct oush_packet_banner
    {
      char fingerprint[50];
    } banner;
    struct oush_packet_io
    {
      size_t size;
      char payload[OUSSH_IO_PAYLOAD_SIZE];
    } io_packet;
    struct oussh_windows_change_packet
    {
      struct winsize ws;
    } window_change_packet;
    struct oussh_pwd_auth {
      char username[17];
      char password[100];
    } pwd_auth;
    struct oussh_pwd_reply {
      int accepted;
    } pwd_reply;
  };
};
