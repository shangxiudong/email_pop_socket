#include "sockettest.h"
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
static uint io_timeout = 180;
#define INVALID_SOCKET -1
int sock_connect_with_timeout(int sock,  const struct addrinfo *ai, uint timeout_secs)
{
    set_nonblock_mode(sock);
    int ret = connect(sock, ai->ai_addr, ai->ai_addrlen);

    if(ret < 0) {
        if(EINPROGRESS == errno) { //Linux中系统调用的错误都存储于 errno中，errno由操作系统维护，存储就近发生的错误，即下一次的错误码会覆盖掉上一次的错误。
            fd_set fds; //建立一个文件描述符集合
            FD_ZERO(&fds); //将 fds集合 清空
            FD_SET(sock, &fds); //将sock加入到 fds 集合中
            //设置select的超时时间
            struct timeval tv;
            tv.tv_sec = timeout_secs; //超时秒
            tv.tv_usec = 0; //超时毫秒
            do {
                /**
                select()机制中提供一fd_set的数据结构，实际上是一long类型的数组，每一个数组元素都能与一打开的文件句柄（不管是socket句柄，还是其他文件或命名管道或设备句柄）建立联系，建立联系的工作由程序员完成，当调用select()时，由内核根据IO状态修改fd_set的内容，由此来通知执行了select()的进程哪一socket或文件发生了可读或可写事件。
                int maxfdp是一个整数值,是指集合中所有文件描述符的范围,即所有文件描述符的最大值加1，不能错!在 Windows中这个参数的值无所谓,可以设置不正确。
                fd_set* readfds是指向fd_set结构的指针,这个集合中应该包括文件描述符,我们是要监视这些文件描述符的读变化的,即我们关心是否可以从这些文件中读取数据了,如果这个集合中有一个文件可读, select就会返回一个大于0的值,表示有文件可读,如果没有可读的文件
                则根据 timeout参数再判断是否超时,若超岀 timeout的时间, select返回0,若发生错误返回负值。可以传入NULL值,表示不关心任何文件的读变化
                fd_set* writefds是指向fd_set结构的指针,这个集合中应该包括文件描述符,我们是要监视这些文件描述符的写变化的,即我们关心是否可以向这些文件中写入数据了,如果这个集合中有一个文件可写,select就会返回一个大于0的值,表示有文件可写,如果没有可写的文件，则根据 timeout参数再判断是否超时,若超岀 timeout的时间, select返回0,若发生错误返回负值。可以传入NULL值,表示不关心任何文件的写变化。
                fd set* errorfds同上面两个参数的意图,用来监视文件错误异常。
                struct timeval timeout是select的超时时间,这个参数至关重要,它可以使select处于三种状态,第一,若将NULL以形参传入,即不传入时间结构,就是将 select置于阻塞状态,一定等到监视文件描述符集合中某个文件描述符发生变化为止;第二,若将时间值设为0秒0毫
                秒,就变成一个纯粹的非阻塞函数,不管文件描述符是否有变化,都立刻返回继续执行,文件无变化返回0,有变化返回一个正值;第三, timeout的值大于0,这就是等待超时时间，即select在 timeout时间内阻塞,超时时间之内有事件到来就返回了,否则在超时后不管怎样
                定返回,返回值同上述。
                */
                ret = select(sock + 1, NULL, &fds, NULL, &tv);
            }while(ret < 0 && EINTR == errno);
            if(ret < 0) {
                perror("select() encountered something unexpected\n");
                return -1;
            } else if(ret == 0) {
                perror("select() timeout\n");
                return -1;
            } else {
                if(!FD_ISSET(sock, &fds)) { //检查sock是否真的添加到fds中监听了
                    perror("fd is not in select set\n");
                    return -1;
                }
                int val;
                uint len;
                len = sizeof (val);
                getsockopt(sock, SOL_SOCKET, SO_ERROR, &val, &len);
                printf("getsockopt() return error:%s\n", strerror(val));
                return -1;
            }
        }
    }
    return 0;
}

int sock_connect_by_getaddrinfo(char *hostname, char *port)
{
    struct addrinfo hints, *res, *ai;
    hints.ai_flags = AI_CANONNAME;
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    int gai_error;
    int sock = INVALID_SOCKET;
    if((gai_error = getaddrinfo(hostname, port, &hints, &res)) != 0) {
        fprintf(stderr, "getaddrinfo error\n");
    }
    for(ai = res; ai != NULL; ai = ai->ai_next) {
        //初始化socket
        sock = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
        if(sock < 0) {
            continue;
        }
        //尝试连接服务器， 只要有一个成功，那么就退出连接循环
        if(sock_connect_with_timeout(sock, ai, io_timeout) == 0) {
            break;
        }
        //没连接成功， 关闭当前的sock
        close(sock);
    }
    if(res != NULL) {
        freeaddrinfo(res);
    }
    //循环到最后也没有连接成功
    if(ai == NULL) {
        return INVALID_SOCKET;
    }
    return sock;
}

int socket_send(int sock_fd, char *buf)
{
    if(select_check_io(sock_fd) == TRUE) {
        long ret = write(sock_fd, buf, strlen(buf));
        return (int)ret;
    }
    printf(stdout, "socket is not in connected\n");
    return -2;
}

int socket_recv(int sock_fd, char *buf)
{
    if(select_check_io(sock_fd) == TRUE) {
        return  (int)read(sock_fd, buf, strlen(buf));
    }
    printf(stdout, "socket is not in connected\n");
    return -2;
}

int set_nonblock_mode(int sock_fd)
{
    // 获取sock_fd的阻塞模式
    int flag = fcntl(sock_fd, F_GETFL, 0);
    flag |= O_NONBLOCK; //或运算， 将flag加上非阻塞位
    // flag &= ~O_NONBLOCK; //这里是设置阻塞模式
    //将修改成非阻塞模式的flag设置回去
    return  fcntl(sock_fd, F_SETFL, flag);

}

boolean select_check_io(int fd)
{
    struct timeval timeout;
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(fd, &fds);
    timeout.tv_sec = 60; //阻塞时间1min
    timeout.tv_usec = 0;
    select(fd + 1, &fds, NULL, NULL,
           &timeout);
    if(FD_ISSET(fd, &fds)) {
        return  TRUE;
    } else {
        return FALSE;
    }
}
