#include "util.h"

void request_handler(int fd);
void get_requesthdrs(rio_t *rp);
void post_requesthdrs(rio_t *rp,int *length);
int parse_uri(char *uri, char *filename, char *cgiargs);
void serve_static(int fd, char *filename, int filesize);
void get_filetype(char *filename, char *filetype);
void get_dynamic(int fd, char *filename, char *cgiargs);
void post_dynamic(int fd, char *filename, int contentLength,rio_t *rp); 
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg);


int main(int argc, char **argv) 
{
    int listenfd, connfd;
    char hostname[MAXLINE], port[MAXLINE];
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;

    if (argc != 2) {
	fprintf(stderr, "usage: %s <port>\n", argv[0]);
	exit(1);
    }

    listenfd = Open_listenfd(argv[1]);
    while (1) {
	clientlen = sizeof(clientaddr);
	connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen); 
        Getnameinfo((SA *) &clientaddr, clientlen, hostname, MAXLINE, 
                    port, MAXLINE, 0);
        printf("Accepted connection from (%s, %s)\n", hostname, port);
	request_handler(connfd);                                             
	Close(connfd);                                           
    }
}

void request_handler(int fd) 
{
    int is_static;
	int isGet=1,contentLength=0;
    struct stat sbuf;
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    char filename[MAXLINE], cgiargs[MAXLINE];
    rio_t rio;

    Rio_readinitb(&rio, fd);
    if (!Rio_readlineb(&rio, buf, MAXLINE))  
        return;
    printf("%s", buf);
    sscanf(buf, "%s %s %s", method, uri, version);       
    if (strcasecmp(method, "GET")!=0 && strcasecmp(method,"POST")!=0) {                     
        clienterror(fd, method, "501", "Not Implemented",
                    "WebcServer does not implement this method  ");
        return;
    }                                                   
    if(strcasecmp(method, "POST")==0)
		isGet=0;

    is_static = parse_uri(uri, filename, cgiargs);      
    if (stat(filename, &sbuf) < 0) {                     
	clienterror(fd, filename, "404", "Not found",
		    "WebcServer couldn't find this file  ");
	return;
    }                                                   

    if (is_static) {        
	get_requesthdrs(&rio);
	    if (!(S_ISREG(sbuf.st_mode)) || !(S_IRUSR & sbuf.st_mode)) {
	    clienterror(fd, filename, "403", "Forbidden",
			"WebcServer couldn't read the file  ");
	    return;
	}
	serve_static(fd, filename, sbuf.st_size);        
    }
    else { 
	if (!(S_ISREG(sbuf.st_mode)) || !(S_IXUSR & sbuf.st_mode)) { 
	    clienterror(fd, filename, "403", "Forbidden",
			"WebcServer couldn't run the CGI program  ");
	    return;
	}
	if(isGet){
		get_requesthdrs(&rio);
	        get_dynamic(fd, filename, cgiargs);          
    }
	else{
		post_requesthdrs(&rio,&contentLength);
		post_dynamic(fd, filename,contentLength,&rio);
	}
    }
}

void get_requesthdrs(rio_t *rp) 
{
    char buf[MAXLINE];

    Rio_readlineb(rp, buf, MAXLINE);
    printf("%s", buf);
    while(strcmp(buf, "\r\n")) 
    {
	Rio_readlineb(rp, buf, MAXLINE);
	printf("%s", buf);
    }
    return;
}

void post_requesthdrs(rio_t *rp,int *length) 
{
    char buf[MAXLINE];
    char *p;

    Rio_readlineb(rp, buf, MAXLINE);
    printf("%s", buf);
    while(strcmp(buf, "\r\n")) 
    {
	Rio_readlineb(rp, buf, MAXLINE);
	if(strncasecmp(buf,"Content-Length:",15)==0)
	{
		p=&buf[15];	
		p+=strspn(p," \t");
		*length=atol(p);
	}
	printf("%s", buf);
    }
    return;
}


int parse_uri(char *uri, char *filename, char *cgiargs) 
{
    char *ptr;

    if (!strstr(uri, "cgi-bin")) {  
	strcpy(cgiargs, "");                             
	strcpy(filename, ".");                           
	strcat(filename, uri);                           
	if (uri[strlen(uri)-1] == '/')                   
	    strcat(filename, "home.html");              
	return 1;
    }
    else {                       
	ptr = index(uri, '?');                           
	if (ptr) {
	    strcpy(cgiargs, ptr+1);
	    *ptr = '\0';
	}
	else 
	    strcpy(cgiargs, "");                        
	strcpy(filename, ".");                          
	strcat(filename, uri);                           
	return 0;
    }
}

void serve_static(int fd, char *filename, int filesize) 
{
    int srcfd;
    char *srcp, filetype[MAXLINE], buf[MAXBUF];
 
    get_filetype(filename, filetype);       
    sprintf(buf, "HTTP/1.0 200 OK\r\n");    
    sprintf(buf, "%sServer: WebcServer\r\n", buf);
    sprintf(buf, "%sConnection: close\r\n", buf);
    sprintf(buf, "%sContent-length: %d\r\n", buf, filesize);
    sprintf(buf, "%sContent-type: %s\r\n\r\n", buf, filetype);
    Rio_writen(fd, buf, strlen(buf));       
    printf("Response headers:\n");
    printf("%s", buf);

    srcfd = Open(filename, O_RDONLY, 0);    
    srcp = Mmap(0, filesize, PROT_READ, MAP_PRIVATE, srcfd, 0);
    Close(srcfd);                          
    Rio_writen(fd, srcp, filesize);         
    Munmap(srcp, filesize);                 
}

void get_filetype(char *filename, char *filetype) 
{
    if (strstr(filename, ".html"))
	strcpy(filetype, "text/html");
    else if (strstr(filename, ".gif"))
	strcpy(filetype, "image/gif");
    else if (strstr(filename, ".png"))
	strcpy(filetype, "image/png");
    else if (strstr(filename, ".jpg"))
	strcpy(filetype, "image/jpeg");
    else
	strcpy(filetype, "text/plain");
}  

void get_dynamic(int fd, char *filename, char *cgiargs) 
{
    char buf[MAXLINE], *emptylist[] = { NULL };

    sprintf(buf, "HTTP/1.0 200 OK\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Server: WebcServer\r\n");
    Rio_writen(fd, buf, strlen(buf));
  
    if (Fork() == 0) 
    { 
	setenv("QUERY_STRING", cgiargs, 1); 
	Dup2(fd, STDOUT_FILENO);         
	Execve(filename, emptylist, environ); 
    }
    Wait(NULL); 
}

void post_dynamic(int fd, char *filename, int contentLength,rio_t *rp)
{
    char buf[MAXLINE],length[32], *emptylist[] = { NULL },data[MAXLINE];
    int p[2];

    sprintf(length,"%d",contentLength);
    memset(data,0,MAXLINE);

    pipe(p);

    if (Fork() == 0)
	{                     
	Close(p[0]);
	Rio_readnb(rp,data,contentLength);
	Rio_writen(p[1],data,contentLength);
	exit(0)	;
	}
    
    sprintf(buf, "HTTP/1.0 200 OK\r\n");
    sprintf(buf, "%sServer: WebcServer\r\n",buf);

    Rio_writen(fd, buf, strlen(buf));

    Dup2(p[0],STDIN_FILENO);  
    Close(p[0]);

    Close(p[1]);
    setenv("CONTENT-LENGTH",length , 1); 

    Dup2(fd,STDOUT_FILENO);       
	Execve(filename, emptylist, environ); 
    
}

void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg) 
{
    char buf[MAXLINE], body[MAXBUF];

    sprintf(body, "<html><title>Server Error</title>");
    sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body);
    sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
    sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
    sprintf(body, "%s<hr><em> WebcServer </em>\r\n", body);

    sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-type: text/html\r\n");
    Rio_writen(fd, buf, strlen(buf));
    sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
    Rio_writen(fd, buf, strlen(buf));
    Rio_writen(fd, body, strlen(body));
}


