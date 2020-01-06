#include "util.h"

int main(void) {
    char *buf,*p;
    int length=0;
    char content[MAXLINE],data[MAXLINE];

    if ((buf = getenv("CONTENT-LENGTH")) != NULL)
    {
    	length=atol(buf); //获得长度
    }
    
    p=fgets(data,length+1,stdin); //利用标准输入读取动态内容到data

    if(p==NULL)
    	sprintf(content, "ERROR!\r\n");//提示出错
    else	
	sprintf(content, "Your information is:%s\r\n",data);   //将动态内容写入缓冲区content
    
    //产生响应头报文并且写入标准输出
    printf("Content-length: %ld\r\n", strlen(content));
    printf("Content-type: text/html\r\n\r\n");
    printf("%s", content); //输出动态内容
    fflush(stdout);
    exit(0);
}
