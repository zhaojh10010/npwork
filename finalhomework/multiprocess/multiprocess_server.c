#include "../common/common.h"
 
void doit(int fd);  
void read_requesthdrs(rio_t *rp);  //读并忽略请求报头  
int parse_uri(char *uri, char *filename, char *cgiargs);   //解析uri，得文件名存入filename中，参数存入cgiargs中.
void serve_static(int fd, char *filename, int filesize);   //提供静态服务。  
void get_filetype(char *filename, char *filetype);  
void serve_dynamic(int fd, char *cause, char *cgiargs);    //提供动态服务。  
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg);
void sigchld_handler(int sig);

int main(int argc, char const *argv[])  
{  
    int listenfd, connfd;
    char hostname[MAXLINE], port[MAXLINE];
    socklen_t  clientlen; 
    struct sockaddr_storage clientaddr;  
  
    if(argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);  
        exit(0);  
    }     
 
    Signal(SIGCHLD, sigchld_handler);
    listenfd = Open_listenfd(atoi(argv[1]));  
    while(1) {  
        clientlen = sizeof(clientaddr);  
        connfd = Accept(listenfd,(SA *)&clientaddr, &clientlen);
        getnameinfo((SA*)&clientaddr, clientlen, hostname, MAXLINE, port, MAXLINE, 0);
        if(Fork() == 0){
            Close(listenfd);
            printf("Accept connection from (%s , %s)\n", hostname, port);
            doit(connfd);
            exit(0);
        }     
        Close(connfd);  
    }
}

void doit(int fd)  
{  
    int is_static;  
    struct stat sbuf;  
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];  
    char filename[MAXLINE],cgiargs[MAXLINE];  
    rio_t rio;  
  
    Rio_readinitb(&rio, fd);  
    Rio_readlineb(&rio, buf, MAXLINE); 
    printf("Request headers:\n"); 
    printf("%s", buf);
    sscanf(buf, "%s %s %s", method, uri, version);  
    if(strcasecmp(method,"GET")) {  
        clienterror(fd, method, "501", "Not Implemented", "The WebServer does not implement this method");  
        return;  
    }  
    read_requesthdrs(&rio);  
  
    is_static = parse_uri(uri, filename, cgiargs);  
    if(stat(filename, &sbuf) < 0) {  
        clienterror(fd, filename, "404", "Not found", "The WebServer coundn't find this file");  
        return;  
    }  
  
    if(is_static) {  
        if(!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode)) {  
            clienterror(fd,filename, "403", "Forbidden","The WebServer coundn't read the file");  
            return;  
        }  
        serve_static(fd, filename, sbuf.st_size);  
    }  
    else {  
        if(!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode)) {  
            clienterror(fd, filename, "403", "Forbidden","The WebServer coundn't run the CGI program");  
            return;  
        }  
        serve_dynamic(fd, filename, cgiargs);  
    }  
}

void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg)  
{  
    char buf[MAXLINE], body[MAXBUF];  
  
    sprintf(body, "<html><title>WebServer Error</title>");  
    sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body);  
    sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);  
    sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);  
    sprintf(body, "%s<hr><em>The Web server</em>\r\n", body);  
  
    sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, longmsg);  
    Rio_writen(fd, buf, strlen(buf));  
    sprintf(buf, "Content-type: text/html\r\n");  
    Rio_writen(fd, buf, strlen(buf));  
    sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));  
    Rio_writen(fd, buf, strlen(buf));  
    Rio_writen(fd, body, strlen(body));  
}

void read_requesthdrs(rio_t *rp)  
{  
    char buf[MAXLINE];  
    Rio_readlineb(rp, buf, MAXLINE);  
    while(strcmp(buf,"\r\n")) {  
        Rio_readlineb(rp, buf, MAXLINE);  
        printf("%s", buf);  
    }  
    return;  
}

int parse_uri(char *uri, char *filename,char *cgiargs)  
{  
    char *ptr;  
  
    if(!strstr(uri,"cgi-bin")) {  
        strcpy(cgiargs,"");  
        strcpy(filename,".");  
        strcat(filename, uri);  
        if(uri[strlen(uri)-1] == '/') {  
            strcat(filename,"home.html");  
        }  
        return 1;  
    }  
    else {  
        ptr = index(uri,'?');  
        if(ptr) {  
            strcpy(cgiargs,ptr+1);  
            *ptr = '\0';  
        }  
        else {  
            strcpy(cgiargs, "");  
        }  
        strcpy(filename, ".");  
        strcat(filename, uri);  
        return 0;  
    }  
}

void serve_static(int fd, char *filename, int filesize)  
{  
    int  srcfd;  
    char *srcp,filetype[MAXLINE],buf[MAXBUF];  
  
    get_filetype(filename,filetype);  
    sprintf(buf,"HTTP/1.0 200 OK\r\n");  
    sprintf(buf,"%sServer:Tiny Web Server\r\n", buf); 
    sprintf(buf,"%sConnection:close\r\n", buf);  
    sprintf(buf,"%sContent-length:%d\r\n", buf, filesize);  
    sprintf(buf,"%sContent-type:%s\r\n\r\n", buf, filetype);  
    Rio_writen(fd, buf, strlen(buf));
    printf("Response headers:\n");
    printf("%s", buf);
  
    srcfd = open(filename, O_RDONLY, 0);  
    srcp = mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0);  
    Close(srcfd);  
    Rio_writen(fd, srcp, filesize);  
    Munmap(srcp, filesize);  
}  
/* 
    打开文件名为filename的文件，把它映射到一个虚拟存储器空间，将文件的前filesize字节映射到从地址srcp开始的 
    虚拟存储区域。关闭文件描述符srcfd，把虚拟存储区的数据写入fd描述符，最后释放虚拟存储器区域。 
*/  
void get_filetype(char *filename, char *filetype)  
{  
    if(strstr(filename, ".html"))  
        strcpy(filetype, "text/html");  
    else if(strstr(filename, ".gif"))  
        strcpy(filetype, "image/gif");  
    else if(strstr(filename, ".jpg"))  
        strcpy(filetype, "image/jpeg");  
    else if(strstr(filename, ".png"))  
        strcpy(filetype, "image/png");  
    else   
        strcpy(filetype, "text/plain");  
}

void serve_dynamic(int fd, char *filename, char *cgiargs)  
{  
    char buf[MAXLINE],*emptylist[] = { NULL };  
  
    sprintf(buf,"HTTP/1.0 200 OK\r\n");  
    Rio_writen(fd, buf, strlen(buf));  
    sprintf(buf,"Server:Web Server\r\n");  
    rio_writen(fd, buf, strlen(buf));  
  
    if(Fork() == 0) {  
        setenv("QUERY_STRING", cgiargs, 1);  
        Dup2(fd, STDOUT_FILENO);  
        Execve(filename, emptylist, environ);  
    }  
    Wait(NULL);  
} 

void sigchld_handler(int sig){
    while(waitpid(-1, 0, WNOHANG) > 0)
        ;
    return;
}

