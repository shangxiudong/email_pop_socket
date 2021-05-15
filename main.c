#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sockettest.h"

#define BUFFSIZE 1024

int main(int argc,const char*argv[])
{
    if(argc<3)
    {
        printf("too few argument\n");
        printf("<host address> <pop port:110>\n");
        return 0;
    }
    char *hostname = NULL, *port = NULL;
    char buf[BUFFSIZE];
    memset(buf, 0, BUFFSIZE);
    hostname = strdup(argv[1]);
    port = strdup(argv[2]);
    int sock = sock_connect_by_getaddrinfo(hostname, port);
    strncpy(buf, "USER bripengandre", 18);
    socket_send(sock, buf);
    memset(buf, 0, BUFFSIZE);
    socket_recv(sock, buf);
    printf("buf:%s\n", buf);
    free(hostname);
    free(port);
    return 0;
}
