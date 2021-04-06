#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>

int bioServer() {
    /**
     建立一个socket，传入socket族（ipv4或ipv6），socket类型, 协议类型三个参数
     方法会返回一个非负的文件描述符，返回-1代表出错
    */
    int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    printf("server创建套接字的文件描述符为：%d\n", sock);
    if(sock == -1){
        printf("socket error\n");
        return 1;
    }
    /**
     构建服务端socket地址（最关键的就是ip地址和端口号），sockaddr_in的数据结构：
     struct sockaddr_in {
         __uint8_t       sin_len;
         sa_family_t     sin_family;    地址类型（一般为ipv4、ipv6）
         in_port_t       sin_port;      2字节端口号
         struct  in_addr sin_addr;      4字节ip地址
         char            sin_zero[8];   8字节填充
     }
     */
    sockaddr_in serverAddr = {};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(8080);
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    /**
     将socket地址绑定到之前创建的socket上，成功返回0，出错返回-1
     */
    if(bind(sock, (sockaddr*) &serverAddr, sizeof(serverAddr)) == -1){
        printf("bind error\n");
        return 1;
    }
    /**
     socket创建的套接字是主动套接字，调用listen后变成监听套接字。TCP状态从CLOSE跃迁到LISTEN状态
     TCP的三次握手是在客户端调用connect函数时完成的，服务器端不会调用connect函数，但是必须有套接字在某个端口监听
     不然会返回客户端RST，终止连接
     */
    if(listen(sock, 2) == -1){
        printf("listen error\n");
        return 1;
    }
    
    sockaddr_in clientAddr = {};
    socklen_t nAddrLen = sizeof(sockaddr_in);
    int connected_sock = -1;
    char firstMessage[] = "Hello, I'm server! Please send messages!";
    while(true){
        /**
         accept函数从已完成连接的队列中取走一个套接字，如果该队列为空，则accept函数阻塞
         accept函数的返回值称为已连接套接字文件描述符，这样就建立一个完整的TCP连接
         源IP地址，源端口号，目的IP地址，目的端口号都是唯一确定的
         服务器每次接受连接请求时都会创建一次已连接套接字，它只存在于服务器为一个客户端服务的过程中
         */
        connected_sock = accept(sock, (sockaddr*) &clientAddr, &nAddrLen);
        printf("已连接的套接字的文件描述符为：%d\n", connected_sock);
        if(connected_sock == -1){
            printf("connect error\n");
            return 1;
        }
        /**
         write方法将应用程序的数据写入TCP发送缓冲区，返回值大于或等于0时表示发送的字节数，为负则表示发送失败
         write执行成功，只是表示应用程序中的数据被成功复制到了kernel中的TCP发送缓冲区，不代表接收方已经收到数据
         write的数据需要暂存在发送缓冲区中，只有收到对方的ack后，kernel才清除这一部分已经被接收的数据腾出空间
         接收端将收到的数据暂存在TCP接收缓冲区中进行确认，但如果接收端socket所在的进程不及时将数据从接收缓冲区中取出
         则会导致接收缓冲区填满，由于TCP的滑动窗口和拥塞控制，接收端会阻止发送端向其发送数据。
         应用程序如果继续发送数据，最终会导致发送缓冲区无法完整存放应用程序发送的数据，write方法将会阻塞。
         */
        if(write(connected_sock, firstMessage, sizeof(firstMessage) + 1) < 0){
            printf("write error\n");
            return 1;
        }
        char buffer[100];
        long nread;
        /**
         read方法将TCP接收缓冲区中的内容复制到应用程序中，返回值大于或等于0时表示接收的字节数，为负则表示接收失败
         当接收缓冲区为空时，read方法将会阻塞，等待发送方发送数据
         */
        while((nread = read(connected_sock, buffer, 100)) > 0){
            if(buffer[nread] != '\0'){
                buffer[nread] = '\0';
            }
            printf("receive a message: %s\n", buffer);
            if(write(connected_sock, buffer, nread) < 0){
                printf("write error");
                return 1;
            }
        }
    }
    return 0;
}
