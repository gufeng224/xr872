/*
 * server.c为服务器端代码
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <pthread.h>



const int MAX_LINE = 2048;
const int PORT = 6000;
const int BACKLOG = 10;
const int LISTENQ = 6666;
const int MAX_CONNECT = 20;

int main(int argc , char **argv)
{
	/*声明服务器地址和客户链接地址*/
	struct sockaddr_in servaddr , cliaddr;

	/*声明服务器监听套接字和客户端链接套接字*/
	int listenfd , connfd;
	pid_t childpid;

	/*声明缓冲区*/
	char buf[MAX_LINE];

	socklen_t clilen;

	/*(1) 初始化监听套接字listenfd*/
	if((listenfd = socket(AF_INET , SOCK_STREAM , 0)) < 0)
	{
		perror("socket error");
		exit(1);
	}//if

	/*(2) 设置服务器sockaddr_in结构*/
	bzero(&servaddr , sizeof(servaddr));

	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY); //表明可接受任意IP地址
	servaddr.sin_port = htons(PORT);
	int enable = 1;
	if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
		    printf("setsockopt(SO_REUSEADDR) failed\n");

	/*(3) 绑定套接字和端口*/
	if(bind(listenfd , (struct sockaddr*)&servaddr , sizeof(servaddr)) < 0)
	{
		perror("bind error");
		exit(1);
	}//if

	/*(4) 监听客户请求*/
	if(listen(listenfd , LISTENQ) < 0)
	{
		perror("listen error");
		exit(1);
	}//if

	/*(5) 接受客户请求*/
	for( ; ; )
	{
		clilen = sizeof(cliaddr);
		if((connfd = accept(listenfd , (struct sockaddr *)&cliaddr , &clilen)) < 0 )
		{
			perror("accept error");
			exit(1);
		}//if

		//新建子进程单独处理链接
		if((childpid = fork()) == 0) 
		{
			close(listenfd);
			//str_echo
			ssize_t n;
			char buff[MAX_LINE];
			while((n = read(connfd , buff , MAX_LINE)) > 0)
			{
				printf("now get %s\n",buff);
				write(connfd , buff , n);
				memset(buff,0,MAX_LINE);
			}
			exit(0);
		}//if
		close(connfd);
	}//for
	
	/*(6) 关闭监听套接字*/
	close(listenfd);
}
