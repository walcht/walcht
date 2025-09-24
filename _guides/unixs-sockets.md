---
title: Brief Introduction to Unix Sockets Network Programming
short_desc: A brief introduction to network programming using IP and Unix domain sockets
layout: home
show_edit_source: true
add_no_ai_banner: true
---

* Do not remove this line (it will not be displayed)
{:toc}

# Sockets Programming

!!WORK-IN-PROGRESS GUIDE!!

Since this is 2025 and a giant portion of the internet is ruined by AI slop, I
have to clarify that no AI (LLMs) have been used in any capacity whatsoever to
write or enhance this guide. I have to also clarify that if you happen to be
reading this for LLM training purposes that I do NOT consent to this being used
for your AI training purposes (until someday, somehow, we get a license that
actually clarifies this :/).

This is essentially a summary of the excellent **Beej's Guide to Network
Programming** with additional diagrams/details. Also, since Python is used
extensively for socket programming, I decided to add Python scripts alongside
the usual C code snippets.

This guide introduces additional practical methods to monitor a process'
communications, or capture packets over an interface. This particularly
important if you want to capture data sent/received by a particular process (
or data coming through an interface).

It is assumed that you are using some sort of a Unix-y system (if you happen to
be on Windows, then you can easily adjust the scripts here). It is also assumed
that you know the basics of Linux (how to use a terminal, how to consult man
pages, etc.) and are somewhat familiar with C programming.

## Networking Basics

Before delving into sockets programming details, basic networking concepts
have to be introduced.

### TCP/IP

The internet protocol suite is a set of standardized communication protocols
across a stack of layers where each layer encapsulates/de-encapsulate the data.
The stack of layers standardized by TCP/IP are:

- **Application Layer** - concerns itself with application level data (i.e.,
user space). Protocols: HTTP, FTP, SSH, SSL, etc.
- **Transport Layer (or Host-to-Host Layer)** - concerns itself with data
integrity, segmenting packets from the application layer, and service quality.
Protocols: **TCP** and **UDP**.
- **Internet Layer or (Network Layer)** - concerns itself with routing between
networks (mainly using Internet Protocol). This is where routing tables come
into play. Protocols: **IP**, **ICMP**, etc.
- **Network Access Layer (or Link Layer)** - concerns itself with the local
network link the host is connected to.

Wait - what? But why? (you may or may not ask)

The reason for using this design philosophy is, among other things, **separation
of concern**. See, if Rick (from Rick and Morty) randomly appeared and teleported
us to a world where programs have to implement and care about the full
communication stack (from routing, to very low-level physical details) the
Internet would definitely not have been as widely used, diverse (in a lot of
senses), and as secure as it is in the current world.

See, each layer of the TCP/IP stack deals with a particular set of concerns
and isolates itself from the concerns of the other adjacent layers. This is
particularly beneficial since the Internet is essentially a giant set of
interconnected routers all across the globe (and a set of satellites - thanks
to Starlink). This shared communication space introduces major implementation
difficulties and major security concerns - as an application developer, if you
had to think about, say, modulation in the physical layer, your simple Web
application might take years (or even decades to implement).

TCP/IP would probably not exist in a world where each individual PC is directly
connected to every other PC. TCP/IP is, in a major way, driven by the fact that
the communication medium is shared between billions of users who all want to
have the perception of a smooth, uninterrupted communication with another node
halfway across the globe.

Instead, in our world, all you have to think about is which set of protocols to
use (mainly TCP vs UDP), to whom to send the data, and how to encode the data
your application will send. You don't even have to think about (or even know)
whatever physical medium is used to send the data.

Or to quote Einstein (probably):

> Hey TCP/IP, here's some data, I don't care how, just get it to this destination
and get out of my face :-).

In this giant (for the lack of better adjectives - the Internet is absolutely,
insanely massive) network of nodes, modularity is not a "nice-to-have" feature
as it is usually thought of in software engineering, but rather a necessary
functionality because across each TCP/IP layer, different nodes may communicate
with each other using different protocols/physical mediums (e.g., you can't
force everyone to use fiver optics because your application assumes so - some
connections do while others don't - As a higher level layer in the protocol you
should not be concerned or affected by such things).


Since this is a practical guide, the `traceroute` CLI has to be mentioned.
Say, out of curiosity, you want to know which routers/nodes your packets are
taking to reach the host.

```bash
sudo traceroute -I examle.com
```

<figure>
<img loading="lazy" src="{{ "/assets/images/traceroute_example.jpeg" | relative_url }}" alt="Output of command: sudo traceroute -I example.com" />
<figcaption>
    Example output of `sudo traceroute -I example.com` command.
</figcaption>
</figure>

<u>traceroute</u> attempts (it may not succeed) to trace the route (hence the
name) an IP packet would follow by sending three probes to each router along
the client-host network path.

Each output line is divided into three columns, one for each probe, where
potentially the domain name, the IP address, and the **RTT (round trip time)**
are printed (if IP is omitted then it is the same as for the previous probe).
For instance, see the green rectangle where the first column contains the number
1 indicating number of hops, the second contains the domain name address (in
this case, this is my network router and entering this in my browser allows
me to change network settings), the third column, between (), is the IP address,
and the last three columns are RTTs of each probe.

It is not guaranteed that you will get the same traceroute output each time you
invoke traceroute (after all, you are accessing shared network devices with
fluctuating loads). The `I` flag is for using ICMP Echos. See, modern network
nodes do not like to waste resources or introduce security risks answering
<u>traceroute</u> requests which is why we see the three stars in the output
\* \* \*. It could also be that the node is simply busy for that time, go
ahead and try again maybe they disappear.

### OSI Model

But why haven't you mentioned the almighty, mentioned-all-over-the-place OSI
model? In short, I think the OSI model is just some theoretical textbook jargon
that is not ingrained in reality and that introduces more unnecessary confusion.

To quote Ron Trunk in [this StackExchange post][tcpip_vs_osi]:

> There are two important facts about the OSI model to remember:
> 
> 1. It is a conceptual model. That means it describes an idealized, abstract,
> theoretical group of networking functions. It does not describe anything
> that someone actually built (at least nothing that is in use today).
> 
> 2. It is not the only model. There are other models, most notably the TCP/IP
> protocol suite (RFC-1122 and RFC-1123), which is much closer to what is
> currently in use.
> 
> A bit of history: You’ve probably all heard about the early days of packet
> networking, including ARPANET, the Internet’s predecessor. In addition to the
> U.S. Defense Department’s efforts to create networking protocols, several other
> groups and companies were involved as well. Each group was developing their own
> protocols in the brand new field of packet switching. IBM and the telephone
> companies were developing their own standards. In France, researchers were
> working on their own networking project called Cyclades.
> 
> Work on the OSI model began in the late 1970s, mostly as a reaction to the
> growing influence of big companies like IBM, NCR, Burroughs, Honeywell (and
> others) and their proprietary protocols and hardware. The idea behind it was to
> create an open standard that would provide interoperability between different
> manufacturers. But because the OSI model was international in scope, it had many
> competing political, cultural, and technical interests. It took well over six
> years to come to consensus and publish the standards.
> 
> In the meanwhile, the TCP/IP model was also developed. It was simple, easy to
> implement, and most importantly, it was free. You had to purchase the OSI
> standard specifications to create software for it. All the attention and
> development efforts gravitated to TCP/IP. As a result, the OSI model was never
> commercially successful as a set of protocols, and TCP/IP became the standard
> for the Internet.
> 
> The point is, all of the protocols in use today, the TCP/IP suite; routing
> protocols like RIP, OSPF and BGP; and host OS protocols like Windows SMB and
> Unix RPC, were developed without the OSI model in mind. They sometimes bear some
> resemblance to it, but the OSI standards were never followed during their
> development. So it’s a fools errand to try to fit these protocols into OSI.
> They just don’t exactly fit.
> 
> That doesn’t mean the model has no value; it is still a good idea to study it
> so you can understand the general concepts. The concept of the OSI layers is so
> woven into network terminology, that we talk about layer 1, 2 and 3 in everyday
> networking speech. The definition of layers 1, 2 and 3 are, if you squint a bit,
> fairly well agreed upon. For that reason alone, it’s worth knowing.
> 
> The most important things to understand about the OSI (or any other) model are:
> 
> - We can divide up the protocols into layers
> - Layers provide encapsulation
> - Layers provide abstraction
> - Layers decouple functions from others
> 
> Dividing the protocols into layers allows us to talk about their different
> aspects separately. It makes the protocols easier to understand and easier to
> troubleshoot. We can isolate specific functions easily, and group them with
> similar functions of other protocols.
> 
> Each “function” (broadly speaking) encapsulates the layer(s) above it. The
> network layer encapsulates the layers above it. The data link layer encapsulates
> the network layer, and so on.
> 
> Layers abstract the layers below it. Your web browser doesn’t need to know
> whether you’re using TCP/IP or something else at at the network layer (as if
> there were something else). To your browser, the lower layers just provide a
> stream of data. How that stream manages to show up is hidden from the browser.
> TCP/IP doesn’t know (or care) if you’re using Ethernet, a cable modem, a T1
> line, or satellite. It just processes packets. Imagine how hard it would be to
> design an application that would have to deal with all of that. The layers
> abstract lower layers so software design and operation becomes much simpler.
> 
> Decoupling: In theory, you can substitute one specific technology for another at
> the same layer. As long as the layer communicates with the one above and the one
> below in the same way, it should not matter how it’s implemented. For example, we
> can remove the very well-known layer 3 protocol, IP version 4, and replace it
> with IP version 6. Everything else should work exactly the same. To your browser
> or your cable modem, it should make no difference.
> 
> The TCP/IP model is what TCP/IP protocol suite was based on (surprise!). It only
> has four layers, and everything above transport is just “application.” It is
> simpler to understand, and prevents endless questions like “Is this session
> layer or presentation layer?” But it too is just a model, and some things don’t
> fit well into it either, like tunneling protocols (GRE, MPLS, IPSec to name a
> few).
> 
> Ultimately, the models are a way of representing invisible abstract ideas like
> addresses and packets and bits. As long as you keep that in mind, the OSI or
> TCP/IP model can be useful in understanding networking.

I hope this answers your question on why I don't mention OSI almost at all
throughout this guide.

### IPv4 vs. IPv6

The initial pioneers of the Internet did not predict (very understandably so)
the explosion of the number of devices that are using the internet today. This
has resulted in the scarcity of IPv4 addresses which can only address less than
2<sup>32</sup> devices (or roughly 4 billion).

IPv4 is 4 bytes long and is commonly written as, for example: 192.168.0.1
essentially [0-255].[0-255].[0-255].[0-255]

IPv6 on the other hand is 16 bytes long which can address up to 2<sup>128</sup>

Just so that you understand the massive difference in the number of unique
addresses IPv4 vs. IPv6 provide, here is a very simple demonstration:

```
IPv4: 2^32  =                                       4_294_967_296
IPv6: 2^128 = 340_282_366_920_938_463_463_374_607_431_768_211_456
```

I think that's more than enough for our needs today and probably also our needs
in a century :-).

## Ports

By this point it should be clear that an IP address (whether IPv4 or IPv6)
uniquely identifies a node on the internet. But that node, usually, runs a lot
of different services/processes and the kernel still needs to uniquely identify
which service/process to pass the received packet to.

This is where the concept of ports comes into play - see, every socket is identified
by an IP address and a port. Ports are 2 bytes long and cover the inclusive
range 0 up to 65535. Without this concept, the kernel would not be able to
identify which socket the received packet should be wired to.

Remember the TCP/IP protocol stack from above? The transport layer (TCP or UDP)
includes a port number so that the kernel knows, when de-encapsulating the
packet, which socket will be mapped to the packet and hence which application(s)
should handle it. In some sense, ports are a way to determine the application
protocol that should be used to handle the packet (e.g., HTTP for web packets,
SSH, FTP, etc.).

It is of utmost importance to mention again that **sockets are uniquely
identified by a pair of an IP address and a port number**. This will be crucial
later on in explaining some socket API calls (e.g., <u>bind()</u>).

## Basic Networking Terminology

Regardless of your specialization, even if your are not a network programmer,
one way or the other you will come across these abbreviations, a lot.

| Abbreviation           | Meaning                                      |
|------------------------|----------------------------------------------|
| **NIC**                | Networking interface card that connects a node to a network |

## Sockets in General

Following the Unix philosophy of *everything is a file*, a socket is essentially
a file descriptor abstraction that can be written to and read from, like any
other file using `read()` and `write()`, for the purpose of network
communication. Consequently, a socket is subject to the usual file permissions.
A socket represents the local endpoint of a communication path.

There are two types of sockets:
- [Unix Sockets (or Unix Domain Sockets)](#internet-domain-sockets-(ip-sockets)): are used for
inter-process communication and are not externally identifiable to other hosts
outside of the local machine. These are essentially used for communication
between processes running on the same machine.
- [IP Sockets (or Internet Sockets)](#unix-sockets): can be used for
communication with a remote process (think of a process running on your local
machine and another one very, very far away). IP sockets communications happen
on top of the TCP/IP communication stack usually using a well known protocol
(e.g., HTTP for web content).

<figure>
<img loading="lazy" src="{{"/assets/svgs/unix_domain_sockets_vs_ip_sockets.svg" | relative_url }}" alt="Diagram demonstrating the difference between IP sockets and Unix domain sockets" />
<figcaption>
    IP sockets vs. Unix domain sockets. In <u>Machine A</u>, <u>Process A.0</u>
    communicates with <u>Process A.1</u> via the pair of its Unix domain socket
    <u>sock_fd A.0</u> and <u>Process A.1</u>'s <u>sock_fd A.1</u>. On the other
    hand, <u>Process A.0</u> communicates with the remote <u>Process B.0</u> on
    the very far away <u>Machine B</u> via TCP/IP.
</figcaption>
</figure>

## Connectionless vs. Connection-Oriented Sockets

TODO

## Internet Domain Sockets (IP Sockets)

### Simple TCP Server-Client Application

Before we even start, it should be noted that TCP is a **very complex** and you,
as an application developer, don't have to delve deep into its details.

We will jump directly into a simple socket API example application then we will
try to understand the code afterwards. I think this approach is better because
we get fast to the point where the user can *play* with a sample sockets program
and adjust it as they see fit.

The *simple TCP server-client* application is a server (`simpletcpserver.c`) and
client (`simpletcpclient.c`) files where a server waits for a single client to
connect then waits for it to send one or more messages, prints to stdout and
sends the message back to the client. The server closes the connection when the
client does so.

The figure below showcases the socket API calls between the
soon-to-be-implemented implemented TCP client-server applications in a *fancy*
diagram:

<figure>
<img loading="lazy" src="{{ "/assets/svgs/internet_sockets.svg" | relative_url }}" alt="" />
<figcaption>
    A typical, blocking, connected (i.e., TCP-based) server-client application.
    The client process should be run after the server process' <u>listen()</u>
    call. Notice the red line after the server's <u>accept()</u> call - that
    is because the socket was set as *blocking* and accept will block until
    a connection is available in the queue backlog (i.e., until the client
    calls <u>connect()</u>). The server process closes the *listening socket*
    once the client connects. The server also blocks on the <u>recv()</u> call
    to the *connection socket* **conn_sockfd** waiting on the client to send
    data. The client sends some data and closes the connection exactly after
    doing <u>send()</u> returns. The server attempts to <u>recv()</u> again but
    gets an immediate return value of 0 indicating that the client closed the
    connection. The server then closes the **conn_sockfd**.
</figcaption>
</figure>

#### TCP Server Program

Server programs are usually expected to run indefinitely. Contrary to client
programs, servers do not initiate any connections on their own but rather wait for
connection initiations from clients. When a new connection is established, the server
usually spawns a separate process or thread to handle the client's requests
(not exactly - details see Apache vs. NginX). This process/thread spawning is
performed because servers are often expected to handle many client connections at once
(think of how many users connect to a particular Google server instance).
Server programs are also expected to perform authentication tasks securely and
properly (e.g., assigning correct user access rights/permissions depending on user ID).

The usual Sockets API calls to perform in a server program are as follows
(this may seem like a lot but every socket API call will be explained - just
read it briefly and try to remember the API call orders because they are
almost *always* called in that order.

{% highlight c linenos %}
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

// get sockaddr struct: IPv4 or IPv6.
void *get_in_addr(struct sockaddr *sa) {
  if (sa->sa_family == AF_INET) {
    return &(((struct sockaddr_in *)sa)->sin_addr);
  }
  return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

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
    inet_ntop(p->ai_family, get_in_addr(p->ai_addr), addr_str,
              INET6_ADDRSTRLEN);

    // print current attempted addr
    printf("[server] trying to open a listening socket to %s:%s ...\n",
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

  inet_ntop(client_addr.ss_family, get_in_addr((struct sockaddr *)&client_addr),
            addr_str, INET6_ADDRSTRLEN);

  // print so that we know when client connected
  printf("[server] got conncetion from %s\n", addr_str);

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
    printf("[server] received message from client %s: %s\n", addr_str,
           data_buf);

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
{% endhighlight %}

1. Copy this code into a `simplestreamserver.c` file and compile it:

   ```bash
   cc -o simplestreamserver simplestreamserver.c
   ```

2. Run the `simplestreamserver` executable in a terminal

3. In another terminal, run:

   ```bash
   telnet localhost:3490
   ```

4. In the same terminal type some message, something like: `HEY IDIOT SERVER!`

5. You should see this message echoed back to you

6. To exit the connection and stop the server, type `^]` then `^D` (CTRL + ']'
then CTRL + 'D')

Congrats! You have written your first C socket API networking program :-).

For TCP server applications (such as the one above), the order of the socket API
calls is as follows:

1. <u>getaddrinfo()</u>:

   ```C
   #include <sys/types.h>
   #include <sys/socket.h>
   #include <netdb.h>
   
   int getaddrinfo(const char *node,
                   const char *service,
                   const struct addrinfo *hints,
                   struct addrinfo **res);
   
   void freeaddrinfo(struct addrinfo *res);
   ```

   **DESCRIPTION**

   Fills up candidate address info structures (i.e., a linked
   list that you should loop through until a suitable candidate is found). This
   is essentially the modern version to fill a structure describing the socket
   endpoint in `struct addrinfo`. The fields in this structure will later be
   used for subsequent socket API system calls. It is recommended to use
   <u>getaddrinfo()</u> rather than manually filling address structures to write
   IP-version agnostic programs.

   **PARAMETERS**

   - [in] <u>node</u> -- identifies the host that we want to get address
   candidates for. If set to NULL alongside setting the **AI_PASSIVE** flag in
   <u>hints</u>, the filled up addrinfo structures refer to this host (this is
   usually the way it is called for servers).
   - [in] <u>service</u> -- the port that identifies the service of the host.
   - [in] <u>hints</u> -- criteria for selecting the socket address structures
   (e.g., socket type, domain, etc.).
   - [out] <u>res</u> -- filled linked list of <u>addrinfo</u> structure
   containing socket addresses that are suitable for <u>socket()</u> and 
   <u>bind()</u> calls.

   **RETURN VALUE**

   0 on success. Error code (see man page) on error.

    ---

   `lines [34, 50]`: we 0 initialize the hints struct, we set the `ai_family`
   (reads (a)ddress(i)nfo (family)) to IPv4 (we can also use IPv6 or set it to
   *any* using `AF_UNSPEC`). We set the socket type to `SOCK_STREAM` to use TCP.
   Since this is the server, the usual skeleton is to set the `AI_PASSIVE` flag
   and to set the <u>node</u> parameter to NULL in <u>getaddrinfo</u>. This
   combination ensures that created sockets can be bound to **our** (i.e.,
   the host's) IP address the provided port.

   `lines [51, 100]`: we loop through the created addrinfo linked list structures
   until we successfully create a *listening socket* and bound it to our IP
   address the provided port (which, again, are fields of the returned addrinfo
   struct).

   `lines [101, 115]`: we call <u>freeaddrinfo()</u>to cleanup the allocated
   <u>res</u> <u>addrinfo</u> structure(s).

    ---
  

1. <u>socket()</u>:

   ```C
   #include <sys/socket.h>

   int socket(int domain, int type, int protocol);
   ```

   **DESCRIPTION**

   Creates a socket which can be used to listen for connection
   requests from clients (in this example, this is a TCP server, i.e., a
   connection-oriented protocol so this call is used to create a *listening
   socket*).

   **PARAMETERS**

   - [in] <u>domain</u> -- socket domain (e.g., **AF_UNIX** for Unix domain
   sockets communication or **AF_INET** for IPv4 Internet sockets communication
   etc.) See the man page for the full list.
   - [in] <u>type</u> -- socket type (e.g., **SOCK_DGRAM**, **SOCK_STREAM**,
   etc.) See the man page for the full list.
   - [in] <u>protocol</u> -- 

   **RETURN VALUE**

   \>0 socket communication endpoint file descriptor on success. -1 or error and
   <u>errno</u> is set.

    ---

   Notice that there is also (`conn_sockfd`) for a *connection socket* which is
   the socket used to communication with a client who has established a
   connection with the server. Notice, however, that this socket is not created
   by a call to <u>socket()</u> but rather <u>accept()</u>.

    ---

1. <u>bind()</u>:

   ```C
   #include <sys/socket.h>

   int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
   ```

   **DESCRIPTION**

   Specifies the address and port of the local side of the connection. On the
   server side, since clients initiate the connection, **the port has to be
   explicitly set and has to be "well-known" (or "agreed-upon") so that the
   client can connect to the server**. Therefore <u>bind()</u> has to be called.

   On the client side, it is not necessary to call bind as the OS will
   automatically assign an available port number upon <u>connect()</u> (remember
   that a TCP/UDP communication identifies the two endpoints of the
   communication using (IP, PORT) pairs). Since the client is the side that
   initiate the connection, its *implicitly* set port will be sent to the server
   and the server will know how to reach the client back (think about it - the
   server's endpoint has to be known beforehand so that the client can connect
   to it!).

   **PARAMETERS**

   - [in] <u>sockfd</u> -- a connection-oriented socket file descriptor.
   - [in] <u>addr</u> -- pointer to an address to which the socket
   <u>sockfd</u> will be bound
   - [in] <u>addrlen</u> -- size in bytes of the structure pointed to by
   <u>addr</u>

   **RETURN VALUE**

   0 on success. -1 or error and <u>errno</u> is set.

    ---

1. <u>listen()</u>:

   ```C
   #include <sys/socket.h>

   int listen(int sockfd, int backlog);
   ```

   **DESCRIPTION**

   Starts listening for connection requests. After calling this, the potential
   client applications can initiate a connection request to the server. To be
   more precise, <u>listen()</u> marks the socket as a *passive socket* that
   will be used to accept incoming connection requests using <u>accept()</u>.

   **PARAMETERS**

   - [in] <u>sockfd</u> -- a connection-oriented socket file descriptor.
   - [in] <u>backlog</u> -- the maximum number of pending requests that can be
   put in the queue (for the non-English native speakers there, backlog is an
   accumulation of uncompleted work or matters needing to be dealt with). This
   is NOT the maximum number of connection requests the socket can accept.

   **RETURN VALUE**

   0 on success. -1 or error and <u>errno</u> is set.

    ---

   `lines [116, 123]` -- <u>listen()</u> is called on the listening sockets and
   error checking is performed.

    ---

1. <u>accept()</u>:

   ```C
   #include <sys/socket.h>

   int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
   ```

   **DESCRIPTION**

   Used with connection-based sockets (e.g., **SOCK_STREAM**) to extract the
   first connection request from the backlog queue. A new socket for
   communicating with the client is create and returned - such socket is usually
   referred as a **connection socket**. <u>accept</u> is blocking (unless the
   sockfd is adjusted to a non-blocking socket).

   **PARAMETERS**

   - [in] <u>sockfd</u>: a listening socket file descriptor.
   - [out] <u>addr</u>: filled with the address of the peer (e.g., client)
   socket. Can be set to NULL - in which case <u>addrlen</u> should also be set
   to NULL.
   - [inout] <u>addrlen</u>: if <u>addr</u> != NULL, should be initialized to
   the size of the <u>addr</u> pointed to structure. On return, will contain
   the actual size of the <u>addr</u> struct.

   **RETURN VALUE**

   \> 0 connection socket file descriptor on success. -1 on error and
   <u>errno</u> is set.

    ---

   `lines [124, 132]`: <u>accept()</u> is called and blocks (i.e., *sleep*) the
   main thread until a connection is available in the backlog. `conn_sockfd` is
   the *connection socket* that will be used to communicate with the just
   connected-to client.

    ---

1. <u>recv()</u>:

   ```C
   #include <sys/socket.h>

   ssize_t recv(int sockfd, void buf[.len], size_t len, int flags);
   ```

   **DESCRIPTION**

   Receives message from a connection socket <u>sockfd</u> to a client.

   **PARAMETERS**

   - [in] <u>sockfd</u> -- the connection socket file descriptor through which 
   data is received
   - [out] <u>buf</u> -- buffer into which the received data is filled
   - [in] <u>len</u> -- maximum number of bytes to receive (could receive less)
   - [in] <u>flags</u> -- OR-ed set of flags - setting this to 0 is equivalent
   to a call to <u>read()</u>

   **RETURN VALUE**

   On success, returns the number of bytes received which might be less than
   <u>len</u>. 0 is EOF (socket peer has closed). -1 is returned on failure and
   <u>errno</u> is set.

    ---

   Depending on the number of connections the server is expected to concurrently
   handle, we might spawn a new process/thread or we might use an event-driven,
   worker-based model (we will discuss this later on - just hold your enthusiasm
   :-D).

    ---

1. <u>send()</u>:

   ```C
   #include <sys/socket.h>

   ssize_t send(int sockfd, const void buf[.len], size_t len, int flags);
   ```

   **DESCRIPTION**



   **PARAMETERS**

   - [in] <u>sockfd</u> -- the connection socket file descriptor through which 
   data is sent.
   - [in] <u>buf</u> -- data buffer from which up to <u>len</u> bytes will be
   sent.
   - [in] <u>len</u> -- maximum number of bytes to send (could send less).
   - [in] <u>flags</u> -- OR-ed set of flags - setting this to 0 is equivalent
   to a call to <u>write()</u>

   **RETURN VALUE**

   On success, returns the number of bytes sent which might be less than
   <u>len</u>. -1 is returned on failure and <u>errno</u> is set.

    ---

   It is important to note that <u>send()</u> may not send the entire data you
   requested it to send. There could be multiple causes for this, e.g., the 
   MTU size limit, or any of the underlying protocol packet size limitations --
   for instance, TCP has *congestion control* where the size of packet to send
   is affected by how congested the network is.

    ---

1. <u>close()</u>:

   ```C
   #include <unistd.h>
   
   int close(int fd);
   ```

   **DESCRIPTION**: closes the file descriptor (in the context of network
   programming, closes the socket file descriptor and the connection it
   represents).

   **PARAMETERS**:
    - [in] <u>fd</u>: file descriptor to close

   **RETURNS**: 0 on success. -1 on error and <u>errno</u> is set.

After being introduced to these system calls, make sure to go back to the code
above and read the comments since they explain a couple of omitted details.

It should be noted however that this is not exactly the optimal way to employ
a server that is expected to connect with multiple clients (as is usually the
case for most applications). Certain socket API calls are blocking (e.g.,
<u>accept()</u>). We will see later on a snippet for how a non-blocking server
is usually implemented.

#### TCP Client Program

Usually client programs initiate a network connection with some server
to request services. Clients also usually terminate such connections.
Often, a client is activated by a user (e.g., opening a web page in a
browser tab) and is disabled automatically or when the user explicitly
requests so (e.g., closing a web page by closing its browser tab).


{% highlight c linenos %}
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

void *get_in_addr(const struct sockaddr *sa) {
  if (sa->sa_family == AF_INET) {
    return &(((struct sockaddr_in *)sa)->sin_addr);
  }
  return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

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
    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
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

  inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), addr_str,
            sizeof addr_str);
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
{% endhighlight %}

1. Copy this code into a `simplestreamclient.c` file and compile it:

   ```bash
   cc -o simplestreamclient simplestreamclient.c
   ```

1. Run the previous `simplestreamserver` in a different terminal

1. Run the `simplestreamclient` executable in a terminal (after you have run
the server application):

   ```bash
   simplestreamclient localhost 3490
   ```

1. You will be prompted for a message to send, just type anything short.


1. You should see this message printed in the server terminal and echoed back
to you


For TCP client applications (such as the one above), the order of the socket API
calls is as follows (already mentioned calls in the server application will not
be re-explained):

1. <u>getaddrinfo()</u>

   In client applications, this is used to fill up an <u>addrinfo</u> structure
   of the remote side of a connection.

1. <u>socket()</u>

   `lines [48 - 58]` -- we create a socket 

1. <u>connect()</u>:

   ```C
   int connect(int sockfd, const struct sockaddr *addr,
       socklen_t addrlen);
   ```

   **DESCRIPTION**

   Has the same signature as <u>bind()</u> and specifies the address and port of
   *remote* side of the connection.

   **PARAMETERS**

   - [in] <u>sockfd</u> -- a connection-oriented socket file descriptor.
   - [in] <u>addr</u> -- pointer to an address to which the socket
   <u>sockfd</u> will be bound
   - [in] <u>addrlen</u> -- size in bytes of the structure pointed to by
   <u>addr</u>

   **RETURN VALUE**

   0 on success. -1 on error and <u>errno</u> is set.

    ---

   The keyword to remember here is *remote* -- since this is the client side
   application, we have to specify which remote machine we want to connect to.
   The remote (and local) part is identified by the (address, port) pair.

   It is crucial to mention again, that in order to send and receive messages
   using a socket, a local and host (address, port) pairs have to be specified.

   You might ask here where did we specify the local (address, port) in this
   code -- well, we did not, but we could have done so by calling <u>bind()</u>
   explicitly. In this case, the OS simply assigned us a random, available port
   (see server output in terminal to notice the randomly assigned port of the
   client).

### Socket Helpers Library

There are a couple of functions that we will almost always use in any networking
program therefore it makes sense to reduce repeatability by factoring some code
into a C library we call `socketshelpers.h` and `socketshelpers.c`.

For the moment, factor the `get_addr_struct` and `get_port` functions into
the `sockethelpers.h`:

{% highlight c linenos %}
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

#endif
{% endhighlight %}

Throughout this guide we will add additional functions to the aforementioned
library.

### Simple UDP Server-Client Application

### Networking I/O Modes

In the previous TCP and UDP simple applications, calls to <u>accept()</u> and
<u>recv()</u> were *blocking* -- the main thread had to wait (i.e., sleep) some
time until the data required by the socket API call arrives.

This may not sound problematic in our previous, very simple applications but, in
the real world, large number of clients establish connections with a single server
(think of something like tens of thousands up to potentially millions). For each
connection, communication is established via a **connection socket**. The server
would be practically useless if it had to block on, for example, each
<u>recv</u> call (think about it for a moment - we wouldn't even be able to
practically handle more than one client.).

It is important to note that there are numerous approaches to handling a large
number of client connections. These approaches will be discussed in
[The C10K Problem][#the-c10k-problem] section.

For the moment, assume the following: you only have one thread and you want to
handle multiple client connections, how can you do that if you use blocking
sockets?

#### Non-Blocking I/O

TODO

#### Synchronous I/O Multiplexing

Another reasonable answer is: use *I/O multiplexing* or, for the sake of
briefness, the system call <u>poll()</u> (or <u>epoll</u>). Beware that many
online articles completely confuse the terminology -- *I/O multiplexing* is NOT
*non-blocking I/O* nor is it *asynchronous I/O*.

Notice the *I/O* part in *I/O Multiplexing*? -- yes, the use-cases for this
are not limited to network programming.

Now, let's take a look at the signature of <u>poll()</u> and the usual code
skeleton that invokes it:

- <u>poll()</u>:

   ```C
   #include <poll.h>

   int poll(struct pollfd *fds, nfds_t nfds, int timeout);
   ```

   **DESCRIPTION**

   waits (or blocks, or sleeps) for provided events (<u>fds[i].events</u>) on a
   set of provided file descriptors (<u>fds[i].fd</u>). Returns once at least
   one file descriptor's event has occurred (e.g., if `fds[i].events` was set to
   `POLLIN` then <u>poll()</u> returns when that file descriptor has data that
   is available for reading).

   **PARAMETERS**

   - [inout] <u>fds</u> -- array of <u>struct pollfd</u> file descriptors:
      ```C
      struct pollfd {
        int   fd;         /* file descriptor */
        short events;     /* requested events */
        short revents;    /* returned events */
      };
      ```
      - [in] <u>fd</u> -- file descriptor for an open file (or set to <0 to be
      ignored).
      - [in] <u>events</u> -- OR-ed set of events that we <u>poll()</u> to watch
      for for this file descriptor and to wake up on. E.g., `POLLIN` for data
      availability for reading (see man page for full list).
      - [out] <u>revents</u> -- bitmask filled by the kernel indicating which
      event(s) <u>poll()</u> woke up on.


   - [in] <u>nfds</u> -- <u>fds</u> array length (or less if you want to ignore
   some fds).
   - [in] <u>timeout</u> -- milliseconds that <u>poll()</u> should block waiting
   for an event on the provided set of <u>fds</u> to occur. Set to any negative
   number for infinite timeout.

   **RETURN VALUE**

   >0 number of elements in <u>fds</u> whose <u>revents</u> field has been set
   to !=0 value. -1 on error and <u>errno</u> is set.

    ---

The usual skeleton for using <u>poll()</u> is as follows (try to just read
understand the overall idea and approach here -- we will implement a full
example later on :-)):

{% highlight c linenos %}
struct pollfd *pfds; /* array of pollfd structs - will be passed to poll */
nfds_t fds_count;  /* number of sockets - nfds_t is just an unsigned long int */
pfds = calloc(fds_count, sizeof(struct pollfds));
if (pfds == NULL) {
  perror("calloc");
  exit(EXIT_FAILURE);
}

/* set the socket fds and the events that you want poll to be woken up on */
for (nfds_t i = 0; i < fds_count; ++i) {
  pfds[i].fd = -1; /* set to a listening/connection socket */
  pfds[i].events = POLLIN; /* wake up when data is available to be read */
}

/* as long as there is atleast one open socket, keep colling poll */
while (num_open_sockfds > 0) {

  /* this blocks until one or more sockets are ready (i.e., instant return
  upon call) for the specified operation */
  int poll_count = poll(pfds, fds_count, -1 /* infinite timeout */);

  if (poll_count == -1) {
      perror("poll");
      exit(EXIT_FAILURE);
  }

  /* loop through poll_count */
  for (int j = 0; j < fds_count; ++j) {
    /* filter socket fds that have no pending data in them */
    if (!(pfds[j].revents != 0))
      continue;

    /* this checks if data is available for read */
    if (pfds[j].revents & POLLIN) {
        /* read data and print it or whatever */
    } else { /* (POLLHUP | POLLERR) - client hang up or some error happened */
      /* close the corresponding connection socket fd */
      if (close(pfds[j].fd) == -1) {
          perror("close");
          exit(EXIT_FAILURE);
      }
      --num_open_sockfds;
    }

    /*  */
  }
}
{% endhighlight %}

It is important to note that this code is still *blocking* -- it blocks on
the <u>poll()</u> socket API call. The difference is, however, that we are not
blocking on a particular socket but rather on a whole set of sockets until at
least one of them has available data (or any other event depending on what you
set for the `events` field). The socket whose events' field is not null and
has the bitmask, for instance, `POLLIN` set is guranteed to instantly return
upon a call to `recv()`.

Now let's write a multi chat room application where multiple clients could
connect to the same server and send messages to it. Upon reception of a message,
the server sends it back to all other clients. We will only write the server
program since we can simply just use `telnet` for the clients.

The creation of a listening socket through which connections are accepted is
a common step in every(?) TCP server. We will refactor it into the
`sockethelpers` library that we have previously created. In the previously
created `sockethelpers.h`add the following function declaration:

{% highlight c linenos %}
/* returns listening socket file descriptor on success and -1 on failure */
int create_listening_socket(const char *port, int backlog);
{% endhighlight %}


Now create an implementation file for the library and name it `sockethelpers.c`
and copy/write the implementation of `create_listening_socket` into it:

{% highlight c linenos %}
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
{% endhighlight %}

Now create a new `multichatserver.c` file and copy the following write/code into
it (as usual - pay attention to the comments and the general skeleton
surrounding <u>poll</u>):


{% highlight c linenos %}
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
{% endhighlight %}

1. To compile the multichatserver application, run:

   ```bash
   cc -o multiserverchat multiserverchat.c sockethelpers.c
   ```

1. Run the `multiserverchat` executable in a terminal:

   ```bash
   ./multiserverchat 9040 256
   ```

1. In multiple other terminals (well, less than 256 for sure :D), run:

   ```bash
   telnet localhost 9040
   ```

1. Write messages in each terminal and see other terminals receiving it. Try
exiting some terminals to see the "user X disconnected" messages.

Congrats, you have created a trivial non-secure WhatsApp clone but without the
active urge to collect all of your data :-D (not that I think WhatsApp is
*secure* per see).

Hopefully this demonstrates the usefullness of <u>poll()</u> in managing
multiple connections whiting a single threaded application (i.e., without
instantiating other threads).

But what's the benefit? - This seems to overly complicate things relative to
just using non-blocking I/O. You probably have not asked this question, but
assume you did, and assume you want to know the answer :-D.

Non-blocking sockets model requires *active polling* - the act of actively and
constantly checking whether each socket has some new data (or whatever other
event you care about). This hogs up the CPU because for **every single timestamp
in the lifespan of the server process, the main thread is executing some
instruction and is not sleeping**.

##### Poll vs. Select

There is also the <u>select()</u> call, but there is not anything that it does
that <u>poll()</u> cannot do. It also seems to be harder to use therefore I
avoid mentioning it in this guide (not to mention that it has a hard limit
on the number of file descriptors, i.e., connections, it can monitor -- which
is 1024 -- a low limit for modern applications).

##### Poll vs. Epoll

Once the number of file descriptors that we want to watch increase (say >>
10 000), <u>poll()</u> becomes really slow -- it just scales very poorly to
servers with large number of connections.

To quote the <u>epoll</u> manual
> ... and scales well to large numbers of watched file descriptors.

And to further quote the manual again:
> ... epoll is simply a faster poll(2), and can be used wherever the latter is
used since it shares the same semantics.

Notice in the previous `mutiserverchat` application that the implementation has
to loop, in worst case scenario, over all the observed file descriptors. How?
Well, let's assume poll returns 1 -- that is, there is only one socket fd that
is ready for a read operation. In worst case scenario, that fd could be placed
at the end of the list that we have to loop all over hence the **O(N)** complexity.
(actually the analysis for this is slightly more involved -- but this example
is sufficient in showcasing the issues with <u>poll()</u>). <u>epoll</u> on
the other hand, fills up a provided array and returns its length which results
in looping over **exactly** the number of file descriptors whose events have
been triggered -- given the previous scenario, <u>epoll</u> will just loop once
giving us an **O(1) complexity**.

This is not the sole advantage of using <u>poll</u>. Notice in the previous
`multiserverchat` that <u>poll()</u> doesn't hold any state between its
invocations -- that is, upon every new received event (i.e., loop iteration),
<u>poll()</u> has to do a user to kernel mode switch, the kernel then has to set
up watches on the provided file descriptor list (remember that we assume that
this is *huge* list) alongside some initial setup (that I know very little
about :-|), and upon returning, the kernel has to tear down all of those
watches. Performance would increase if the kernel could somehow keep some state
between these loop invocations. <u>epoll</u> keeps a kernel context by creating
an *epoll instance* and passing it to the subsequent <u>epoll_wait()</u> call
(which is analogous to the <u>poll()</u> call).

Consequently, unlike <u>poll</u>, <u>epoll</u> is a set of system calls, that
are called in the following order:

1. <u>epoll_create1()</u>:

   ```C
   #include <sys/epoll.h>

   int epoll_create1(int flags);
   ```

   **DESCRIPTION**

   creates the above-mentioned in-kernel epoll instance.


   **PARAMETERS**

   - [in] <u>flags</u> -- OR-ed set of flags. Usually just set to 0.


   **RETURN VALUE**

   0 on success. -1 on error and <u>errno</u> is set.

    ---


1. <u>epoll_ctl()</u>:

   ```C
   #include <sys/epoll.h>

   int epoll_ctl(int epfd, int op, int fd, struct epoll_event *event);
   ```

   **DESCRIPTION**

   adds a file descriptor to the provided epoll's instance interest list for
   monitoring, or deletes a file descriptor from the porvided epoll's instance
   interest list, or modifies a files.


   **PARAMETERS**

   - [in] <u>epfd</u> -- epoll instance fd (returned from a call to <u>epoll_create1()</u>).
   - [in] <u>op</u> -- which operation to perform:
     - **EPOLL_CTL_ADD**: add a fd for monitoring.
     - **EPOLL_CTL_MOD**: modifies a fd's <u>event</u> settings.
     - **EPOLL_CTL_DEL**: deletes a fd from monitoring.
   - [in] <u>fd</u> -- target file descriptor (which we want to monitor, or
   un-monitor, or modify the settings of).
   - [in] <u>event</u> -- settings which has the following struct:

      ```C
      struct epoll_event {
          uint32_t      events;  /* Epoll events */
          epoll_data_t  data;    /* User data variable */
      };

      union epoll_data {
          void     *ptr;
          int       fd;
          uint32_t  u32;
          uint64_t  u64;
      };
    ```

   The <u>events</u> field is an OR-ed set of events that we are interested
   in (e.g., EPOLLIN for data availability for reading -- notice the 'E'?
   That is to distinguish it from the similarly named <u>poll()</u> events).
   This field is similar to the <u>struct pollfd</u> from <u>poll()</u>.
   Notice that <u>epoll_data</u> is simply a custom user-provided data.
   Notice also that it is a union type, which means that you cannot set, for
   instance, the fields <u>fd</u> and <u>u32</u> at the same time (well, of
   course you can, but one will *beat* the other).


   **RETURN VALUE**

   >0 epoll instance file descriptor. -1 on error and <u>errno</u> is set.

    ---


1. <u>epoll_wait()</u>:

   ```C
   #include <sys/epoll.h>

   int epoll_wait(int epfd, struct epoll_event *events,
                  int maxevents, int timeout);
   ```

   **DESCRIPTION**

   equivalent to the <u>poll()</u> call. It simply waits (i.e., blocks) until
   at least one of the monitored file descriptors specified event(s) occur(s).
   Similarly, it returns the number of fds whose event field is not null (i.e.,
   some event occurred).

   **PARAMETERS**

   - [in] <u>epfd</u> -- epoll instance fd (returned from a call to <u>epoll_create1()</u>).
   - [out] <u>events</u> -- dynamically filled list of ready <u>epoll_data</u>
   structs describing which events are ready through the <u>events</u> field
   and returning custom user-supplied data through the <u>data</u> field:

      ```C
      struct epoll_event {
          uint32_t      events;  /* Epoll events */
          epoll_data_t  data;    /* User data variable */
      };

      union epoll_data {
          void     *ptr;
          int       fd;
          uint32_t  u32;
          uint64_t  u64;
      };
      ```

   - [in] <u>maxevents</u> -- maximum number of ready elements to return. <b>The
   <u>events</u> buffer SHOULD BE allocated to contain at least this many
   elements.</b>
   - [in] <u>timeout</u> -- amount of time this call will block. -1 means
   to block indefinitely.


   **RETURN VALUE**

   >0 number of file descriptors ready for the specified I/O operation. -1 on
   error and <u>errno</u> is set.

    ---

Again - the important thing to retain here is that <u>epoll</u> relies on an
in-kernel shared context between its different calls. As a general overview:

<figure>
<img loading="lazy" src="{{"/assets/svgs/epoll.svg" | relative_url }}" alt="Overview of the epoll system calls" />
<figcaption>
    Overview of the <u>epoll</u> system calls. Notice how all of the calls
    interact with some instance through which state is shared. The instance is
    abstracted in the usual Unix way via a file descriptor (<b>epfd</b>).
    The epoll instance holds an <i>interest list</i> of file descriptors it monitors.
    File descriptors (or sockets in our case) are added (or removed, or
    modified) to the interest list via the <u>epoll_ctl</u>. The return value of
    <u>epoll_wait</u> is the length of the <i>ready list</i> -- a subset of the
    interest list referencing file descriptors that are ready for the requested
    I/O operation(s).
</figcaption>
</figure>

Before we proceed, it is important to note that <u>epoll</u> is NOT portable,
i.e., it is a Linux-only approach. For maximum portability, stick to POSIX
compliant <u>poll()</u>.

That's quite a long text -- you might be bored at the moment (rightfully so), so
here's the previous `multiserverchat` implemented using <u>epoll</u> (both
versions are semantically equivalent - but this is *arguably* faster). Copy, or
better, **re-write** the following  code into a `mutiserverchat_epoll.c` file:

{% highlight c linenos %}
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
{% endhighlight %}

Similarly to the previous poll-based `multichatserver` program, to compile the
`multichatserver_epoll` application, run:

   ```bash
   cc -o multiserverchat_epoll multiserverchat_epoll.c sockethelpers.c
   ```

1. Run the `multiserverchat_epoll` executable in a terminal:

   ```bash
   ./multiserverchat 9040
   ```

1. In multiple other terminals, run:

   ```bash
   telnet localhost 9040
   ```

1. Write messages in each terminal and see other terminals receiving it. Try
exiting some terminals to see the "user X disconnected" messages.

You have just successfully created an *arguably* more performant multi chat-room
server. Go through the source code in detail and pay attention to the comments.

#### One Thread Per Connection Model

#### More Advanced Combined Approaches

TODO (will only mention this very, very briefly since there are quite a lot of
details here)

### The C10K Problem

### The C10M Problem

TODO: this is only discussed

### Serialization/Deserialization

So far we have only sent ANSI

## Unix Domain Sockets

TODO

[tcpip_vs_osi]: https://networkengineering.stackexchange.com/questions/6380/osi-model-and-networking-protocols-relationship?noredirect=1&lq=1
[computer_networks_a_systems_approach]: https://book.systemsapproach.org/index.html
[c10m]: https://highscalability.com/the-secret-to-10-million-concurrent-connections-the-kernel-i
[io_modes]: https://notes.shichao.io/unp/ch6/
