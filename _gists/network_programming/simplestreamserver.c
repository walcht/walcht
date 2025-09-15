#include "sockethelpers.h"
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define PORT "3490" /* this server's port */
#define BACKLOG 10  /* max number of connections to be help in the backlog */

int main(void) {
  int list_sockfd, conn_sockfd; /* listening and connection socket fds */
  int rv;                       /* return value - always check for success */
  struct addrinfo hints, *servinfo, *p; /* hints for getaddrinfo() and servinfo
                                         * is its output */
  int yes = 1;
  char addr_str[INET6_ADDRSTRLEN]; /* holds address representation string */
  struct sockaddr_storage client_addr;
  socklen_t client_addr_len = sizeof(struct sockaddr_storage);
  char data_buf[4096]; /* data buffer to hold received client message */

  /* Fill up the addrinfo hints by choosing IPv4 vs. IPv6, UDP vs. TCP and the
   * target IP address. The AI_PASSIVE flag means "hey, look for this host IPs
   * that can be used to accept connections (i.e., bind() calls) */
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;       /* only IPv4 */
  hints.ai_socktype = SOCK_STREAM; /* TCP */
  hints.ai_flags = AI_PASSIVE;     /* host IPs */

  /* get socket addresses for the provided hints - in this case, since node is
   * NULL and AI_PASSIVE flag is set, the returned sockets are suitable for
   * bind() calls (i.e., suitable for server applications to accept connections
   * or recvieve data using recvfrom) */
  rv = getaddrinfo(NULL, PORT, &hints, &servinfo);
  if (rv != 0) { /* error handling here - use gai_strerror() */
    perror("getaddrinfo");
    exit(EXIT_FAILURE);
  }

  /* loop through the linked list res until you have successfully created the
   * connection socket and bound it */
  for (p = servinfo; p != NULL; p = p->ai_next) {
    /* this function reads: inet network to presentation. It converts a given
     * address (IPv4 or IPv6) into a string representation */
    inet_ntop(p->ai_family, get_addr_struct(p->ai_addr), addr_str,
              INET6_ADDRSTRLEN);

    // print current attempted addr
    printf("[server] trying to open a listening socket at %s:%s ...\n",
           addr_str, PORT);

    // try to create a socket for the current addrinfo candidate
    list_sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);

    // check if socket creation failed
    if (list_sockfd == -1) {
      /* check errno */
      perror("socket");
      printf("[server] opening a listening socket for %s failed\n", addr_str);
      continue;
    }

    /* running this server multiple times, in succession, with small delay
     * can cause the "Address already in use" error. Very briefly, the TCP
     * socket was left in a TIME_WAIT state - which by default could cause
     * an error when a reuse attempt of the socket is made. To "fix" this,
     * we set the REUSEADDR socket layer option (if you really want to get
     * an idea why I put fix between quotes then read this absolutely
     * gorgeous article here:
     *
     * https://vincent.bernat.ch/en/blog/2014-tcp-time-wait-state-linux)
     */
    rv = setsockopt(list_sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
    if (rv == -1) {
      perror("setsockopt");
      exit(EXIT_FAILURE);
    }

    /* try to bind the created socket to this host's IP and a sepcified port
     * p->ai_addr contains this host's IP address and the port */
    rv = bind(list_sockfd, p->ai_addr, p->ai_addrlen);
    if (rv == 0)
      break; /* success */

    // failed to bind (check errno) - do NOT forget to close the socket fd!
    perror("bind");
    close(list_sockfd);
  }

  // check whether we were successful in finding a candidate
  // but we called freeaddrinfo before, you might ask. Well,
  if (p == NULL) { /* failed to find a candidate */
    exit(EXIT_FAILURE);
  }

  printf("[server] successfully opened a listening socket to %s:%s\n", addr_str,
         PORT);

  // remember to call this to avoid a memory leak!
  // freeaddrinfo frees the linked list but does NOT NULL assign the struct's
  // ai_next pointers
  freeaddrinfo(servinfo);

  /* start listening for connection requests (client side programs can now
   * connect to this socket by calling, you guessed it, connect()) */
  rv = listen(list_sockfd, BACKLOG);
  if (rv == -1) { /* error handling here */
    perror("listen");
    exit(EXIT_FAILURE);
  }

  // wait for the client to connect ...
  // or if you don't care about client's addr: accept(list_sockfd, NULL, NULL)
  conn_sockfd =
      accept(list_sockfd, (struct sockaddr *)&client_addr, &client_addr_len);
  if (conn_sockfd == -1) { /* handle error */
    perror("accept");
    exit(EXIT_FAILURE);
  }

  inet_ntop(client_addr.ss_family,
            get_addr_struct((struct sockaddr *)&client_addr), addr_str,
            INET6_ADDRSTRLEN);

  // print so that we know when client connected
  printf("[server] got conncetion from %s:%d\n", addr_str,
         get_port((struct sockaddr *)&client_addr));

  /* in this example we expect only one client to connect - so we no longer need
   * the listening socket */
  close(list_sockfd);

  while (1) {
    printf("[server] waiting for message from client %s\n", addr_str);

    /* wait (i.e., block) until we receive a message from the client or the
     * until the client closes the connection */
    int nbytes = recv(conn_sockfd, data_buf, 4096, 0);

    if (nbytes <= 0) {   /* error or connection closed */
      if (nbytes == 0) { /* connection closed */
        printf("[server] client %s hung up\n", addr_str);
        break;
      } else {
        perror("recv");
      }
    }

    // just to be sure that data_buf string is null terminated
    // telnet adds null termination after hitting \r - but it is always
    // important to be safe :-)
    data_buf[nbytes] = '\0';

    // echo the received message
    printf("[server] received message from client %s:%d\n", addr_str,
           get_port((struct sockaddr *)&client_addr));

    /* now send the data back again to the client
     * send may in fact not send the entirety of your data and it is your
     * reponsibility to keep re-sending until all chunks of your message are
     * sent.*/
    rv = send(conn_sockfd, data_buf, nbytes, 0);
    if (rv == -1) { /* handle error */
      perror("send");
      continue;
    }
  }

  // do NOT forget to close the connection sockfd!
  close(conn_sockfd);
  exit(EXIT_SUCCESS);
}
