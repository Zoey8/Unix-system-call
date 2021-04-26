#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/event.h>
#include <sys/types.h>
#define MAX_EVNETS 2000

int kqueueServer(){
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
    int kq = kqueue();
    struct kevent events[MAX_EVNETS];
    struct kevent event_addition;
    EV_SET(&event_addition, sock, EVFILT_READ, EV_ADD, 0, 0, NULL);
    kevent(kq, &event_addition, 1, NULL, 0, NULL);
    while(true){
        int ready_num = kevent(kq, NULL, 0, events, MAX_EVNETS, NULL);
        if(ready_num == -1){
            printf("kevent error: %d", errno);
            return 1;
        }
        sockaddr_in clientAddr = {};
        socklen_t nAddrLen = sizeof(sockaddr_in);
        char firstMessage[] = "Hello, I'm server! Please send messages!\n";
        for(int i = 0; i < ready_num; ++i){
            struct kevent current_event = events[i];
            if(current_event.ident == sock){
                int connected_sock = accept(sock, (sockaddr*) &clientAddr, &nAddrLen);
                printf("connected socket fd：%d\n", connected_sock);
                if(write(connected_sock, firstMessage, sizeof(firstMessage)) == -1){
                    printf("write error: %d\n", errno);
                    return 1;
                }
                EV_SET(&event_addition, connected_sock, EVFILT_READ, EV_ADD, 0, 0, NULL);
                kevent(kq, &event_addition, 1, NULL, 0, NULL);
            }else{
                int connected_sock = (int) current_event.ident;
                char buffer[100];
                long nread;
                if((nread = read(connected_sock, buffer, sizeof(buffer))) == 0){
                    close(connected_sock);
                    printf("connection closed by client\n");
                    EV_SET(&event_addition, current_event.ident, EVFILT_READ, EV_DELETE, 0, 0, NULL);
                    kevent(kq, &event_addition, 1, NULL, 0, NULL);
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
            }
        }
    }
    return 0;
}

int main(){
    return kqueueServer();
}
