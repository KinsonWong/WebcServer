#include "util.h"

int main(void) {
    char *buf,*p;
    int length=0;
    char content[MAXLINE],data[MAXLINE];

    if ((buf = getenv("CONTENT-LENGTH")) != NULL)
    {
    	length=atol(buf);
    }
    
    p=fgets(data,length+1,stdin);
    if(p==NULL)
    	sprintf(content, "ERROR!\r\n");
    else	
	sprintf(content, "Your information is:%s\r\n",data);

    printf("Content-length: %ld\r\n", strlen(content));
    printf("Content-type: text/html\r\n\r\n");
    printf("%s", content);
    fflush(stdout);
    exit(0);
}
