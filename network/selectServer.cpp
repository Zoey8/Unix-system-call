#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

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
     新建一个位图，保存监听可读事件的文件描述符。fd_set是个整数数组，用每个bit位来表示fd
     比如，一个32位整数，则数组第一个整数表示0-31的fd，第二个整数表示32-63的fd，以此类推
     最大能表示的fd数量为1024
     typedef struct fd_set {
         __int32_t       fds_bits[__DARWIN_howmany(__DARWIN_FD_SETSIZE, __DARWIN_NFDBITS)];
     } fd_set;
     */
    fd_set read_set;
    /**
     connected_socks列表不是必须的，但它可以显著减少遍历的个数，列表中保存了需要监听的文件描述符
     */
    int connected_socks[__DARWIN_FD_SETSIZE];
    int max_index = -1;
    int max_fd = sock;
    for(int i = 0; i < __DARWIN_FD_SETSIZE; ++i){
        connected_socks[i] = -1;
    }
    __DARWIN_FD_ZERO(&read_set);
    __DARWIN_FD_SET(sock, &read_set);
    sockaddr_in clientAddr = {};
    socklen_t nAddrLen = sizeof(sockaddr_in);
    char firstMessage[] = "Hello, I'm server! Please send messages!\n";
    while(true){
        int ready_num = select(max_fd + 1, &read_set, NULL, NULL, NULL);
        if(ready_num < 0){
            printf("select error: %d\n", errno);
            return 1;
        }
        if(__DARWIN_FD_ISSET(sock, &read_set)){
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
            __DARWIN_FD_SET(connected_sock, &read_set);
            for(int i = 0; i <= max_index; ++i){
                int connected_sock;
                if((connected_sock = connected_socks[i]) < 0){
                    continue;
                }
                char buffer[100];
                long nread;
                if(__DARWIN_FD_ISSET(connected_sock, &read_set)){
                    while(true){
                        if((nread = read(connected_sock, buffer, sizeof(buffer))) == 0){
                            close(connected_sock);
                            printf("connection closed by client\n");
                            __DARWIN_FD_CLR(connected_sock, &read_set);
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

int main(){
    return selectServer();
}
