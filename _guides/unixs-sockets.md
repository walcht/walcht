---
title: Brief Introduction to Unix Sockets Network Programming
short_desc: A brief introduction to network programming using IP and Unix domain sockets
layout: home
---

# Sockets Programming

!!WORK-IN-PROGRESS GUIDE!!

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

## Sockets in General

Following the Unix philosophy of *everything is a file*, a socket is essentially
a file descriptor that can be written to and read from for the purpose of
network communication. Consequently, a socket is subject
to the usual file permissions. A socket represents the local endpoint of a
communication path.

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
<img loading="lazy" src="/assets/svgs/unix_domain_sockets_vs_ip_sockets.svg" alt="Diagram demonstrating the difference between IP sockets and Unix domain sockets" />
<figcaption>
    IP sockets vs. Unix domain sockets. In <u>Machine A</u>, <u>Process A.0</u>
    communicates with <u>Process A.1</u> via the pair of its Unix domain socket
    <u>sock_fd A.0</u> and <u>Process A.1</u>'s <u>sock_fd A.1</u>. On the other
    hand, <u>Process A.0</u> communicates with the remote <u>Process B.0</u> on
    the very far away <u>Machine B</u> via TCP/IP.
</figcaption>
</figure>

## Networking Basics

Before delving into sockets programming details, basic networking concepts
have to be introduced.

### TCP/IP

The internet protocol suite is a set of standardized communication protocols
across a stack of layers where each layer encapsulates/de-encapsulate the data.
The stack of layers standardized by TCP/IP are:

- **Application Layer** - 
- **Transport Layer (or Host-to-Host Layer)** - concerns itself with data
integrity, chunking packets from the application layer, and service quality.
The main protocols are TCP and UDP.
- **Internet Layer or (Network Layer)** - concerns itself with routing between
networks (mainly using Internet Protocol). This is where routing tables come
into play.
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
<img loading="lazy" src="/assets/images/traceroute_example.jpeg" alt="Output of command: sudo traceroute -I example.com" />
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
> below in the same way, it shouldn’t matter how it’s implemented. For example, we
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

## Basic Networking Terminology

## Unix Domain Sockets

TODO

## Internet Domain Sockets (IP Sockets)

### Creating TCP Client Programs

Usually client programs initiate a network connection with some server
to request services. Clients also usually terminate such connections.
Often, a client is activated by a user (e.g., opening a web page in a
browser tab) and is disabled automatically or when the user explicitly
deactivates it (e.g., closing a web page by closing its browser tab).

The usual Sockets API calls to perform in a client program are as follows:

- <u>connect()</u> has the same signature as <u>bind()</u> and specifies
 the address and port of remote side of the connection
    ```C
    int connect(int sockfd, const struct sockaddr *addr,
        socklen_t addrlen);
    ```

### Creating TCP Server Programs

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
(each Sockets API call will be explained later on):

```C
#define PORT "3490"  /* this server's port */
#define BACKLOG 10   /* max number of connections to be help in the backlog */

int list_sockfd, conn_sockfd;      /* listening and connection socket fds */
int rv;                            /* return value - always check for success */
struct addrinfo hints, *servinfo;  /* hints for getaddrinfo() and servinfo is
                                    * its output */

/* Fill up the addrinfo hints by choosing IPv4 vs. IPv6, UDP vs. TCP and the
 * target IP address. The AI_PASSIVE flag means "hey, look for this host IPs
 * that can be used to accept connections (i.e., bind() calls) */
memset(&hints, 0, sizeof(hints));
hints.ai_family = AF_INET;          /* only IPv4 */
hints.ai_socktype = SOCK_STREAM;    /* TCP */
hints.ai_flags = AI_PASSIVE;        /* host IPs */

/* get socket addresses for the provided hints - in this case, since node is
 * NULL and AI_PASSIVE flag is set, the returned sockets are suitable for
 * bind() calls (i.e., suitable for server applications to accept connections
 * or recvieve data using recvfrom) */
s = getaddrinfo(NULL, PORT, &hints, &servinfo);
if (s != 0) { /* error handling here - use gai_strerror() */ }

/* loop through the linked list res until you have successfully created the
 * connection socket and bound it */
for (p = servinfo; p != NULL; p = p->ai_next) {
    // try to create a socket for the current addrinfo candidate
    conn_sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);

    // check if socket creation failed
    if (conn_sockfd == -1) {
        /* check errno */
        continue;
    }

    /* try to bind the created socket to this host's IP and a sepcified port
     * p->ai_addr contains this host's IP address and the port */
    rv = bind(list_sockfd, p->ai_addr, p->ai_addrlen);
    if (rv == 0)
      break; /* success */

    // failed to bind (check errno) - do NOT forget to close the socket fd!
    close(list_sockfd);
}

// remember to call this to avoid a memory leak!
freeaddrinfo(servinfo);

// check whether we were successful in finding a candidate
if (p == NULL) { /* failed to find a candidate */ }

/* start listening for connection requests (client side programs can now
 * connect to this socket by calling, you guessed it, connect()) */
rv = listen(list_sockfd, BACKLOG);
if (rv == -1) { /* error handling here */ }

// wait for the client to connect ...
conn_sockfd = accept(list_sockfd, /* output args here */ );
if (conn_sockfd == -1) { /* handle error */ }

/* in this example, we expect only one client to connect - so we no longer
 * need the listening socket */
clost(list_sockfd);

/* send may in fact not send the entirety of your data and it is your
 * reponsibility to keep re-sending until all chunks of your message are sent.*/
rv = send(conn_sockfd, "Hi client - you have successfully connected!", 13, 0);
if (rv == -1) { /* handle error */ }
else if (rv == 0) { /* this means client connection is closed */ }

// do NOT forget to close the connection sockfd!
close(conn_sockfd);
```

This may seem like a lot but every socket API call will be explained - just
read it briefly and try to remember the API call orders because they are
almost *always* called in that order.

It should be noted however that this is not exactly the optimal way to employ
a server that is expected to connect with multiple clients (as is usually the
case for most applications). Certain socket API calls are blocking (e.g., accept()).
We will see later on a snippet for how a non-blocking server is usually implemented.

In more details:

1. <u>getaddrinfo()</u> provided a set of hints, gets candidate address
structures (you should loop through them until a suitable candidate is found):

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

   <u>freeaddrinfo()</u> has to be called to cleanup the allocated <u>res</u>
   <u>addrinfo</u> structure(s).
  

2. <u>socket()</u> creates the **listening socket** which is used to listen for
 connection requests from clients. Notice in the code snippet above that there
 are two sockets - one for listening to connections and one for sending and
 receiving data from the connected client:

   ```C
   int socket(int domain, int type, int protocol);
   ```

3. <u>bind()</u> specifies the address and port of local side of the connection:

   ```C
   int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
   ```

   On the server side, since clients initiate the connection, the
   port has to be explicitly set and has to be "well-known" (or "agreed-upon")
   so that the client can connect to the server. Therefore <u>bind()</u> has to
   be called.

   On the client side, it is not necessary to call bind as the OS
   will automatically assign an available port number upon <u>connect()</u>.
   Since the client is the side that initiate the connection, its
   *implicitly* set port will be sent to the server and the server will
   know how to reach the client (think about it - the server's endpoint has to
   be known beforehand so that the client can connect to it!).

4. <u>listen()</u> starts listening for connections. After calling this, the
potential client applications can initiate a connection request to the server.

   ```C
   int listen(int sockfd, int backlog);
   ```
   
   <u>backlog</u> is simply the maximum number of pending requests that can be
   put in the queue (for the non-English native speakers there, backlog is an
   accumulation of uncompleted work or matters needing to be dealt with).
   
   To be more precise, <u>listen()</u> marks the socket as a *passive socket*
   that will be used to accept incoming connection requests using
   <u>accept()</u>.

5. <u>accept()</u> is used with connection-based sockets (e.g., **SOCK_STREAM**)
to extract the first connection request from the backlog queue. A new socket
for communicating with the client is create and returned - such socket is
usually referred as a **connection socket**. Accept is blocking (unless the
sockfd is adjusted to a non-blocking socket).

   ```C
   int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
   ```

6. <u>send()</u> and <u>recv()</u> are used to handle requests and send
 responses to the client through the newly created **connection socket**
 (depending on the number of connections the server is expected to concurrently
 handle, we might spawn a new process/thread or we might use an event-driven,
 worker-based model):

   ```C
   #include <sys/socket.h>

   ssize_t send(int sockfd, const void buf[.len], size_t len, int flags);
   ```

   **RETURN VALUE**: on success, send returns the number of bytes sent which
   might be less than <u>len</u>. -1 is returned on failure and <u>errno</u>
   is set.

7. <u>close()</u> is used to close the **connection socket** upon, for instance,
client connection closure (when <u>recv</u> returns 0):

   ```C
   #include <unistd.h>
   
   int close(int fd);
   ```

8. go to 5.) and repeat


<figure>
<img loading="lazy" src="/assets/svgs/internet_sockets.svg" alt="" />
<figcaption>
</figcaption>
</figure>

[tcpip_vs_osi]: https://networkengineering.stackexchange.com/questions/6380/osi-model-and-networking-protocols-relationship?noredirect=1&lq=1
