#include "server.h"
#include "client.h"

struct server *
init ()
{
  struct server *tmp = (struct server *) calloc (1, sizeof (struct server));
  tmp->start = &create_socket;
  tmp->get_client = &get_client;
  tmp->log_connections = &log_connections;
  tmp->drop_client = &drop_client;
  tmp->get_client_address = get_client_address;
  tmp->wait_for_client = wait_for_client;
  tmp->send400 = &send400;
  tmp->send404 = &send404;
  tmp->serve = &serve;
  return tmp;
}


const char *
get_content_type (const char *path)
{
  char *file_extension = strrchr (path, '.');
  char *delimiter = "    ";
  static char mime_type[128] = "application/octet-stream";
  char line[128];
  char *token;
  file_extension++;
  FILE *mime_types_ptr = fopen ("mime_types", "rb");
  if (mime_types_ptr != (void *) 0)
    {
      while (fgets (line, sizeof (line), mime_types_ptr) != (void *) 0)
	{
	  token = strtok (line, delimiter);
	  if (token != (void *) 0)
	    {
	      if (strcmp (token, file_extension) == 0)
		{
		  token = strtok (NULL, delimiter);
		  strcpy (mime_type, token);
		  mime_type[strcspn (mime_type, "\r\n")] = 0;
		  break;
		}
	    }
	}
    }
  fclose (mime_types_ptr);
  return mime_type;
}

int
create_socket (void)
{
  fprintf (stdout, "Configuring local address... \n");
  struct addrinfo hints;
  memset (&hints, 0, sizeof (hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;

  struct addrinfo *bind_address;
  (void) getaddrinfo (0, "3000", &hints, &bind_address);

  fprintf (stdout, "Creating socket... \n");
  int socket_listen;
  socket_listen = socket (bind_address->ai_family, bind_address->ai_socktype,
			  bind_address->ai_protocol);

  if (socket_listen < 0)
    {
      fprintf (stderr, "socket() failed with error number %d \n", errno);
      fprintf (stdout, "%s\n", strerror (errno));
      return errno;
    }

  /* This pice of code allows us to restart the server without having to wait
   * TIME_WAIT seconds */
  /* I.E. Prevents those dreaded "Address already in use" \ code 98 errors */
  fprintf (stdout, "Setting socket options \n");
  if (setsockopt (socket_listen, SOL_SOCKET, SO_REUSEADDR, &(int)
		  { 1 },
		  sizeof (int)) < 0)
    {
      fprintf (stderr, "setsockopt() failed with error number %d\n", errno);
      fprintf (stdout, "%s\n", strerror (errno));
      return errno;
    }

  fprintf (stdout, "Binding socket to local address... \n");
  if (bind (socket_listen, bind_address->ai_addr, bind_address->ai_addrlen))
    {
      fprintf (stderr, "bind() failed with error number %d \n", errno);
      fprintf (stdout, "%s\n", strerror (errno));
      return errno;
    }

  freeaddrinfo (bind_address);

  fprintf (stdout, "Listening... \n");
  if (listen (socket_listen, 10) < 0)
    {
      fprintf (stderr, "listen() failed with error number %d", errno);
      fprintf (stdout, "%s\n", strerror (errno));
      exit (1);
    }

  return socket_listen;
}

struct client *
get_client (struct client **client_list, int socket)
{

  struct client *client_information = *client_list;

  while (client_information)
    {
      if (client_information->socket == socket)
	{
	  break;
	}
      client_information = client_information->next;
    }

  if (client_information)
    {
      return client_information;
    }

  struct client *new_client = calloc (1, sizeof (struct client));

  if (new_client == (void *) 0)
    {
      fprintf (stderr,
	       "Failed to allocate enough memory for a new client...");
      exit (1);
    }

  new_client->address_length = sizeof (new_client->address);

  new_client->next = *client_list;

  *client_list = new_client;

  return new_client;
}

void
log_connections (struct client *target_client)
{
  fprintf (stdout, "New connection from: %s\n",
	   get_client_address (target_client));
}

void
drop_client (struct client **client_list, struct client *target_client)
{
  close (target_client->socket);

  struct client **temp = client_list;
  while (*temp)
    {
      if (*temp == target_client)
	{
	  *temp = target_client->next;
	  free (target_client);
	  return;
	}
      temp = &(*temp)->next;
    }
  fprintf (stderr, "No such client...");
  exit (1);
}

const char *
get_client_address (struct client *client)
{
  getnameinfo ((struct sockaddr * restrict) &client->address,
	       client->address_length, client->address_buffer,
	       sizeof (client->address_buffer), 0, 0, NI_NUMERICHOST);
  return client->address_buffer;
}

fd_set
wait_for_client (struct client **client_list, int server)
{
  fd_set read;
  FD_ZERO (&read);
  FD_SET (server, &read);
  int m_socket = server;

  struct client *client_information = *client_list;

  while (client_information != (void *) 0)
    {
      FD_SET (client_information->socket, &read);
      if (client_information->socket > m_socket)
	{
	  m_socket = client_information->socket;
	}
      client_information = client_information->next;
    }

  if (select (m_socket + 1, &read, 0, 0, 0) < 0)
    {
      fprintf (stderr, "select() failed with error number %d \n", errno);
      fprintf (stdout, "%s\n", strerror (errno));
      exit (1);
    }

  return read;
}

void
send400 (struct client **client_list, struct client *target_client)
{
  const char *response = "HTTP/1.1 400 Bad Request\r\n"
    "Connection: close\r\n" "Content-Length: 11\r\n\r\nBad Request";
  send (target_client->socket, response, strlen (response), 0);
  drop_client (client_list, target_client);
}

void
send404 (struct client **client_list, struct client *target_client)
{
  const char *response = "HTTP/1.1 404 Not Found\r\n"
    "Connection: close\r\n" "Content-Length: 9\r\n\r\nNot Found";
  send (target_client->socket, response, strlen (response), 0);
  drop_client (client_list, target_client);
}

void
serve (struct client **client_list, struct client *target_client,
       const char *path_to_resource)
{
  fprintf (stdout, "serving to client %s the resource %s\n",
	   get_client_address (target_client), path_to_resource);
  if (strcmp (path_to_resource, "/") == 0)
    {
      path_to_resource = "/index.html";
    }
  else if (strlen (path_to_resource) > 100)
    {
      send400 (client_list, target_client);
      return;
    }
  else if (strstr (path_to_resource, "..") || strstr (path_to_resource, "~")
	   || strstr (path_to_resource, "$HOME$"))
    {
      send404 (client_list, target_client);
      return;
    }

  char absolute_path[128];
  sprintf (absolute_path, "public%s", path_to_resource);

  FILE *file = fopen (absolute_path, "rb");

  if (file == (void *) 0)
    {
      send404 (client_list, target_client);
      fprintf (stdout, "%s\n", strerror (errno));
      return;
    }

  fseek (file, 0L, SEEK_END);
  size_t cl = ftell (file);
  rewind (file);

  const char *mime_type = get_content_type (absolute_path);

  char buffer[1024];

  sprintf (buffer, "HTTP/1.1 200 OK\r\n");
  send (target_client->socket, buffer, strlen (buffer), 0);

  sprintf (buffer, "Connection: close\r\n");
  send (target_client->socket, buffer, strlen (buffer), 0);

  sprintf (buffer, "Content-Length: %zu\r\n", cl);
  send (target_client->socket, buffer, strlen (buffer), 0);

  sprintf (buffer, "Content-Type: %s\r\n", mime_type);
  send (target_client->socket, buffer, strlen (buffer), 0);

  sprintf (buffer, "\r\n");
  send (target_client->socket, buffer, strlen (buffer), 0);

  int r = fread (buffer, 1, 1024, file);
  while (r)
    {
      send (target_client->socket, buffer, r, 0);
      r = fread (buffer, 1, 1024, file);
    }

  fclose (file);
  drop_client (client_list, target_client);
}
