#include <sys/socket.h>
#include <iostream>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
using namespace std;
#define MAX_WAIT_NUM 10 //最大backlog数

int main() {
    int sock_server,sock_conn;
    struct sockaddr_in server,client;
    //创建服务器套接字
    if(sock_server=socket(AF_INET, SOCK_STREAM,0) ==-1 )
        cout << "Create server socket failed" << endl;
    //绑定端口
    server.sin_family = AF_INET;//协议族
    server.sin_addr.s_addr = htonl(INADDR_ANY);//存放32位IP地址
    server.sin_port = htons(80);//端口

    if(bind(sock_server, (sockaddr*)&server, sizeof(server)) == -1) {
        server.sin_port = htons(8888);
        if(bind(sock_server, (sockaddr*)&server, sizeof(server)) == -1) {
            cout << "Bind port 80 & 8888 failed" << endl;
            exit(-1);
        }
    }
    //开始监听连接
    if(listen(sock_server,MAX_WAIT_NUM) != -1) {
        cout << "Listen failed" << endl;
        exit(-1);
    }

    while(1) {
        //等待接收连接
        if(sock_conn=accept(sock_server,(sockaddr*) &client, (socklen_t *)sizeof(struct sockaddr)) == -1) {
            cout << "Create connection failed" << endl;
            continue;
        }
        //建立连接成功
        cout << "A connection come from: " << inet_ntoa(client.sin_addr)
            << "  Port: " << ntohs(client.sin_port) << endl;
        close(sock_conn);
        cout << "Connection closed" << endl;
    }
    close(sock_server);
    cout << "Server closed" << endl;
    return 0;
}
