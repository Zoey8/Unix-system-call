#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

/**
 注意：无论是select、poll还是epoll，使用单进程时，都不是具有并行能力的服务器
 他们实现的功能仅仅只是IO复用，也可以被看做网络通信中的时分复用
 */
int selectServer(){
    int sock;
    if((sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1){
        printf("socket error\n");
        return 1;
    }
    printf("server socket fd：%d\n", sock);
    sockaddr_in serverAddr = {};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(8080);
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    if(bind(sock, (sockaddr*) &serverAddr, sizeof(serverAddr)) == -1){
        printf("bind error: %d\n", errno);
        return 1;
    }
    if(listen(sock, 20) == -1){
        printf("listen error: %d\n", errno);
        return 1;
    }
    /**
     新建一个位图表示一个集合，保存select函数返回的处于可读状态的文件描述符
     fd_set是个整数数组，用每个bit位来表示fd的状态
     比如，一个32bit的整数，则数组第一个整数表示0-31的fd，第二个整数表示32-63的fd，以此类推
     最大能表示的fd数量为1024，位图内容为此文件描述符的事件（这里为读事件）是否被监听，用1或0表示
     typedef struct fd_set {
         __int32_t       fds_bits[__DARWIN_howmany(__DARWIN_FD_SETSIZE, __DARWIN_NFDBITS)];
     } fd_set;
     */
    fd_set read_ready_set;
    /**
     新建一个位图表示一个集合，保存正在被监听可读事件的所有文件描述符
     */
    fd_set all_listend_read_set;
    /**
     connected_socks列表不是必须的，但它可以帮助显著减少遍历文件描述符的个数，列表中保存了需要监听的已连接套接字文件描述符
     如果没有这个列表，则需要每次需要遍历从0到max_fd的文件描述符，如果中间有许多不需要监听的文件描述符，则会浪费性能
     */
    int connected_socks[__DARWIN_FD_SETSIZE];
    /**
     列表中保存的需要监听的文件描述符的下标的最大值
     */
    int max_index = -1;
    /**
     被监听的文件描述符的最大值
     */
    int max_fd = sock;
    for(int i = 0; i < __DARWIN_FD_SETSIZE; ++i){
        connected_socks[i] = -1;
    }
    /**
     将读事件监听集合全部清空
     */
    __DARWIN_FD_ZERO(&all_listend_read_set);
    /**
     将创建的被动套接字加入读事件监听集合
     */
    __DARWIN_FD_SET(sock, &all_listend_read_set);
    sockaddr_in clientAddr = {};
    socklen_t nAddrLen = sizeof(sockaddr_in);
    char firstMessage[] = "Hello, I'm server! Please send messages!\n";
    while(true){
        /**
         巧妙的传参给read_set，保证all_listend_read_set不会受select函数改变
         */
        read_ready_set = all_listend_read_set;
        /**
         参数列表：
         nfds:监控的文件描述符集里最大文件描述符加1，此参数会告诉内核检测从0开始前多少个文件描述符的状态
         readfds：传入为需要监听读事件的文件描述符集合，传出为可读的文件描述符集合
         writefds：传入为需要监听写事件的文件描述符集合，传出为可写的文件描述符集合
         exceptfds：传入为需要监听异常事件的文件描述符集合，传出为发生异常的文件描述符集合
         timeout：定时阻塞监控时间，NULL为永久阻塞，大于0为等待时间，等于0为非阻塞直接返回
         
         select函数使用要注意文件描述符传入和传出的意义是不同的，注意在之前暂存监听集合
         select函数返回后，read_set已经变为可读文件描述符集合
         */
        int ready_num = select(max_fd + 1, &read_ready_set, NULL, NULL, NULL);
        if(ready_num < 0){
            printf("select error: %d\n", errno);
            return 1;
        }
        /**
         FD_ISSET函数用来判断一个文件描述符是否属于一个集合，这里是判断之前创建的被动套接字是否处于可读状态
         */
        if(__DARWIN_FD_ISSET(sock, &read_ready_set)){
            int connected_sock = accept(sock, (sockaddr*) &clientAddr, &nAddrLen);
            printf("connected socket fd：%d\n", connected_sock);
            if(write(connected_sock, firstMessage, sizeof(firstMessage)) == -1){
                printf("write error: %d\n", errno);
                return 1;
            }
            for(int i = 0; i < __DARWIN_FD_SETSIZE; ++i){
                if(connected_socks[i] == -1){
                    connected_socks[i] = connected_sock;
                    if(connected_sock > max_fd){
                        max_fd = connected_sock;
                    }
                    if(i > max_index){
                        max_index = i;
                    }
                    break;
                }
            }
            /**
             将已连接套接字加入被监听读事件的集合，很明显这里要加入到all_listend_read_set中
             */
            __DARWIN_FD_SET(connected_sock, &all_listend_read_set);
            if(--ready_num == 0){
                continue;
            }
        }
        for(int i = 0; i <= max_index; ++i){
            int connected_sock;
            if((connected_sock = connected_socks[i]) < 0){
                continue;
            }
            char buffer[100];
            long nread;
            /**
             判断已连接套接字是否处于可读状态
             */
            if(__DARWIN_FD_ISSET(connected_sock, &read_ready_set)){
                if((nread = read(connected_sock, buffer, sizeof(buffer))) == 0){
                    close(connected_sock);
                    printf("connection closed by client\n");
                    /**
                     断开连接时需要将相应的文件描述符从被监听读事件的集合中移除
                     */
                    __DARWIN_FD_CLR(connected_sock, &all_listend_read_set);
                    connected_socks[i] = -1;
                    break;
                }else if(nread < 0){
                    printf("read error: %d", errno);
                    return 1;
                }else{
                    if(buffer[nread] != '\0'){
                        buffer[nread] = '\0';
                    }
                    printf("receive a message: %s, length: %zu\n", buffer, strlen(buffer));
                    /**
                     实现功能：服务端将客户端发送的数据原样返回给客户端
                     */
                    if(write(connected_sock, buffer, nread) == -1){
                        printf("write error: %d\n", errno);
                        return 1;
                    }
                }
                if(--ready_num == 0){
                    break;
                }
            }
        }
    }
    return 0;
}
