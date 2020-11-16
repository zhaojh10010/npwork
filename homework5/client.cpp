#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>
#include <netdb.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

using namespace std;
#define MAXDATASIZE 1000
#define SERVER_PORT 10000
int clifd;
struct sockaddr_in addr;

/**
 * process client request
 */
void process(int signum);

int main(int argc, char *argv[]) {
    char** pptr;
    struct sigaction act;
    
    cout << "===============Client started===============" << endl;
    bzero(&addr,sizeof(addr));
    if((clifd=socket(AF_INET,SOCK_STREAM,0))==-1) {
        perror("Create socket failed: ");
        exit(-1);
    }
    addr.sin_family = AF_INET;
    addr.sin_port = htons(SERVER_PORT);
    addr.sin_addr.s_addr = inet_addr("10.0.0.10");
    cout << "Start connecting to server" << endl;
    //Reconstruct connect method after a signal interrupt
    act.sa_handler = process;
    act.sa_flags = 0;
    sigaction(SIGINT,&act,NULL);
    while(1) {
        if(connect(clifd, (sockaddr*)&addr, sizeof(struct sockaddr))==-1) {
            cout << "errno: " << errno << endl;
            if(errno== EINTR) {
                cout << "Interrupted and restart." << endl;
                continue;
            } else {
                perror("Connecting failed: ");
                exit(-1);
            }
        } else {
            cout << "Connected to the server "<< *(--argv) << ":" << SERVER_PORT << endl;
            cout << "local address: " << inet_ntoa(addr.sin_addr) << ":" << addr.sin_port << endl;
            break;
        }
    }
    close(clifd);
    return 0;
}

void process(int signum) {
    cout << "=============In signal function=============" << endl;
}