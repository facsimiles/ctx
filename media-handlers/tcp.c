#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

typedef struct TcpServer {
  int fd;
  int port;
} TcpServer;

int server_listen(TcpServer* server)
{
  int err = (server->fd = socket(AF_INET, SOCK_STREAM, 0));
  if (err == -1) {
    perror("socket");
    printf("Failed to create socket endpoint\n");
    return err;
  }
  struct sockaddr_in server_addr = { 0 };

	// `sockaddr_in` provides ways of representing a full address
	// composed of an IP address and a port.
	//
	// SIN_FAMILY   address family   AF_INET refers to the address
	//                               family related to internet
	//                               addresses
	//
	// S_ADDR       address (ip) in network byte order (big endian)
	// SIN_PORT     port in network byte order (big endian)
	server_addr.sin_family = AF_INET;

        // INADDR_ANY is a special constant that signalizes "ANY IFACE",
        // i.e., 0.0.0.0.
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_port = htons(server->port);

        err = bind(server->fd,
	           (struct sockaddr*)&server_addr,
	           sizeof(server_addr));
	if (err == -1) {
		perror("bind");
		printf("Failed to bind socket to address\n");
		return err;
	}

        err = listen(server->fd, 0);
	if (err == -1) {
		perror("listen");
		printf("Failed to put socket in passive mode\n");
		return err;
	}
  return 0;
}

int server_accept(TcpServer* server)
{
  int err = 0;
  int conn_fd;
  socklen_t client_len;
  struct sockaddr_in client_addr;

  client_len = sizeof(client_addr);

  err =
  (conn_fd = accept(
     server->fd, (struct sockaddr*)&client_addr, &client_len));
  if (err == -1)  {
	perror("accept");
	printf("failed accepting connection\n");
	return err;
  }

  printf("Client connected! %i\n", client_len);

  char buf[64]={0,};
  for (;;)
  {
    memset (buf, 0, sizeof (buf));
    //int count = read (server->fd, buf, sizeof(buf));
    printf (buf);
    //if (count > 0)
    //   write (server->fd, &data, 1);
  }

  err = close(conn_fd);
  if (err == -1)
  {
    perror("close");
    printf("failed to close connection\n");
    return err;
  }
}


int ctx_tcp_main (int argc, char **argv)
{
	TcpServer server = { 0, 2021 };
  for (int i = 1; argv[i]; i++)
  {
    if (!strcmp (argv[i], "-p"))
    {
      if (argv[i+1])
        server.port = atoi (argv[i+1]);
    }
  }


int err = 0;
  fprintf (stderr, "should start server on port %i\n", server.port);

	err = server_listen(&server);
	if (err) {
		printf("Failed to listen on address 0.0.0.0:%d\n", 
                        server.port);
		return err;
	}

	for (;;) {
		err = server_accept(&server);
		if (err) {
			printf("Failed accepting connection\n");
			return err;
		}
	}


  return 0;
}
