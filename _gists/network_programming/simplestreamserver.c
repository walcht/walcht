/*
 * run this in some terminal
 * in another terminal run:
 *  telnet hostmachinename 3490
 *
 * where hostmachinename is the name of the machine you are running on
 *
 */
#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define PORT "3490"
#define BACKLOG 1 // max nbr pending connections to be help

void sigchld_handler(int s) {
  (void)s; // so that compiler doesn't bitch about unused variable warning
  // save errno because it might be overwritten by waitpid()
  int saved_errno = errno;
  while (waitpid(-1, NULL, WNOHANG) > 0)
    ;
  errno = saved_errno;
}

// get sockaddr struct: IPv4 or IPv6.
void *get_in_addr(struct sockaddr *sa) {
  if (sa->sa_family == AF_INET) {
    return &(((struct sockaddr_in *)sa)->sin_addr);
  }
  return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

int main(void) {
  int sockfd;
  struct addrinfo hints, *servinfo;
  int yes = 1;

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_INET;       // IPv4
  hints.ai_socktype = SOCK_STREAM; // TCP
  hints.ai_flags = AI_PASSIVE;     // use my IP

  int rv; // return value
  if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
    return 1;
  }

  // loop through all the results and bind to the first one possible
  struct addrinfo *p;
  for (p = servinfo; p != NULL; p = p->ai_next) {
    if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
      perror("server: socket");
      continue;
    }

    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
      perror("setsockopt");
      exit(1);
    }

    if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
      close(sockfd);
      perror("server: bind");
      continue;
    }

    // success: found a candidate
    break;
  }

  // check if we failed in finding a candidate
  if (p == NULL) {
    fprintf(stderr, "server: failed to bind\n");
    exit(1);
  }

  // free the servinfo struct filled by getaddrinfo
  freeaddrinfo(servinfo);

  if (listen(sockfd, BACKLOG) == -1) {
    perror("listen");
    exit(1);
  }

  struct sigaction sa;
  sa.sa_handler = sigchld_handler;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = SA_RESTART;
  if (sigaction(SIGCHLD, &sa, NULL) == -1) {
    perror("sigaction");
    exit(1);
  }

  printf("server: waiting for connections ...\n");

  struct sockaddr_storage their_addr; // connector's address
  socklen_t sin_size;
  int new_fd;
  char addr_str[INET6_ADDRSTRLEN];
  while (1) {
    sin_size = sizeof their_addr;
    new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
    if (new_fd == -1) {
      perror("accept");
      continue;
    }

    inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr),
              addr_str, sizeof addr_str);

    printf("server: got conncetion from %s\n", addr_str);

    if (!fork()) {   // child-process-only code
      close(sockfd); // child doesn't need the listening socket
      if (send(new_fd, "Hello, world!", 13, 0) == -1) {
        perror("send");
      }
      close(new_fd); // no longer need the connection socket
      exit(0);
    }

    // parent-process-only code
    close(new_fd); // no longer need the connection socket
  } // END WHILE

  return 0;
}
