#include "sockethelpers.h"
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define MAX_NBR_CLIENT 16384
#define MAX_CLIENT_MSG_LENGTH 256

/* Sends a message to all recipients in fds except to the listening and
 * except_fd sockets. */
void broadcast_msg(const int *fds, int fds_count, const char *buf,
                   uint64_t buf_len, int list_fd, int except_fd) {
  for (int i = 0; i < fds_count; ++i) { /* send the message to all others */
    int dest_fd = fds[i];
    if (dest_fd != list_fd &&
        dest_fd != except_fd /* without this an infinite loop occurs */) {
      if (send(dest_fd, buf, buf_len, 0) == -1) {
        perror("send");
      }
    }
  }
  /* and print the sent message to this server's stdout */
  printf("%s", buf);
}

/* Adds provided fd to the fds list, monitors it via the epoll instance epfd,
 * and increments fds_count. */
int add_to_fds(int epfd, int *fds, uint64_t *fds_count, int fd) {
  struct epoll_event ev;
  int idx = *fds_count;

  fds[idx] = fd;

  ev.events = EPOLLIN; /* data is available for reading */
  ev.data.u64 = idx;   /* set custom user field to the index so that we can
                          retrieve the fd later on */
  if (epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev) == -1) {
    perror("epoll_ctl");
    return -1;
  }

  ++(*fds_count);
  return 0;
}

/* Deletes provided fd from the fds list, closes it, un-monitors it from the
 * the epoll instance epfd, and decrements fds_count. */
int del_fr_fds(int epfd, int *fds, uint64_t *fds_count, uint64_t idx) {
  struct epoll_event ev;
  int fd = fds[idx];

  /* not necessarily needed (read questions in man epoll) - stop monitoring */
  if (epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL) == -1) {
    perror("epoll_ctl");
    return -1;
  }

  /* close the socket */
  if (close(fd) == -1) {
    perror("close");
    return -1;
  }

  --(*fds_count);

  /* if this is possitioned at the end of the array => do nothing */
  if (idx == *fds_count) {
    return 0;
  }

  fds[idx] = fds[*fds_count]; /* assign prev last element to this elm index */

  /* call epoll_ctl to update the custom user data - i.e., the new index */
  ev.events = EPOLLIN;
  ev.data.u64 = idx;
  if (epoll_ctl(epfd, EPOLL_CTL_MOD, fds[idx], &ev) == -1) {
    perror("epoll_ctl");
    return -1;
  }

  return 0;
}

/* Handles a new connection by calling accept, performing error checks, and
 * adding the new connected socket (client) to fds. */
void handle_new_connection(int epfd, int *fds, uint64_t *fds_count,
                           int list_sockfd) {
  struct sockaddr_storage remoteaddr;
  socklen_t addrlen;
  int newfd;
  char remoteIP[INET6_ADDRSTRLEN];

  addrlen = sizeof remoteaddr;
  newfd = accept(list_sockfd, (struct sockaddr *)&remoteaddr, &addrlen);
  if (newfd == -1) {
    perror("accept");
  } else {
    add_to_fds(epfd, fds, fds_count, newfd);
    /* broadcast to all clients (except the new client itself) that a new user
     * has joined the chat room */
    char msg_buf[256];
    snprintf(msg_buf, sizeof(msg_buf), "user %d joined the chat room\n", newfd);
    broadcast_msg(fds, *fds_count, msg_buf, strlen(msg_buf) + 1, list_sockfd,
                  newfd);
  }
}

/* Handles the client data which amounts to either receiving a message and
 * broadcasting it to other clients or hang up in which case the client's socket
 * fd is closed, removed from the fds list and fd_count is decremented.
 */
void handle_client_data(int epfd, int *fds, uint64_t *fds_count,
                        int list_sockfd, uint64_t idx) {
  char buf[MAX_CLIENT_MSG_LENGTH];
  int sender_fd = fds[idx];
  int nbytes = recv(sender_fd, buf, sizeof(buf), 0);

  if (nbytes <= 0) {   /* error or connection closed */
    if (nbytes != 0) { /* error */
      perror("recv");
    }

    del_fr_fds(epfd, fds, fds_count, idx);

    /* broadcast to all clients that this user disconnected */
    char msg_buf[256];
    snprintf(msg_buf, sizeof(msg_buf), "user %d disconnected\n", sender_fd);
    broadcast_msg(fds, *fds_count, msg_buf, strlen(msg_buf) + 1, list_sockfd,
                  -1);
    return;
  }

  /* since this is a messaging up - expected data is a set of messages (i.e.,
   * null terminated char buffers - thus you have to absolutely sure to set
   * the null termination character! */
  buf[nbytes] = '\0';

  /* broadcast what this user sent to all other clients */
  char msg_buf[32 + MAX_CLIENT_MSG_LENGTH]; /* "user %d: " max length is
                                               assmumed to be <= 32 */
  snprintf(msg_buf, sizeof(msg_buf), "user %d: %s", sender_fd, buf);
  broadcast_msg(fds, *fds_count, msg_buf, strlen(msg_buf) + 1, list_sockfd,
                sender_fd);
}

int main(int argc, char *argv[]) {
  /* usual arguments checking (of course you should not do this in production
   * code and you should use something like getopt) */
  if (argc != 2) {
    printf("Usage: %s PORT\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  int list_sockfd, epfd; /* listening socket, epoll instance fd */
  char *port = argv[1];
  struct epoll_event *events; /* these will be filled by epoll_wait */
  int *fds;                   /* keep track of socket fds */
  uint64_t fds_count = 0;     /* count of elements in pfds */

  /* create the epoll instance */
  epfd = epoll_create1(0);
  if (epfd == -1) {
    perror("epoll_create1");
    exit(EXIT_FAILURE);
  }

  /* allocate the maximum possible number of clients - this will be dynamically
   * filled by epoll_wait but NOT allocated by it*/
  events =
      (struct epoll_event *)calloc(MAX_NBR_CLIENT, sizeof(struct epoll_event));
  if (events == NULL) {
    perror("calloc");
    exit(EXIT_FAILURE);
  }

  /* allocate maximum number of possible clients - this is a client-side list
   * to keep track of client socket fds */
  fds = (int *)calloc(MAX_NBR_CLIENT, sizeof(int));
  if (fds == NULL) {
    perror("calloc");
    exit(EXIT_FAILURE);
  }

  /* create the listening socket */
  list_sockfd = create_listening_socket(port, 512);
  if (list_sockfd == -1) {
    perror("create_listening_socket");
    exit(EXIT_FAILURE);
  }

  /* add the listening socket to the fds list */
  add_to_fds(epfd, fds, &fds_count, list_sockfd);

  puts("started the main poll loop");

  /* poll loop, poll-ing loop, main loop, or whatever you want to call it */
  for (;;) {

    /* this blocks until one or more sockets are ready (i.e., instant return
    upon call) for the specified operation */
    int epoll_count =
        epoll_wait(epfd, events, MAX_NBR_CLIENT, -1 /* infinite timeout */);
    if (epoll_count == -1) {
      perror("epoll_wait");
      exit(EXIT_FAILURE);
    }

    /* loop through ready sockets */
    for (int j = 0; j < epoll_count; ++j) {

      uint64_t idx =
          events[j].data.u64; /* data is a custom user-filled field - in this
                                 case we fill the u64 field with the index of
                                 the fd in the fds array */
      int fd = fds[idx];

      /* data is available for reading or client hang up */
      if (events[j].events & (EPOLLIN | EPOLLHUP)) {

        /* if this the listening socket fd then we have a new connection */
        if (fd == list_sockfd) {
          handle_new_connection(epfd, fds, &fds_count, list_sockfd);
        } else { /* either got msg from this client or conn closed */
          handle_client_data(epfd, fds, &fds_count, list_sockfd, idx);
        }

      } else { /* (EPOLLERR) - some error happened */

        if (fd != list_sockfd) {

          char msg_buf[256];
          snprintf(msg_buf, sizeof(msg_buf),
                   "client %d disconnected due to error", fd);
          broadcast_msg(fds, fds_count, msg_buf, strlen(msg_buf) + 1,
                        list_sockfd, fd);
        }

        del_fr_fds(epfd, fds, &fds_count, idx);
      }

    } // END INNER FOR LOOP

  } // END MAIN FOR LOOP

  /* don't forget to free the previously allocated epoll events array (of
   * course, this is not needed here because the process will exit in next
   * instruction - but better always keep this muscle memory) */
  free(events);

  /* close epfd, and fds here etc ... */

  exit(EXIT_SUCCESS);
}
