#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <iostream>
using namespace std;
#define BACKLOG_NUM 10 //maximum backlog num
#define PORT 80

int main() {
    cout << "Server is starting..." << endl;
    int servfd,connfd;
    struct sockaddr_in servaddr,client;
    //create socket
    if((servfd=socket(AF_INET, SOCK_STREAM,0)) ==-1 ) {
        cout << "Create server socket failed" << endl;
        perror("Error: ");
        exit(-1);
    }
    
    bzero(&servaddr,sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(80);
    int opt = SO_REUSEADDR;
    setsockopt(servfd,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));
    //bind the port
    if(bind(servfd, (sockaddr*)&servaddr, sizeof(servaddr)) == -1) {
        cout << "Bind port 80 failed" << endl;
	    perror("Error: ");
        exit(-1);
    }
    cout << "Server started at port 80 successfully" << endl;
    //listen to connections
    if(listen(servfd,BACKLOG_NUM) == -1) {
        cout << "Listen failed" << endl;
        perror("Error: ");
        exit(-1);
    }
    cout << "Listening to connections" << endl;
    while(1) {
	    //wait to create conn
        if((connfd=accept(servfd,(sockaddr*) &client, (socklen_t *)sizeof(struct sockaddr_in))) == -1) {
            cout << "Create connection failed" << endl;
            perror("Error: ");
            continue;
        }
        cout << "Connecting from: " << inet_ntoa(client.sin_addr)
            << "  Port: " << ntohs(client.sin_port) << endl;
        close(connfd);
        cout << "Connection closed" << endl;
    }
    close(servfd);
    cout << "Server closed" << endl;
    return 0;
}
