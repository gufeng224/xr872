

/*
 * client.c为客户端代码
*/

//#include <stdio.h>
//#include <stdlib.h>
//#include <string.h>
//#include <sys/socket.h>
//#include <sys/types.h>
//#include <errno.h>
//#include <netinet/in.h>
//#include <netdb.h>
//#include <arpa/inet.h>
//#include <unistd.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <lwip/tcpip.h>
#include <lwip/inet.h>
#include "lwip/sockets.h"
#include <lwip/ip.h>
#include <lwip/icmp.h>
#include <lwip/inet_chksum.h>
#include <unistd.h>


#include <pthread.h>

const int PORT = 6000;
/*readline函数实现*/
static ssize_t s_readline(int fd, char *vptr, size_t maxlen)
{
	ssize_t	n, rc;
	char	c, *ptr;

	ptr = vptr;
	for (n = 1; n < maxlen; n++) {
		if ( (rc = read(fd, &c,1)) == 1) {
			*ptr++ = c;
			if (c == '\n')
				break;	/* newline is stored, like fgets() */
		} else if (rc == 0) {
			*ptr = 0;
			return(n - 1);	/* EOF, n - 1 bytes were read */
		} else {
		    printf("now read get error:%d\n",rc);
			return(rc);		/* error, errno set by read() */
		}
	}

	*ptr = 0;	/* null terminate like fgets() */
	return(n);
}
int sockfd;

void socket_send()
{
	char sendline[960];
	int i = 0;
	int ret = 0;
	while(1) {
		for(i=0;i<10;i++) {
       		memset(sendline, 48+i, 959);
			sendline[959] = '\0';
			ret = write(sockfd , sendline , strlen(sendline));
			if(ret < 0) {
				printf("now write error %d\n",ret);
				exit(1);
			} else if(ret == 0)
			{
				printf("now no write %d\n",ret);
				usleep(100000);
			} else {
				usleep(30000);
			}
		}
	}
}

void socket_read()
{
	char recvline[960];
	int ret = 0;
	while(1) {
		ret = s_readline(sockfd , recvline , 960);
		if(ret < 0)
		{
			printf("server terminated prematurely ret:%d\n",ret);
			exit(1);
		}
		//printf("now read buf %s\n",recvline);
		memset(recvline, 0, 960);
	}
}

int socket_test(char *server_ip)
{
	/*声明套接字和链接服务器地址*/

	struct sockaddr_in servaddr;

	/*(1) 创建套接字*/
	if((sockfd = socket(AF_INET , SOCK_STREAM , 0)) == -1)
	{
		printf("socket error\n");
		exit(1);
	}//if

	/*(2) 设置链接服务器地址结构*/
	bzero(&servaddr , sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(PORT);
	if(inet_pton(AF_INET,server_ip, &servaddr.sin_addr) < 0)
	{
		printf("inet_pton error for %s\n",server_ip);
		exit(1);
	}//if

reconnect:
	/*(3) 发送链接服务器请求*/
	if( connect(sockfd , (struct sockaddr *)&servaddr , sizeof(servaddr)) < 0)
	{
		printf("connect error,reconnect\r\n");
		sleep(2);
		goto reconnect;
	//	exit(1);
	}//if

	pthread_t pthread_send;
		//play need init first,then init record
	(void) pthread_create(&pthread_send, NULL,socket_send,NULL);
	pthread_detach(pthread_send);

	pthread_t pthread_read;
		//play need init first,then init record
	(void) pthread_create(&pthread_read, NULL,socket_read,NULL);
	pthread_detach(pthread_read);

	//while(1);

	/*(5) 关闭套接字*/
	//close(sockfd);
}
