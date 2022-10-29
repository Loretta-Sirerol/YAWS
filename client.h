#ifndef CLIENT_H
#define CLIENT_H

#include <sys/socket.h>

struct client
{
  socklen_t address_length;
  struct sockaddr_storage address;
  char address_buffer[128];
  int socket;
  char request[2048];		// HTTP Request with a maximum size of 2 MiB
  int received;
  struct client *next;
};

#endif
