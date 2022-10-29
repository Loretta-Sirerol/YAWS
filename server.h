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

struct server *init ();

static const char *get_content_type (const char *);

static int create_socket (void);

static struct client *get_client (struct client **, int);

static void log_connections (struct client *);

static void drop_client (struct client **, struct client *);

static const char *get_client_address (struct client *);

static fd_set wait_for_client (struct client **, int);

/* The server cannot or will not process the request due to something that is
 * perceived to be a client error */
static void send400 (struct client **, struct client *);

/* The server can not find the requested resource. In the browser, this means
 * the URL is not recognized */
static void send404 (struct client **, struct client *);

static void serve (struct client **, struct client *, const char *);

#endif
