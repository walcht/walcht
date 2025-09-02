#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>

int main(int argc, char *argv[]) {
  struct addrinfo hints, *res, *p;
  int status;
  char ipstr[INET6_ADDRSTRLEN];

  if (argc != 2) {
    fprintf(stderr, "usage: showipaddrs hostname\n");
    return 1;
  }

  // make sure hints is null initialized
  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;     // can be either IPv4 or IPv6
  hints.ai_socktype = SOCK_STREAM; // TCP

  // why is the port NULL?
  if ((status = getaddrinfo(argv[1], NULL, &hints, &res)) != 0) {
    fprintf(stderr, "getaddrinfor: %s\n", gai_strerror(status));
    return 2;
  }

  printf("IP addresses for %s:\n\n", argv[1]);

  // getaddrinfo returns a linked list of addrinfo structs. Why?
  // well, the domain name resolution might resolve to different IP addresses
  // (e.g., one for IPv4 and one for IPv6)
  for (p = res; p != NULL; p = p->ai_next) {
    void *addr; /* pointer to the ipv4/ipv6 address struct */
    char *ipver;
    struct sockaddr_in *ipv4;
    struct sockaddr_in6 *ipv6;

    // the p->ai_addr is a different struct depending on the type of the IP
    // (IPv4 vs. IPv6)
    if (p->ai_family == AF_INET) { // IPv4
      ipv4 = (struct sockaddr_in *)p->ai_addr;
      addr = &(ipv4->sin_addr);
      ipver = "IPv4";
    } else { // IPv6
      ipv6 = (struct sockaddr_in6 *)p->ai_addr;
      addr = &(ipv6->sin6_addr);
      ipver = "IPv6";
    }

    // (n)etwork (to) (p)resentation - convert the IP struct pointer to a string
    // reprensentation
    inet_ntop(p->ai_family, addr, ipstr, sizeof ipstr);
    printf("  %s: %s\n", ipver, ipstr);
  }

  freeaddrinfo(res);
  return 0;
}
