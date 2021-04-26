#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <poll.h>
#define MAX_CONNECTED_SOCKS 2000

/**
 注意：无论是select、poll还是epoll，都不是具有并行能力的服务器
 仅仅只是IO复用，也可以被看做网络通信中的时分复用
 */
int pollServer(){
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
    struct pollfd connected_socks[MAX_CONNECTED_SOCKS];
    connected_socks[0].fd = sock;
    connected_socks[0].events = POLLRDNORM;
    int max_index = 0;
    for(int i = 1; i < MAX_CONNECTED_SOCKS; ++i){
        connected_socks[i].fd = -1;
    }
    sockaddr_in clientAddr = {};
    socklen_t nAddrLen = sizeof(sockaddr_in);
    char firstMessage[] = "Hello, I'm server! Please send messages!\n";
    while(true){
        int ready_num = poll(connected_socks, max_index + 1, -1);
        if(ready_num < 0){
            printf("select error: %d\n", errno);
            return 1;
        }
        if(connected_socks[0].revents & POLLRDNORM){
            int connected_sock = accept(sock, (sockaddr*) &clientAddr, &nAddrLen);
            printf("connected socket fd：%d\n", connected_sock);
            if(write(connected_sock, firstMessage, sizeof(firstMessage)) == -1){
                printf("write error: %d\n", errno);
                return 1;
            }
            for(int i = 1; i < MAX_CONNECTED_SOCKS; ++i){
                if(connected_socks[i].fd == -1){
                    connected_socks[i].fd = connected_sock;
                    connected_socks[i].events = POLLRDNORM;
                    if(i > max_index){
                        max_index = i;
                    }
                    break;
                }
            }
            if(--ready_num == 0){
                continue;
            }
        }
        for(int i = 0; i <= max_index; ++i){
            int connected_sock = connected_socks[i].fd;
            if(connected_sock == -1){
                continue;
            }
            char buffer[100];
            long nread;
            if(connected_socks[i].revents & POLLRDNORM){
                if((nread = read(connected_sock, buffer, sizeof(buffer))) == 0){
                    close(connected_sock);
                    printf("connection closed by client\n");
                    connected_socks[i].fd = -1;
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
