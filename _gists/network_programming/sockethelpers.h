#ifndef SOCKET_HELPERS_H
#define SOCKET_HELPERS_H

#include <netinet/in.h>
#include <sys/socket.h>

// returns sockaddr struct from provided sockaddr in IPv4 or IPv6.
static inline void *get_addr_struct(const struct sockaddr *sa) {
  return sa->sa_family == AF_INET
             ? (void *)(&(((struct sockaddr_in *)sa)->sin_addr))
             : (void *)(&(((struct sockaddr_in6 *)sa)->sin6_addr));
}

// returns port from given sockaddr in IPv4 or IPv6.
static inline in_port_t get_port(const struct sockaddr *sa) {
  return sa->sa_family == AF_INET ? ((struct sockaddr_in *)sa)->sin_port
                                  : ((struct sockaddr_in6 *)sa)->sin6_port;
}

/* returns listening socket file descriptor on success and -1 on failure */
int create_listening_socket(const char *port, int backlog);

#endif
