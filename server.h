#ifndef SERVER_H
#define SERVER_H

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct server
{
  int server_socket;
  int (*start) (void);
  struct client *(*get_client) (struct client **, int);
  void (*log_connections) (struct client *);
  void (*drop_client) (struct client **, struct client *);
  const char *(*get_client_address) (struct client *);
  fd_set (*wait_for_client) (struct client **, int);
  void (*send400) (struct client **, struct client *);
  void (*send404) (struct client **, struct client *);
  void (*serve) (struct client **, struct client *, const char *);
};

struct server *init (void);

#endif
