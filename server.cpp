#include <sys/socket.h>
#include <iostream>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
using namespace std;
#define MAX_WAIT_NUM 10 //maximum backlog num
#define PORT 80

int main() {
    cout << "Server started" << endl;
    int servfd,sock_conn;
    struct sockaddr_in server,client;
    //create socket
    if((servfd=socket(AF_INET, SOCK_STREAM,0)) ==-1 )
        cout << "Create server socket failed" << endl;    
    
    bzero(&server,sizeof(server));
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(80);
    int opt = SO_REUSEADDR;
    setsockopt(servfd,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));
    //bind the port
    if(bind(servfd, (sockaddr*)&server, sizeof(server)) == -1) {
        cout << "Bind port 80 failed" << endl;
	perror("Error: ");
        exit(-1);
    }
    //listen to connection
    if(listen(servfd,MAX_WAIT_NUM) == -1) {
        cout << "Listen failed" << endl;
        exit(-1);
    }
    cout << "Server is listening to connections" << endl;
    while(1) {
	//wait to create conn
        if((sock_conn=accept(servfd,(sockaddr*) &client, (socklen_t *)sizeof(struct sockaddr))) == -1) {
            cout << "Create connection failed" << endl;
            continue;
        }
        cout << "A connection come from: " << inet_ntoa(client.sin_addr)
            << "  Port: " << ntohs(client.sin_port) << endl;
        close(sock_conn);
        cout << "Connection closed" << endl;
    }
    close(servfd);
    cout << "Server closed" << endl;
    return 0;
}
