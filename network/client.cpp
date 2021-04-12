#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string>

int client(){
    /**
     建立socket，第三个参数无需声明使用TCP连接
     方法会返回一个非负的文件描述符，返回-1代表出错
     */
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if(sock == -1){
        printf("socket error\n");
        return 1;
    }
    /**
     连接前需要指定服务端的地址，须与服务端一致
     与服务端不同，客户端不需要bind，内核会自动使用一个随机端口作为源端口
     */
    sockaddr_in serverAddr = {};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(8080);
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    /**
     connect方法的作用是建立连接，字节流套接字将使用三次握手建立连接，数据报套接字则不发送任何数据
     connect方法若连接成功则返回0，否则返回-1
     */
    int ret = connect(sock, (sockaddr*) &serverAddr, sizeof(serverAddr));
    if(ret == -1){
        printf("connect error");
        return 1;
    }
    while(true){
        char readBuffer[100];
        char writeBuffer[100];
        long nread;
        if((nread = read(sock, readBuffer, 100)) < 0){
            printf("read error");
            return 1;
        }
        if(readBuffer[nread] != '\0'){
            readBuffer[nread] = '\0';
        }
        printf("receive a message: %s\n", readBuffer);
        scanf("%s", writeBuffer);
        if(write(sock, writeBuffer, strlen(writeBuffer)) < 0){
            printf("write error");
            return 1;
        }
    }
    return 0;
}
