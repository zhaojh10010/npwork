#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <iostream>
using namespace std;
#define BACKLOG_NUM 10 //maximum backlog num
#define PORT 8888

int main() {
    cout << "Server is starting..." << endl;
    int listenfd,connfd;
    struct sockaddr_in servaddr,cliaddr;
    //create socket
    if((listenfd=socket(AF_INET, SOCK_STREAM,0)) ==-1 ) {
        cout << "Create server socket failed" << endl;
        perror("Error: ");
        exit(-1);
    }
    
    bzero(&servaddr,sizeof(servaddr));
    bzero(&cliaddr,sizeof(cliaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(PORT);
    int opt = SO_REUSEADDR;
    setsockopt(listenfd,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));
    //bind the port
    if(bind(listenfd, (sockaddr*)&servaddr, sizeof(servaddr)) == -1) {
        cout << "Bind port 80 failed" << endl;
	    perror("Error: ");
        exit(-1);
    }
    cout << "Server started at port " << PORT << " successfully" << endl;
    //listen to connections
    if(listen(listenfd,BACKLOG_NUM) == -1) {
        cout << "Listen failed" << endl;
        perror("Error: ");
        exit(-1);
    }
    cout << "Listening to connections" << endl;
    while(1) {
	    //wait to create conn
        socklen_t clilen = sizeof(cliaddr);
        if((connfd=accept(listenfd,(sockaddr*) &cliaddr, &clilen)) == -1) {
            cout << "Create connection failed" << endl;
            perror("Error: ");
            continue;
        }
        cout << "Connecting from: " << inet_ntoa(cliaddr.sin_addr)
            << "  Port: " << ntohs(cliaddr.sin_port) << endl;
        close(connfd);
        cout << "Connection closed" << endl;
    }
    close(listenfd);
    cout << "Server closed" << endl;
    return 0;
}
