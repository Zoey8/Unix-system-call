#include <cstdio>
#include <cstdlib>
#include <netinet/in.h>
#include <sys/socket.h>
#include <cstring>
#include <unistd.h>
#include <cerrno>

int bioServer() {
    int sock;
    /**
     建立一个socket，传入socket族（ipv4或ipv6），socket类型, 协议类型三个参数
     如果成功，方法会返回一个非负的文件描述符，如果返回-1代表出错
    */
    if((sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1){
        printf("socket error\n");
        return 1;
    }
    printf("server socket fd：%d\n", sock);
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
    sockaddr_in server_addr = {};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    /**
     将socket地址绑定到之前创建的socket上，成功返回0，出错返回-1
     */
    if(bind(sock, (sockaddr*) &server_addr, sizeof(server_addr)) == -1){
        printf("bind error: %d\n", errno);
        return 1;
    }
    /**
     socket创建的套接字是主动套接字，调用listen后变成监听套接字(被动套接字)。TCP状态从CLOSE跃迁到LISTEN状态
     TCP的三次握手是在客户端调用connect函数时完成的，服务器端不会调用connect函数，但是必须有套接字在某个端口监听
     不然会返回客户端RST，终止连接
     listen函数的两个参数为socket的文件描述符和已连接队列的长度(与accept函数有关)
     */
    if(listen(sock, 3) == -1){
        printf("listen error: %d\n", errno);
        return 1;
    }
    sockaddr_in client_addr = {};
    socklen_t client_addr_length = sizeof(sockaddr_in);
    char first_message[] = "Hello, I'm server! Please send messages!\n";
    while(true){
        int connected_sock;
        /**
         accept函数从已完成连接的队列中取走一个套接字，返回文件描述符，如果该队列为空，则accept函数阻塞
         accept函数的返回值称为已连接套接字文件描述符，这样就建立一个完整的TCP连接
         源IP地址，源端口号，目的IP地址，目的端口号都是唯一确定的
         服务器每次接受连接请求时都会创建一次已连接套接字，它只存在于服务器为一个客户端服务的过程中
         */
        if((connected_sock = accept(sock, (sockaddr*) &client_addr, &client_addr_length)) == -1){
            /**
             我们用慢系统调用来描述那些可能永远阻塞的系统调用（函数调用），如：accept，read等
             永远阻塞的系统调用是指调用有可能永远无法返回，多数网络函数都属于这一类
             当一个进程阻塞于慢系统调用时捕获到一个信号，等到信号处理程序返回时，系统调用可能返回一个EINTR错误
             有些内核会自动重启某些被中断的系统调用。为了便于移植，当我们编写一个捕获信号的程序时
             必须对慢系统调用返回EINTR有所准备，我们要手动重启被中断的系统调用
             */
            if(errno == EINTR){
                continue;
            }
            printf("connect error: %d\n", errno);
            return 1;
        }
        printf("connected socket fd：%d\n", connected_sock);
        /**
         很明显这边要新建子进程来处理请求，以应对多个客户端的同时连接
         fork方法将会在父进程和子进程中各自返回一次
         父进程中fork会返回新建的子进程的pid
         子进程中fork会返回0
         */
        int p = fork();
        /**
         当p等于0时，下面代码块为子进程执行的代码，父进程将不会执行
         */
        if(p == 0){
            close(sock);
            /**
             write方法将应用程序的数据写入TCP发送缓冲区，返回值大于或等于0时表示发送的字节数，为负则表示发送失败
             write执行成功，只是表示应用程序中的数据被成功复制到了kernel中的TCP发送缓冲区，不代表接收方已经收到数据
             write的数据需要暂存在发送缓冲区中，只有收到对方的ack后，kernel才清除这一部分已经被接收的数据腾出空间
             接收端将收到的数据暂存在TCP接收缓冲区中进行确认，但如果接收端socket所在的进程不及时将数据从接收缓冲区中取出
             则会导致接收缓冲区填满，由于TCP的滑动窗口和拥塞控制，接收端会阻止发送端向其发送数据。
             应用程序如果继续发送数据，最终会导致发送缓冲区无法完整存放应用程序发送的数据，write方法将会阻塞。
             */
            if(write(connected_sock, first_message, sizeof(first_message)) == -1){
                printf("write error: %d\n", errno);
                printf("child thread exit\n");
                return 1;
            }
            char buffer[100];
            long nread;
            /**
             read方法将TCP接收缓冲区中的内容复制到应用程序中，返回值大于0时表示接收的字节数，为负则表示接收失败，等于0时代表对方已经关闭连接
             当接收缓冲区为空时，read方法将会阻塞，等待发送方发送数据
             */
            while((nread = read(connected_sock, buffer, 100)) > 0){
                if(buffer[nread] != '\0'){
                    buffer[nread] = '\0';
                }
                printf("receive a message: %s, length: %zu\n", buffer, strlen(buffer));
                /**
                 实现功能：客户端发送exit时，服务端主动断开连接
                 */
                if(strcmp(buffer, "exit") == 0 || strcmp(buffer, "exit\n") == 0){
                    close(connected_sock);
                    printf("connection closed by server\n");
                    break;
                }
                /**
                 实现功能：服务端将客户端发送的数据原样返回给客户端
                 */
                if(write(connected_sock, buffer, nread) == -1){
                    printf("write error: %d\n", errno);
                    return 1;
                }
            }
            if(nread < 0){
                printf("read error: %d", errno);
                return 1;
            }
            /**
             客户端主动关闭连接，服务端的read函数将返回0
             */
            if(nread == 0){
                close(connected_sock);
                printf("connection closed by client\n");
            }
            return 0;
        }else if(p > 0){
            close(connected_sock);
        }else{
            printf("fork error: %d", errno);
            return 1;
        }
    }
}
