#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "client.h"
#include "server.h"

int
main (void)
{
  struct server *srv = init ();
  srv->server_socket = srv->start ();
  struct client *client_list = 0;
  while (1)
    {
      fd_set read;
      read = srv->wait_for_client (&client_list, srv->server_socket);

      if (FD_ISSET (srv->server_socket, &read))
	{
	  struct client *client = srv->get_client (&client_list, -1);
	  client->socket = accept (srv->server_socket,
				   (struct sockaddr *
				    ) &(client->address),
				   (socklen_t *
				    ) &(client->address_length));
	  if (client->socket < 0)
	    {
	      fprintf (stderr, "accept() failed with error number %d \n",
		       errno);
	      fprintf (stdout, "%s\n", strerror (errno));
	      return errno;
	    }
	  srv->log_connections (client);
	}
      struct client *client = client_list;
      while (client)
	{
	  struct client *new_client = client->next;
	  if (FD_ISSET (client->socket, &read))
	    {
	      if (2048 == client->received)
		{
		  srv->send400 (&client_list, client);
		  client = new_client;
		  continue;
		}
	      int r =
		recv (client->socket, client->request + client->received,
		      2048 - client->received, 0);
	      if (r < 1)
		{
		  fprintf (stderr, "Unexpected disconnect from %s\n",
			   srv->get_client_address (client));
		  srv->drop_client (&client_list, client);
		}
	      else
		{
		  /* HTTP HEADER PARSER */
		  client->received += r;
		  client->request[client->received] = 0;
		  char *q = strstr (client->request, "\r\n\r\n");
		  if (q)
		    {
		      *q = 0;
		      if (strncmp ("GET /", client->request, 5))
			{
			  srv->send400 (&client_list, client);
			}
		      else
			{
			  char *path = client->request + 4;
			  char *end_path = strstr (path, " ");
			  if (!end_path)
			    {
			      srv->send400 (&client_list, client);
			    }
			  else
			    {
			      *end_path = 0;
			      srv->serve (&client_list, client, path);
			    }
			}
		    }
		}
	    }
	  client = new_client;
	}
    }
  fprintf (stdout, "Shuting down server NOW...\n");
  close (srv->server_socket);
  return 0;
}
