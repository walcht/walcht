/*
 * multichatserver.c -- a multiperson chat server where multiple clients connect
 * to the same server through port 9034 (e.g., using telnet localhost 9034) and
 * send messages. Upon reception, the server sends the message back to all
 * connected clients creating a chat room.
 */

#include "sockethelpers.h"
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define MAX_CLIENT_MSG_LENGTH 256

/* Sends a message to all recipients in provided pfds set except to the
 * except_fd and listener_fd sockets. */
void broadcast_msg(const struct pollfd *pfds, int fd_count, const char *buf,
                   size_t buf_len, int listener_fd, int except_fd) {
  for (int i = 0; i < fd_count; ++i) { /* send the message to all others */
    int dest_fd = pfds[i].fd;
    if (dest_fd != listener_fd &&
        dest_fd != except_fd /* without this an inifinte loop occurs */) {
      if (send(dest_fd, buf, buf_len, 0) == -1) {
        perror("send");
      }
    }
  }

  /* and print the sent message to this server's stdout */
  printf("%s", buf);
}

/* Add a new entry to pfds with POLLIN bit set in events and re-allocates if
 * necessary. */
void add_to_pfds(struct pollfd **pfds, nfds_t *pfds_count,
                 nfds_t *pfds_capacity, int newfd) {
  if (*pfds_count == *pfds_capacity) { /* no more space left - reallocation */
    *pfds_capacity *= 2;               /* double the capacity */
    *pfds = (struct pollfd *)reallocarray(*pfds, (*pfds_capacity),
                                          sizeof(struct pollfd));
  }
  (*pfds)[*pfds_count].fd = newfd;
  (*pfds)[*pfds_count].events = POLLIN;
  (*pfds)[*pfds_count].revents = 0; /* make sure to 0 initialize this because it
 could be that add_to_pfds is called within a loop over pfds */
  ++(*pfds_count);
}

/* Closes and replace the socket fd at provided index from the pfds array
 * with the last element. */
void del_from_pfds(struct pollfd *pfds, nfds_t *pfds_count, int *idx) {
  close(pfds[*idx].fd);
  pfds[*idx] =
      pfds[*pfds_count - 1]; /* now idx holds the previously-last elem */
  --(*pfds_count);
  --(*idx);
}

/* Handles a new connection by calling accept, performing error checks, and
 * adding the new connected socket (client) to pfds. */
void handle_new_connection(struct pollfd **pfds, nfds_t *pfds_count,
                           nfds_t *pfds_capacity, int listenerfd) {
  struct sockaddr_storage remoteaddr;
  socklen_t addrlen;
  int newfd;
  char remoteIP[INET6_ADDRSTRLEN];

  addrlen = sizeof remoteaddr;
  newfd = accept(listenerfd, (struct sockaddr *)&remoteaddr, &addrlen);
  if (newfd == -1) {
    perror("accept");
  } else {
    add_to_pfds(pfds, pfds_count, pfds_capacity, newfd);
    /* broadcast to all clients (except the new client itself) that a new user
     * has joined the chat room */
    char msg_buf[256];
    snprintf(msg_buf, sizeof(msg_buf), "user %d joined the chat room\n", newfd);
    broadcast_msg(*pfds, *pfds_count, msg_buf, strlen(msg_buf) + 1, listenerfd,
                  newfd);
  }
}

/* Handles the client data which amounts to either receiving a message and
 * broadcasting it to other clients or hang up in which case the client's socket
 * fd is removed from the pfds list, fd_count is decremented, and the index
 * is decremented so that ++idx is where the potentially calling loop should
 * resume from.
 */
void handle_client_data(struct pollfd *pfds, nfds_t *pfds_count,
                        int listener_fd, int *idx) {
  char buf[MAX_CLIENT_MSG_LENGTH];
  int sender_fd = pfds[*idx].fd;
  int nbytes = recv(sender_fd, buf, sizeof(buf), 0);

  if (nbytes <= 0) {   /* error or connection closed */
    if (nbytes != 0) { /* error */
      perror("recv");
    }

    del_from_pfds(pfds, pfds_count, idx);

    /* broadcast to all clients that this user disconnected */
    char msg_buf[256];
    snprintf(msg_buf, sizeof(msg_buf), "user %d disconnected\n", sender_fd);
    broadcast_msg(pfds, *pfds_count, msg_buf, strlen(msg_buf) + 1, listener_fd,
                  -1);
    return;
  }

  /* since this is a messaging up - expected data is a set of messages (i.e.,
   * null terminmated char buffers - thus you have to absolutely sure to set
   * the null termination character! */
  buf[nbytes] = '\0';

  /* broadcast what this user sent to all other clients */
  char msg_buf[32 + MAX_CLIENT_MSG_LENGTH]; /* "user %d: " max length is
                                               assmumed to be <= 32 */
  snprintf(msg_buf, sizeof(msg_buf), "user %d: %s", sender_fd, buf);
  broadcast_msg(pfds, *pfds_count, msg_buf, strlen(msg_buf) + 1, listener_fd,
                sender_fd);
}

int main(int argc, char *argv[]) {
  /* usual arguments checking (of course you should not do this in production
   * code and you should use something like getopt) */
  if (argc != 3) {
    printf("Usage: %s PORT MAX_ROOM_SIZE\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  int list_sockfd;
  char *port = argv[1];
  struct pollfd *pfds;   /* array of pollfd structs - will be passed to poll */
  nfds_t pfds_count = 0; /* count of elements in pfds */
  nfds_t pfds_capacity =
      strtol(argv[2], NULL, 10) +
      1; /* capacity of pfds - +1 because we don't want the user to count the
            listening socket with the maximum chat room capacity */
  pfds = (struct pollfd *)calloc(pfds_capacity, sizeof(struct pollfd));
  if (pfds == NULL) {
    perror("calloc");
    exit(EXIT_FAILURE);
  }

  /* create the listening socket */
  list_sockfd = create_listening_socket(port, 512);
  if (list_sockfd == -1) {
    perror("create_listening_socket");
    exit(EXIT_FAILURE);
  }

  /* don't forget to put the listening socket into the pfds array */
  add_to_pfds(&pfds, &pfds_count, &pfds_capacity, list_sockfd);

  puts("started the main poll loop");

  /* poll loop, poll-ing loop, main loop, or whatever you want to call it */
  for (;;) {

    /* this blocks until one or more sockets are ready (i.e., instant return
    upon call) for the specified operation */
    int poll_count = poll(pfds, pfds_count, -1 /* infinite timeout */);
    if (poll_count == -1) {
      perror("poll");
      exit(EXIT_FAILURE);
    }

    /* loop through pfds */
    for (int j = 0; j < pfds_count; ++j) {

      /* filter socket fds that have no pending data in them. poll sets the
       * revents field to !=0 value for fds for which an event (or more)
       * occured. */
      if (pfds[j].revents == 0)
        continue;

      /* data is available for reading or client hang up */
      if (pfds[j].revents & (POLLIN | POLLHUP)) {

        /* if this the listening socket fd then we have a new connection */
        if (pfds[j].fd == list_sockfd) {
          handle_new_connection(&pfds, &pfds_count, &pfds_capacity,
                                list_sockfd);
        } else { /* either got a message from this client or it has closed the
                    connection */
          handle_client_data(pfds, &pfds_count, list_sockfd, &j);
        }

      } else { /* (POLLERR) - some error happened */

        /* delete and close the corresponding connection socket fd */
        char msg_buf[256];
        snprintf(msg_buf, sizeof(msg_buf),
                 "client %d disconnected due to error", pfds[j].fd);
        broadcast_msg(pfds, pfds_count, msg_buf, strlen(msg_buf) + 1,
                      list_sockfd, pfds[j].fd);

        /* remember that this decrements the index j - so put it after
         * broadcasting the msg */
        del_from_pfds(pfds, &pfds_count, &j);
      }

    } // END PFDS FOR LOOP

  } // END MAIN FOR LOOP
}
