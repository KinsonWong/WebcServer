#include "util.h"

int main(void) {
    char *buf, *p;
    char arg1[MAXLINE], arg2[MAXLINE], content[MAXLINE];
    int n1=0, n2=0;

    if ((buf = getenv("QUERY_STRING")) != NULL) {
	p = strchr(buf, '&');
	*p = '\0';
	strcpy(arg1, buf);
	strcpy(arg2, p+1);
	//将字符串变为整型
	n1 = atoi(arg1);
	n2 = atoi(arg2);
    }
    //生成动态内容，存入缓冲区content
    sprintf(content, "%s result：%d + %d = %d\r\n<p>", content, n1, n2, n1 + n2);

    //产生响应头报文并且写入标准输出
    printf("Connection: close\r\n");
    printf("Content-length: %d\r\n", (int)strlen(content));
    printf("Content-type: text/html\r\n\r\n");
    //将动态内容发送
    printf("%s", content);
    fflush(stdout);

    exit(0);
}
