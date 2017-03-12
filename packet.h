
#define OUSSH_IO_PAYLOAD_SIZE 256

enum oush_packet_type
{
  OUSSH_IO,
  OUSSH_WINDOWS_CHANGE
}

struct oussh_packet
{
  enum oush_packet_type type;
  struct oush_packet_io
  {
    size_t size;
    char payload[OUSSH_IO_PAYLOAD_SIZE];
  } io_packet;
  struct oush_windows_change_packet
  {
    struct winsize ws;
  } windows_change_packet;

}
