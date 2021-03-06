#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
using namespace std;
#define BACKLOG_NUM 10 //maximum backlog num
#define PORT 8888
#define MAXDATASIZE 1000

int main() {
    int listenfd,connfd,numbytes;
    char buf[MAXDATASIZE];
    struct sockaddr_in servaddr,cliaddr;
    cout << "==============================" << endl;
    cout << "Server is starting..." << endl;
    //create socket
    if((listenfd=socket(AF_INET, SOCK_STREAM,0)) ==-1 ) {
        perror("Create server socket failed: ");
        exit(-1);
    }
    
    bzero(&servaddr,sizeof(servaddr));
    bzero(&cliaddr,sizeof(cliaddr));
    servaddr.sin_family = AF_INET;
    //Remeber all these two variables need to be converted
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(PORT);
    //Set socket options
    int opt = SO_REUSEADDR;
    setsockopt(listenfd,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));
    //bind the port
    if(bind(listenfd, (sockaddr*)&servaddr, sizeof(servaddr)) == -1) {
	    perror("Bind port failed: ");
        exit(-1);
    }
    cout << "Server started at port " << PORT << " successfully." << endl;
    //listen to connections
    if(listen(listenfd,BACKLOG_NUM) == -1) {
        perror("Listen failed: ");
        exit(-1);
    }
    cout << "Listening to connections..." << endl;
    cout << "==============================" << endl;
    while(1) {
	    //wait to create conn
        socklen_t clilen = sizeof(cliaddr);//Here must allocate a variable or 'accept' cannot write into the &clilen
        if((connfd=accept(listenfd,(sockaddr*) &cliaddr, &clilen)) == -1) {
            perror("Create connection failed: ");
            continue;
        }
        cout << "New connection from: " << inet_ntoa(cliaddr.sin_addr)
            << ":" << ntohs(cliaddr.sin_port) << endl;

        string welc="Welcome to my server!";
        if(send(connfd,welc.c_str(),welc.length()+1,0)<=0) {
            perror("Connection is closed.");
            close(connfd);
            continue;
        }
        //Receive msg from client
        pthread_t pid;
        
        while(1) {
            bzero(buf,MAXDATASIZE);
            cout << "Client: ";
            if((numbytes=recv(connfd,buf,MAXDATASIZE,0))<=0) {
                cout << "No information received.Connection will end immediately." << endl;
                break;
            };
            cout << buf << endl;
            // cout << "received " << strlen(buf) << " bits data" << endl;
            //Reverse received string
            int len = strlen(buf);
            int i=len-1,j=0;
            char temp[i];
            while(i>-1) {
                temp[j++]=buf[i--];
            }
            if(send(connfd,temp,len,0)<=0) {
                perror("Send message failed: ");
                break;
            }
        }

        close(connfd);
        cout << "Connection "<< inet_ntoa(cliaddr.sin_addr)
            << ":" << ntohs(cliaddr.sin_port) << " closed."<< endl;
    }
    close(listenfd);
    cout << "Server shutdown." << endl;
    return 0;
}
