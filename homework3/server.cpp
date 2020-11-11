#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

using namespace std;
#define BACKLOG_NUM 10 //maximum backlog num
#define PORT 8888
#define MAXDATASIZE 1000
#define MAXMESSAGESIZE 10000
static pthread_key_t key;
static pthread_once_t key_once = PTHREAD_ONCE_INIT;

/**
 * execute after a thread start, 
 * mainly used to store client infomation in TSD
 * @param arg client information
 */
void *start(void *arg);


/**
 * Process client request
 */
void process();

/**
 * Allocate TSD key
 */
static void alloc_key_once();

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
    int listenfd,connfd;
    struct sockaddr_in servaddr,cliaddr;
    struct c_info cinfo;
    socklen_t clilen;
    pthread_t tid;
    cout << "======================Server started=======================" << endl;
    //create socket
    if((listenfd=socket(AF_INET, SOCK_STREAM,0)) ==-1 ) {
        perror("Create server socket failed: ");
        exit(-1);
    }
    
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
    //init thread specific data key
    pthread_once(&key_once,alloc_key_once);
    cout << "Listening to connections..." << endl;
    cout << "===========================================================" << endl;
    while(1) {
	    //wait to create conn
        bzero(&cliaddr,sizeof(cliaddr));
        clilen = sizeof(cliaddr);//Here must alloc a variable or 'accept' cannot write into the clilen
        if((connfd=accept(listenfd,(sockaddr*) &cliaddr, &clilen)) == -1) {
            perror("Create connection failed: ");
            continue;
        }
        
        //Create thread and allocate private variable
        cinfo.addr = inet_ntoa(cliaddr.sin_addr);
        cinfo.port = ntohs(cliaddr.sin_port);
        cinfo.msgpos = 0;
        cinfo.connfd = connfd;
        //create client thread
        if(pthread_create(&tid,NULL,&start,&cinfo)<0) {
            perror("Create thread failed..");
            close(cinfo.connfd);
            cout << "Connection "<< cinfo.addr
                << ":" << cinfo.port << " closed."<< endl;
            continue;
        }
    }
    close(listenfd);
    cout << "Server shutdown." << endl;
    return 0;
}

void *start(void *arg) {
    //allocate a separated space for thread
    struct c_info cinfo = *(struct c_info *)arg;
    pthread_setspecific(key,&cinfo);
    process();
    return NULL;
}

void process() {
    //get private info from TSD
    struct c_info *cinfo = (struct c_info *)pthread_getspecific(key);
    int connfd = cinfo->connfd;
    char message[MAXMESSAGESIZE];
    cinfo->msg = message;
    //reveive client name
    char name[MAXDATASIZE];
    if(recv(connfd,name,MAXDATASIZE,0)==-1) {
        perror("Receive client name failed: ");
        cout << "Connection "<< cinfo->addr
            << ":" << cinfo->port << " closed."<< endl;
        return;
    }
    cinfo->name = name;
    cout << "New connection from: " << cinfo->name << "@" << cinfo->addr
        << ":" << cinfo->port << endl;
    char welc[]="Welcome to my server,";
    strcat(welc,cinfo->name);
    if(send(connfd,welc,strlen(welc)+1,0)==-1) {
        perror("Send message failed: ");
        cout << "Connection "<< cinfo->addr
            << ":" << cinfo->port << " closed."<< endl;
        return;
    }
    while(1) {
        char buf[MAXDATASIZE];
        int numbytes,i,j;
        bzero(buf,MAXDATASIZE);
        if((numbytes=recv(connfd,buf,MAXDATASIZE,0))<=0) {
            cout << "===========================================================" << endl;
            cout << "Warning: No information received from "<< cinfo->name << "@" << cinfo->addr
                << ":" << cinfo->port << ".Connection will close immediately." << endl;
            if(numbytes==0) close(connfd);
            break;
        };
        cout << cinfo->name << ":" << buf << endl;
        // cout << "received " << strlen(buf) << " bits data" << endl;
        //Suppose the MAXMESSAGESIZE is suitable for a client in a session
        //so ignore buffer overflow problem
        int len = strlen(buf);
        for(i=0;i<len;i++) {
            cinfo->msg[cinfo->msgpos++] = buf[i];
        }
        cinfo->msg[cinfo->msgpos] = '\n';//add '\n' for every received msg
        cinfo->msgpos++;

        //Reverse received string
        i=len-1;
        j=0;
        char temp[i];
        while(i>-1) {
            temp[j++]=buf[i--];
        }
        if(send(connfd,temp,len,0)==-1) {
            perror("Send message failed: ");
            break;
        }
    }
    close(connfd);
    //Print all data tranfered from client
    cout << "Notice: Connection from " << cinfo->name << "@" << cinfo->addr
        << ":" << cinfo->port << " closed."<< endl;
    
    if(strlen(cinfo->msg)>0) {
        cout << "All data from " << cinfo->name << "@" << cinfo->addr
            << ":" << cinfo->port << " are:\n" << cinfo->msg;
    }
    cout << "===========================================================" << endl;
}

static void alloc_key_once() {
    pthread_key_create(&key, NULL);
}