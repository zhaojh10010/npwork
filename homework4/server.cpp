#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/select.h>
#include <sys/time.h>
#include <errno.h>

using namespace std;
#define BACKLOG_NUM 10 //maximum backlog num
#define PORT 8888
#define MAXDATASIZE 100
#define MAXMESSAGESIZE 9999
fd_set rset,allset;

/**
 * Process client request
 * @param c client info
 * @param buf buffer
 */
void process(struct c_info *c, char *buf);

/**
 * Close client and clear FD_SET bits
 * @param c client info
 */
void close_cli(struct c_info *c);

/**
 * A client struct for thread
 * @param connfd client socket descriptor
 * @param name client name
 * @param addr client ip address
 * @param port client port
 * @param msg  all messages in one session
 * @param msgpos index of first idle area in msg
 */
struct c_info{
    int connfd;
    char *name;
    char *addr;
    int port;
    char *msg;
    int msgpos;
};

int main() {
    int listenfd,connfd,clifd,maxfd,nready,i,maxi;
    struct sockaddr_in servaddr,cliaddr;
    socklen_t clilen;
    c_info clients[FD_SETSIZE];
    char buf[MAXDATASIZE];

    for(i=0;i<FD_SETSIZE;i++)
        clients[i].connfd = -1;
    
    cout << "======================Server started=======================" << endl;
    //create socket
    if((listenfd=socket(AF_INET, SOCK_STREAM,0)) ==-1 ) {
        perror("Create server socket failed: ");
        exit(-1);
    }
    maxfd = listenfd;
    FD_ZERO(&allset);
    FD_SET(listenfd,&allset);
    
    bzero(&servaddr,sizeof(servaddr));
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
    cout << "===========================================================" << endl;
    while(1) {
        bzero(&cliaddr,sizeof(cliaddr));
        rset = allset;
        if((nready = select(maxfd+1,&rset,NULL,NULL,NULL))<0) {
            if(errno!=EINTR)
                perror("Select error: ");
            continue;
        }
        if(FD_ISSET(listenfd,&rset)) {
            //wait to create conn
            clilen = sizeof(cliaddr);//Here must alloc a variable or 'accept' cannot write into the clilen
            if((connfd=accept(listenfd,(sockaddr*) &cliaddr, &clilen)) == -1) {
                perror("Create connection failed: ");
                continue;
            }
            //add connfd to clients array
            for(i=0;i<FD_SETSIZE;i++)
                if(clients[i].connfd==-1) {
                    //Create thread and allocate private variable
                    clients[i].connfd = connfd;
                    clients[i].addr = inet_ntoa(cliaddr.sin_addr);
                    clients[i].port = ntohs(cliaddr.sin_port);
                    clients[i].name = new char[MAXDATASIZE];
                    clients[i].msg = new char[MAXMESSAGESIZE];
                    clients[i].msgpos = 0;
                    break;
                }
            if(i==FD_SETSIZE) cout << "Warning: too many clients" << endl;
            FD_SET(connfd,&allset);
            if(connfd>maxfd) maxfd=connfd;
            if(i>maxi) maxi=i;//record latest conn index
            if(--nready<=0) continue;
        }
        for(i=0;i<=maxi;i++) {
            bzero(buf,MAXDATASIZE);
            struct c_info *c = &clients[i];
            if((clifd=c->connfd)<0) continue;
            if(FD_ISSET(clifd,&rset)) {
                if(strlen(c->name)==0) {//first connect
                    //reveive client name
                    if(recv(c->connfd,c->name,MAXDATASIZE,0)==-1) {
                        perror("Receive client name failed: ");
                        close_cli(c);
                        break;
                    }
                    cout << "New connection from: " << c->name << "@" << c->addr
                        << ":" << c->port << endl;
                    char welc[]="Welcome to my server,";
                    strcat(welc,c->name);
                    if(send(connfd,welc,strlen(welc)+1,0)==-1) {
                        perror("Send error: ");
                        close_cli(c);
                        break;
                    }
                }
                
                //process client request
                // process(&cinfo,buf);
                process(c,buf);
                if(--nready<=0) break;
            }
        }
    }
    close(listenfd);
    cout << "======================Server shutdown=======================" << endl;
    return 0;
}

void process(struct c_info *c, char *buf) {
    int connfd = c->connfd;
    
    int numbytes,i,j;
    if((numbytes=recv(connfd,buf,MAXDATASIZE,0))<=0) {
        if(numbytes==0)
            close(connfd);
        close_cli(c);
        return;
    };
    cout << c->name << ":" << buf << endl;
    
    //Suppose the MAXMESSAGESIZE is suitable for a client in a session
    //so ignore buffer overflow problem

    int len = strlen(buf);
    for(i=0;i<len;i++) {
        c->msg[c->msgpos++] = buf[i];
    }
    c->msg[c->msgpos] = '\n';//add '\n' for every received msg
    c->msgpos++;
    //send message back
    if(send(connfd,buf,len+1,0)==-1) {
        perror("Send message failed: ");
        close_cli(c);
        return;
    }
}

void close_cli(struct c_info *c) {
    // struct c_info cinfo = *c;
    FD_CLR(c->connfd, &allset);
    cout << "===========================================================" << endl;
    cout << "Notice: Connection from " << c->name << "@" << c->addr
            << ":" << c->port << " closed."<< endl;
    if(strlen(c->msg)>0)
        cout << "All data from " << c->name << "@" << c->addr
            << ":" << c->port << " are:\n" << c->msg;
    else cout << "No data transferred." << endl;
    cout << "===========================================================" << endl;
    c->connfd=-1;
    delete c->name;
    delete c->msg;
}