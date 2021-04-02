#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>

int client(){
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if(sock == -1){
        printf("socket error\n");
        return 1;
    }
    sockaddr_in clientAddr = {};
    clientAddr.sin_family = AF_INET;
    clientAddr.sin_port = htons(1234);
    clientAddr.sin_addr.s_addr = INADDR_ANY;
    int ret = connect(sock, (sockaddr*) &clientAddr, sizeof(clientAddr));
    if(ret == -1){
        printf("connect error");
    }
    
}
