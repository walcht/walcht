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

#define MAXDATASIZE 4096 // max number of bytes we can get/send at once

int main(int argc, char *argv[]) {
  int sockfd; /* socket file descriptor used to send/receive data */
  struct addrinfo hints, *servinfo, *p;
  int num_bytes;                   /* number of bytes sent/received */
  char addr_str[INET6_ADDRSTRLEN]; /* holds IP address string representation */
  char buf[MAXDATASIZE];
  int rv; /* holds return value of system calls - ALWAYS check for errors! */

  if (argc != 3) {
    fprintf(stderr, "usage: simplestreamclient HOSTNAME PORT\n");
    exit(1);
  }

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  /* get socket address info structures that we can connect to for the provided
   * host IP and service (i.e., port). Each returned address info structure
   * represents a potential socket that we can connect to (using connect()). */
  if ((rv = getaddrinfo(argv[1], argv[2], &hints, &servinfo)) != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
    exit(EXIT_FAILURE);
  }

  /* loop through the linked list res until we have successfully created the
   * socket and connected it */
  for (p = servinfo; p != NULL; p = p->ai_next) {
    if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
      perror("[client] socket");
      continue;
    }

    /* this function reads: inet network to presentation. It converts a given
     * address (IPv4 or IPv6) into a string representation */
    inet_ntop(p->ai_family, get_addr_struct((struct sockaddr *)p->ai_addr),
              addr_str, sizeof addr_str);

    printf("[client] attempting connection to %s ...\n", addr_str);
    rv = connect(sockfd, p->ai_addr, p->ai_addrlen);
    if (rv == 0)
      break; /* success */

    perror("[client] connect");
    close(sockfd);

  } // END FOR LOOP

  if (p == NULL) {
    fprintf(stderr, "[client] failed to connect\n");
    exit(EXIT_FAILURE);
  }

  inet_ntop(p->ai_family, get_addr_struct((struct sockaddr *)p->ai_addr),
            addr_str, sizeof addr_str);
  printf("[client] connected to %s\n", addr_str);

  /* remember to call this to avoid a memory leak! freeaddrinfo frees the
   * linked list but does NOT NULL assign the struct's ai_next pointers */
  freeaddrinfo(servinfo);

  /* prompts the user for a message to send to the server */
  printf("what to send to the server?\n");
  fgets(buf, sizeof(buf), stdin);
  long msg_len = strlen(buf);
  /* remove newline character */
  if (buf[msg_len - 1] == '\n') {
    buf[msg_len - 1] = '\0';
    --msg_len;
  }

  /* we send the prompted message to the server */
  num_bytes = send(sockfd, buf, msg_len, 0);
  if (num_bytes == -1) {
    perror("send");
    exit(EXIT_FAILURE);
  }

  /* print the just-sent message */
  buf[num_bytes] = '\0'; // send() could have sent less than what is requested
  printf("[client] sent '%s'\n", buf);

  /* wait (i.e., block) until we receive a message from the server or the
   * until the server closes the connection */
  if ((num_bytes = recv(sockfd, buf, sizeof(buf) - 1, 0)) == -1) {
    perror("recv");
    exit(EXIT_FAILURE);
  }

  // be absolutely sure that received string from server is null terminated
  buf[num_bytes] = '\0';
  printf("[client] received '%s'\n", buf);

  // do NOT forget to close the connected sockfd!
  close(sockfd);

  exit(EXIT_SUCCESS);
}
