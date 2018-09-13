#include<unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include<signal.h>
#include<sys/wait.h>
#include<poll.h>
#include<stdlib.h>
#include<stdio.h>
#include<errno.h>
#include<string.h>
#include<vector>
#include<iostream>
#define ERR_EXIT(m)\
	do\
	{\
		perror(m);\
		exit(EXIT_FAILURE);\
	}while(0)
//定义一个向量，用来存储文件描述符
using namespace std;
int main()
{
	int socketfd;
	//设置套接字为非阻塞
	if ((socketfd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP))<0)
		ERR_EXIT("socket");
	struct sockaddr_in servaddr;
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(5188);
	servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	if (connect(socketfd, (struct sockaddr*)&servaddr, sizeof(servaddr))<0)
		ERR_EXIT("connect");
	struct sockaddr_in localaddr;
	socklen_t addrlen = sizeof(localaddr);
	if (getsockname(socketfd, (struct sockaddr*)&localaddr, &addrlen)<0)
		ERR_EXIT("getsockname");
	cout << "ip=" << inet_ntoa(localaddr.sin_addr) << " port=" << ntohs(localaddr.sin_port) << endl;
	char sendbuf[1024] = { 0 };
	char recvbuf[1024] = { 0 };
	while (fgets(sendbuf, sizeof(sendbuf), stdin) != NULL)
	{
		write(socketfd, sendbuf, strlen(sendbuf));
		read(socketfd, recvbuf, sizeof(recvbuf));
		fputs(recvbuf, stdout);
		memset(sendbuf, 0, sizeof(sendbuf));
		memset(recvbuf, 0, sizeof(recvbuf));
	}
	close(socketfd);	
	return 0;
}
