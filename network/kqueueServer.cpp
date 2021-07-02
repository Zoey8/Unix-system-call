#include <cstdio>
#include <cstdlib>
#include <netinet/in.h>
#include <sys/socket.h>
#include <cstring>
#include <unistd.h>
#include <cerrno>
#include <sys/event.h>
#define MAX_EVENTS 2000

/**
 注意：无论是select、poll还是epoll（kqueue），使用单进程时，都不是具有并行能力的服务器
 他们实现的功能仅仅只是IO复用，也可以被看做网络通信中的时分复用
 */
int kqueueServer(){
    int sock;
    if((sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1){
        printf("socket error\n");
        return 1;
    }
    printf("server socket fd：%d\n", sock);
    sockaddr_in server_addr = {};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    if(bind(sock, (sockaddr*) &server_addr, sizeof(server_addr)) == -1){
        printf("bind error: %d\n", errno);
        return 1;
    }
    if(listen(sock, 20) == -1){
        printf("listen error: %d\n", errno);
        return 1;
    }
    int kq = kqueue();
    struct kevent events[MAX_EVENTS];
    struct kevent event_addition;
    EV_SET(&event_addition, sock, EVFILT_READ, EV_ADD, 0, 0, NULL);
    kevent(kq, &event_addition, 1, NULL, 0, NULL);
    while(true){
        int ready_num = kevent(kq, NULL, 0, events, MAX_EVENTS, NULL);
        if(ready_num == -1){
            printf("kevent error: %d", errno);
            return 1;
        }
        sockaddr_in client_addr = {};
        socklen_t client_addr_length = sizeof(sockaddr_in);
        char first_message[] = "Hello, I'm server! Please send messages!\n";
        for(int i = 0; i < ready_num; ++i){
            struct kevent current_event = events[i];
            if(current_event.ident == sock){
                int connected_sock = accept(sock, (sockaddr*) &client_addr, &client_addr_length);
                printf("connected socket fd：%d\n", connected_sock);
                if(write(connected_sock, first_message, sizeof(first_message)) == -1){
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
                    EV_SET(&event_addition, connected_sock, EVFILT_READ, EV_DELETE, 0, 0, NULL);
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
}
