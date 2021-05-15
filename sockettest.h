#ifndef __mSOCKET_H__
#define __mSOCKET_H__
#include <netdb.h>
typedef enum {
    FALSE = 0,
    TRUE = 1
} boolean;

int sock_connect_with_timeout(int sock, const struct addrinfo *ai, uint timeout_secs);
int sock_connect_by_getaddrinfo(char *hostname, char *port);

int socket_send(int sock_fd, char *buf);
int socket_recv(int sock_fd, char *buf);
boolean select_check_io(int fd);
int set_nonblock_mode(int sock_fd);
#endif //__SOCKET_H__
