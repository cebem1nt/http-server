# http-server

This is a bare http server implementation that returns any file based on requested route. This project is more of an investigation, used to figure out and show how sockets work.

## How does it work?

Basically creating an http server in c comes down to calling some syscalls, setup sockets communication and just wait for any client messages. Then parse http requests and send responses. Sockets by themselves are just "magical" file descriptors, the communication between clients is just read from file, write to a file.

## Steps

I always forget the steps needed to setup sockets, so I'll document them here

> [!NOTE]
> **All the given syscalls might return -1 in case of error. Don't forget to check that**

```c
#include <sys/socket.h>
...
// int socket(int domain, int type, int protocol);
int server_fd = socket(AF_INET, SOCK_STREAM, 0);
...
```

> `man 2 socket`
> The `socket` syscall creates an endpoint for a communication

Allrigth, what does that mean? For the linux kernel, sockets are communicational endpoints, the file descriptor `socket()` returns is purely internal, it is needed for the kernel.

### Params

- `int domain`
    This is the protocol family used for the socket, there are a plenty of them: `AF_INET` stands for IPv4, `AF_INET6` is IPv6, `AF_UNIX` is for local communication on your system
- `int type`
    This is the way communication will be driven, `SOCK_STREAM` means endless reliable two-way communication.
- `int protocol`
    The protocol used inside of the family (domain), there is generally only one, so provide 0 to select the one available

```c
struct sockaddr_in server_addr = {0};

server_addr.sin_family = AF_INET;
server_addr.sin_addr.s_addr = htonl(INADDR_ANY); // Accept connection on any local address
server_addr.sin_port = htons(8888);
```

Next step is to create a structure with additional info: the port our server will use, the address on which we will receive connections and the protocol hamily we use.

> `hton`, `ntoh` functions are used to convert bytes order. 
>   `h to n` -> host order to network order
>   `n to h` -> network order to host order
> additional s or l are used to identify input type: _short_ or _long_

- `INADDR_ANY`
    constant used to define that we accept connections on any addres the server machine has.
    (Ethernet, Wi‑Fi, virtual adapters, etc.)

```c
...
// int bind(int sockfd, const struct sockaddr *addr,
//          socklen_t addrlen);
bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr));
...
```

> `man 2 bind`
> bind() assigns the address specified by addr to the socket referred
> to by the file descriptor sockfd. addrlen specifies the size, in bytes, 
> of the address structure pointed to by addr. 
> Traditionally, this operation is called “assigning a name to a socket”.

```c
...
// int listen(int sockfd, int backlog);
listen(server_fd, 10);
...
```

`listen` just tells the kernel to mark socket as ready to acept connections. The `backlog` arguments tells how much pending connections there might be in the queue.

```c
struct sockaddr client_addr = {0}; // Structure used to store info about client
socklen_t client_addr_s = sizeof(client_addr);

int client_fd = accept(server_fd, &client_addr, &client_addr_s);
```

Finally the `accept` syscall will wait until there is a client, store the information about it into given `struct sockaddr` (`client_addr` in the example above) and return a **file descriptor referring to a communication point with that client.**

After this you can `read` and `write` into `client_fd` as if it would be a regular file. 
