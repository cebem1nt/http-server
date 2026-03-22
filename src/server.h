#define PORT 8080
#define BACKLOG 10

#define N_ROUTES (sizeof(routes) / sizeof(routes[0]))

typedef struct {
    const char* route;
    const char* file;
} route_t;

const route_t routes[] = {
    { "/", "../index.html" }
};