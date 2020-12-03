#include "common.h"

typedef struct
{
    int *buf;		/*Buffer array*/
    int n;		    /*Maximum number of slots*/
    int front; 	    /*buf[(front+1)%n] is first item*/
    int rear;		/*buf[rear%n] is last item*/
    sem_t mutex;	/*Protects accesses to buf*/
    sem_t slots;	/*Counts available slots*/
    sem_t items;	/*Counts available items*/
}sbuf_t;

void doit(int fd);  
void read_requesthdrs(rio_t *rp);  //读并忽略请求报头  
int parse_uri(char *uri, char *filename, char *cgiargs);   //解析uri，得文件名存入filename中，参数存入cgiargs中.
void serve_static(int fd, char *filename, int filesize);   //提供静态服务。  
void get_filetype(char *filename, char *filetype);  
void serve_dynamic(int fd, char *cause, char *cgiargs);    //提供动态服务。  
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg);
void sigchld_handler(int sig);
void* thread(void *vargp);
void sbuf_init(sbuf_t *sp, int n);
void sbuf_deinit(sbuf_t *sp);
void sbuf_insert(sbuf_t *sp, int item);
int sbuf_remove(sbuf_t *sp);

sbuf_t sbuf; /*global shared buffer of connected fd*/

int main(int argc, char **argv) {
    int listenfd, connfd, port, clientlen;
    struct sockaddr_in clientaddr;
    clientlen = sizeof(clientaddr);
    pthread_t tid;
    int NTHREADS,SBUFSIZE;

    if (argc != 4) {
        fprintf(stderr, "usage: %s <port> <num/thread> <fdbufsize>\n", argv[0]);
        exit(0);
    }
    port = atoi(argv[1]);
    NTHREADS = atoi(argv[2]);
    SBUFSIZE = atoi(argv[3]);

    Signal(SIGPIPE, SIG_IGN);
    Signal(SIGCHLD, sigchld_handler);

    sbuf_init(&sbuf, SBUFSIZE);
    listenfd = Open_listenfd(port);

    int i;
    for(i=0; i<NTHREADS; i++)/*create worker threads*/
    Pthread_create(&tid, NULL, thread, NULL);

    while (1) {
    //connfd = Malloc(sizeof(int));//avoid race condition
        connfd = Accept(listenfd, (SA *)&clientaddr,&clientlen);
        sbuf_insert(&sbuf,connfd);     
    }
}
/* $end tinymain */  
void *thread(void *vargp)
{
    Pthread_detach(pthread_self());
    while(1)
    {
        int connfd = sbuf_remove(&sbuf);
        doit(connfd);
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

void sbuf_init(sbuf_t *sp,int n)
{
    sp->buf = Calloc(n,sizeof(int));//初始化
    sp->n = n;       				
    sp->front = sp->rear = 0;		
    Sem_init(&sp->mutex,0,1);		
    Sem_init(&sp->slots,0,n);		
    Sem_init(&sp->items,0,0);		
}

/* 清空*/
void sbuf_deinit(sbuf_t *sp)
{
    Free(sp->buf);
}

/* 插入到末尾*/
void sbuf_insert(sbuf_t *sp,int item)
{
    P(&sp->slots);						 
    P(&sp->mutex);						 
    sp->buf[(++sp->rear)%(sp->n)] = item;
    V(&sp->mutex);						 
    V(&sp->items);						 
}


/* 移除并返回从buffer的sp返回第一个item*/
int sbuf_remove(sbuf_t *sp)
{
    int item;
    P(&sp->items);                          /* 等待 */
    P(&sp->mutex);                          /* 锁定 */
    item = sp->buf[(++sp->front)%(sp->n)];  /* 移除 */
    V(&sp->mutex);                          /* 解锁 */
    V(&sp->slots);                          、
    return item;
}

