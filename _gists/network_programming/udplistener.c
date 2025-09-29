#include "sockethelpers.h"
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
  int sockfd, rv;
  char *port;               /* filled from command line */
  uint64_t maxbuflen = 256; /* filled from command line */
  uint64_t nbr_bytes = 0;   /* number of received bytes from a client */
  struct addrinfo hints, *servinfo, *p;
  char addr_str[INET6_ADDRSTRLEN];
  struct sockaddr_storage their_addr;     /* holds address of a client */
  socklen_t addr_len = sizeof their_addr; /* inout parameter for recvfrom */
  char buf[maxbuflen];

  if (argc != 3) {
    fprintf(stderr, "Usage: %s PORT MAXBUFLEN\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  port = argv[1];
  maxbuflen = strtol(argv[2], NULL, 10);

  /* The AI_PASSIVE flag means "hey, look for this host's IPs that can
   * be used to accept connection (i.e., bind() calls). This is the usual
   * pattern for IPv6 servers (for IPv4 all you need to do is change the
   * ai_family to AF_INET -- how utterly complex!) */
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET6;     /* only IPv6 */
  hints.ai_socktype = SOCK_DGRAM; /* UDP */
  hints.ai_flags = AI_PASSIVE;    /* 'our' IPs (or interfaces) */

  /* get socket addresses for the provided hints - in this case, since node is
   * NULL and AI_PASSIVE flag is set, the returned sockets are suitable for
   * bind() calls (i.e., suitable for server applications to accept connections
   * or recvieve data using recvfrom) */
  rv = getaddrinfo(NULL, port, &hints, &servinfo);
  if (rv != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
    exit(EXIT_FAILURE);
  }

  /* loop through the returned host addresses from getaddrinfo() and open a
   * socket for the first valid one */
  for (p = servinfo; p != NULL; p = p->ai_next) {
    /* try to create a blocking socket */
    sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
    if (sockfd == -1) {
      continue;
    }

    rv = bind(sockfd, p->ai_addr, p->ai_addrlen);
    if (rv == 0)
      break; /* success */

    close(sockfd);
  }

  /* remember to call this to avoid a memory leak! */
  freeaddrinfo(servinfo);

  /* freeaddrinfo does not NULL assign the ai_next pointers in the address
   * structures */
  if (p == NULL) {
    fprintf(stderr, "[listener] failed to bind socket\n");
    exit(2);
  }

  printf("[listener] waiting to recvfrom ...\n");

  /* Receive data from a client (any client that calls sendto() to 'us'). Here
   * we are using recvfrom() rather than receive() because this is a
   * connectionless protocol and we can provide an addr and addr_len output
   * arguments for recvfrom() to fill with the client's address and its length,
   * respectively. Also important to note that this socket is blocking - this
   * will block (i.e., sleep) until data is available for reading */
  nbr_bytes = recvfrom(sockfd, buf, maxbuflen - 1, 0,

                       (struct sockaddr *)&their_addr, &addr_len);
  if (nbr_bytes == -1) {
    perror("recvfrom");
    exit(1);
  }

  printf("[listener] got packet from %s\n",
         inet_ntop(their_addr.ss_family,
                   get_addr_struct((struct sockaddr *)&their_addr), addr_str,
                   sizeof addr_str));

  printf("[listener] packet is %lu bytes long\n", nbr_bytes);

  /* be absolutely sure to null terminate the received string */
  buf[nbr_bytes] = '\0';

  printf("[listener] packet contains '%s'\n", buf);

  if (close(sockfd) == -1) {
    perror("close");
    exit(EXIT_FAILURE);
  }

  exit(EXIT_SUCCESS);
}
