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
  int sockfd, rv, nbr_bytes;
  struct addrinfo hints, *servinfo, *p;
  char *port, *hostname, *msg;     /* filled from command line */
  char addr_str[INET6_ADDRSTRLEN]; /* holds address representation string */

  if (argc != 4) {
    fprintf(stderr, "Usage: %s HOSTNAME PORT MESSAGE\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  hostname = argv[1];
  port = argv[2];
  msg = argv[3];

  /* Fill up the addrinfo hints by choosing IPv4 vs. IPv6, UDP vs. TCP and the
   * target IP address. In this case, we are using UDP (datagram) and IPv6. */
  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_INET6;
  hints.ai_socktype = SOCK_DGRAM;

  /* get socket addresses for the provided hints - in this case, node is set to
   * the server (or hostname) that we want to send data to. Remember that this
   * call simply fills up a linked list of addrinfo structs -- it does NOT set
   * anything up! Later, a candidate will be picked from the structs and will
   * be used as arguments to the actual calls that send data. */
  rv = getaddrinfo(hostname, port, &hints, &servinfo);
  if (rv != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
    exit(EXIT_FAILURE);
  }

  /* loop through the linked list returned from getaddrinfo until a socket is
   * successfully created */
  for (p = servinfo; p != NULL; p = p->ai_next) {
    /* this function reads: inet network to presentation. It converts a given
     * address (IPv4 or IPv6) into a string representation */
    inet_ntop(p->ai_family, get_addr_struct(p->ai_addr), addr_str,
              INET6_ADDRSTRLEN);
    /* print current attempted address */
    printf("[server] trying to create a socket for %s:%s ...\n", addr_str,
           port);

    sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
    if (sockfd != -1)
      break; /* success */
  }

  /* free the linked list allocated by previous call to getaddrinfo */
  freeaddrinfo(servinfo);

  /* if p is null this means that we were NOT successful in creating a socket */
  if (p == NULL) {
    fprintf(stderr, "talker: failed to create socket\n");
    exit(EXIT_FAILURE);
  }

  /* send the message -- notice the use of sendto() and NOT send() because this
   * is a connectionless protocol (i.e., we have to specify to whom we are
   * sending the data every time because the protocol does NOT maintain state
   * between packets!) */
  nbr_bytes = sendto(sockfd, msg, strlen(msg), 0, p->ai_addr, p->ai_addrlen);
  if (nbr_bytes == -1) {
    perror("sendto");
    exit(EXIT_FAILURE);
  }

  printf("talker: sent %d bytes to %s\n", nbr_bytes, hostname);

  /* do NOT forget to close the socket */
  if (close(sockfd) == -1) {
    perror("close");
    exit(EXIT_FAILURE);
  }

  exit(EXIT_SUCCESS);
}
