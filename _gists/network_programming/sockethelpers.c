#include "sockethelpers.h"
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

/* Creates a listening socket that can be used to accept connection requests.
 * Returns the created listening socket fd. -1 on error. */
int create_listening_socket(const char *port, int backlog) {
  int sfd;
  int rv;
  struct addrinfo hints, *servinfo, *p;

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;

  if (getaddrinfo(NULL, port, &hints, &servinfo) != 0) {
    perror("getaddrinfo");
    return -1;
  }

  for (p = servinfo; p != NULL; p = p->ai_next) {
    sfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
    if (sfd == -1)
      continue;

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
    int y = 1;
    if (setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &y, sizeof(int)) == -1) {
      close(sfd);
      perror("setsockopt");
      continue;
    }

    if (bind(sfd, p->ai_addr, p->ai_addrlen) == 0)
      break; /* success */

    close(sfd);
    perror("bind");
  }

  freeaddrinfo(servinfo);

  if (p == NULL) {
    return -1;
  }

  if (listen(sfd, backlog) == -1) {
    close(sfd);
    perror("listen");
    return -1;
  }

  return sfd;
}
