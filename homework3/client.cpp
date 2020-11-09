#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>
#include <netdb.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

using namespace std;
#define MAXDATASIZE 100
#define SERVER_PORT 8888

int main(int argc, char *argv[]) {
    int clifd, numbytes;
    struct hostent* he;
    struct sockaddr_in addr;
    char buf[MAXDATASIZE];
    char name[MAXDATASIZE];
    char** pptr;
    cout << "===============Client started===============" << endl;
    if(argc!=2) {
        cout << "Please input ip or domain name correctly." << endl;
        exit(-1);
    }
    argv++;
    bzero(&addr,sizeof(addr));
    //get address information from input
    for(;*argv!=NULL;argv++) {
        //when argv is ip address
        if((inet_aton(*argv, &addr.sin_addr))!=0) {
            he = gethostbyaddr(&addr.sin_addr,sizeof(struct in_addr),AF_INET);
            cout << "Get IP Address: " << *argv << endl;
        } else {//when argv is domain name
            he = gethostbyname(*argv);
            cout << "Get domain name: " << *argv << endl;
        }
        if(he==NULL) {
            continue;
        }
    }
    if(he==NULL) {
        cout << "No address information found." << endl;
        exit(-1);
    }
    //Print server address information
    cout << "Get address information as follows:" << endl;
    cout << "- host name: " << he->h_name << endl;
    for (pptr = he->h_aliases; *pptr!=NULL; pptr++) {
        cout << "- alias: " << *pptr << endl;
    }
    for (pptr = he->h_addr_list; *pptr!=NULL; pptr++) {
        cout << "- ip address: " << inet_ntoa(*(in_addr*) (*pptr)) << endl;
    }
    cout << "============================================" << endl;
    if((clifd=socket(AF_INET,SOCK_STREAM,0))==-1) {
        perror("Create socket failed: ");
        exit(-1);
    }
    addr.sin_family = AF_INET;
    addr.sin_port = htons(SERVER_PORT);
    addr.sin_addr = *((struct in_addr*)he->h_addr);//h_addr is the first address in h_addr_list
    // cout << "Start connecting to server" << endl;
    if(connect(clifd, (sockaddr*)&addr, sizeof(struct sockaddr))==-1) {
        perror("Connecting failed: ");
        exit(-1);
    }
    cout << "Connected to the server "<< *(--argv) << ":" << SERVER_PORT << endl;
    cout << "local address: " << inet_ntoa(addr.sin_addr) << ":" << addr.sin_port << endl;
    //transfer client name
    cout << "Please input your name, and press ENTER to send(no more than 100 characters):\nYour name: ";
    cin.getline(name,MAXDATASIZE);
    if(strlen(name)==0)
        strcpy(name,"anonymous");
    if((numbytes=send(clifd,name,MAXDATASIZE,0))==-1) {
        perror("Send name error: ");
        exit(-1);
    }
    cout << "============================================" << endl;
    // cout << "Receving msg from server: " << endl;
    if((numbytes=recv(clifd,buf,MAXDATASIZE,0))<=0) {
        perror("Recv data error: ");
        exit(-1);
    }
    //buf[numbytes]='\0';

    cout << "Server: " << buf << endl;
    //read message from command line
    cout << "============================================" << endl;
    while(1) {
        cout << "Input your message, and press ENTER to send(no more than 100 characters):\n" << name <<":";
        bzero(buf,MAXDATASIZE);
        cin.getline(buf,MAXDATASIZE);
        if(cin.eof()) break;//recognize Ctrl+D
        if(strlen(buf)==0) continue;//recognize ENTER
        if((numbytes=send(clifd,buf,MAXDATASIZE,0))==-1) {
            perror("Send data error: ");
            exit(-1);
        }
        //reveive message from server
        bzero(buf,MAXDATASIZE);
        if((numbytes=recv(clifd,buf,MAXDATASIZE,0))==-1) {  
            perror("Recv data error: ");
            exit(-1);
        } 
        // cout << "received " << numbytes << " bits data" << endl;
        cout << "Server:" << buf << endl;
    }
    cout << "===========================================" << endl;
    cout << "Data transfer finished" << endl;
    close(clifd);
    return 0;
}