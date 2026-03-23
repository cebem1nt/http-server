#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define HTTPP_IMPLEMENTATION
#include "httpp.h"

#include "server.h"

#define error_exit(...) do {                \
    fprintf(stderr, __VA_ARGS__);           \
    exit(1);                                \
} while(0)

#define perror_exit(msg) do  {              \
    perror(msg);                            \
    exit(1);                                \
} while(0)

char* readfile(const char* fname, size_t* out_size) 
{
    char*  out = NULL;
    size_t fsize;
    int    fd;
    
    if ((fd = open(fname, O_RDONLY)) == -1) {
        perror("open");
        goto _exit;
    }

    if ((fsize = lseek(fd, 0, SEEK_END)) == -1) {
        perror("open");
        goto _exit;
    }

    if (lseek(fd, 0, SEEK_SET) == -1) {
        perror("open");
        goto _exit;
    }

    if ((out = (char*) malloc(fsize + 1)) == NULL) {
        goto _exit;
    }

    if (read(fd, out, fsize) == -1) {
        perror("read");
        free(out);
        goto _exit;
    }

    if (out_size)
        *out_size = fsize;

    out[fsize] = '\0';
_exit:
    close(fd);
    return out;
}

char* get_client_ip(struct sockaddr* client_addr) 
{
    struct sockaddr_in* client_addr_in = (struct sockaddr_in*) client_addr;
    return inet_ntoa(client_addr_in->sin_addr);
}

void handle_client(int client_fd) 
{
    char    buf[BUFSIZ];
    int     n;

    // TODO: this aint good
    // Assume that our buffer is big enough to fit any request (at once)
    if ((n = recv(client_fd, buf, BUFSIZ, 0)) == -1)
        perror_exit("recv");

    // Now parse http request
    HTTPP_NEW_REQ(req, 50);
    httpp_parse_request(buf, n, &req);

    char*   html = NULL;
    size_t  html_len;

    for (int i = 0; i < N_ROUTES; i++) {
        if (httpp_span_eq(&req.route, routes[i].route)) {
            html = readfile(routes[i].file, &html_len);
            break;
        }
    }

    HTTPP_NEW_RES(res, 50, html ? 200
                                : 404);

    char html_lenstr[10];
    snprintf(html_lenstr, 10, "%lu", html_len);

    httpp_res_add_header(&res, "Content-Type", "text/html; charset=utf-8");
    httpp_res_add_header(&res, "Content-Length", html_lenstr);

    httpp_res_set_body(res, html, html_len);
    
    size_t raw_len;
    char* raw = httpp_res_to_raw(&res, &raw_len);

    if (write(client_fd, raw, raw_len) == -1)
        perror_exit("write");

    free(raw);
    free(html);
}

int main() 
{
    int server_fd, client_fd;

    struct sockaddr_in server_addr = {0};
    struct sockaddr    client_addr = {0};

    socklen_t client_addr_s = sizeof(client_addr);

    // Setup server comunication endpoint
    // AF_INET     -> IPv4 family
    // SOCK_STREAM -> Stream comunication
    // 0           -> Sub protocol inside of AF_INET domain
    //                We have only one available, so 0
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        perror_exit("socket");

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY); // Accept connection on any local address
    server_addr.sin_port = htons(PORT);

    // Make socket reuse the same adress, 
    // if not provided, you'll have to wait each rerun 
    int opt = 1; // Reuse address
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    // Bind the socket (to be able to accept requests in future), provide our info
    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1)
        perror_exit("bind");

    // Mark as ready to connect.
    // BACKLOG -> max queue size for connections
    if (listen(server_fd, BACKLOG) == -1)
        perror_exit("listen");

    // Ignore exit codes of the children
    signal(SIGCHLD, SIG_IGN);
    printf("Listening on: %i\n", PORT);
 
    // Accept any client, assign comunication stream into client fd
    while ((client_fd = accept(server_fd, &client_addr, &client_addr_s))) {
        if (client_fd == -1)
            perror_exit("accept");

        char* client_ip = get_client_ip(&client_addr);
        printf("New client: %s\n\n", client_ip);
        
        if (fork() == 0) {  // Fork and make child handle the client
            close(server_fd);
            handle_client(client_fd);
            close(client_fd);
            _exit(0);
        }

        close(client_fd);
    }

    return 0;
}