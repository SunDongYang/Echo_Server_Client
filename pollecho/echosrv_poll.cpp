#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
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
typedef std::vector<struct pollfd> PollFdList;

int main()
{
	signal(SIGPIPE, SIG_IGN);
	signal(SIGCHLD, SIG_IGN);
	int idleFd = open("/dev/null", O_RDONLY | O_CLOEXEC);
	int listenfd;	//设置套接字为非阻塞
	if ((listenfd = socket(PF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP))<0)
		ERR_EXIT("socket");
	struct sockaddr_in servaddr;
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;	servaddr.sin_port = htons(5188);
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	int on = 1;
	if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on))<0)
		ERR_EXIT("setsocketopt");
	if (bind(listenfd, (struct sockaddr*)&servaddr, sizeof(servaddr))<0)
		ERR_EXIT("bind");
	if (listen(listenfd, SOMAXCONN)<0)
		ERR_EXIT("listen");
	struct pollfd pfd;
	pfd.fd = listenfd;//监听描述符的可读事件，我们所关心的
	pfd.events = POLLIN;
	PollFdList pollfds;
	pollfds.push_back(pfd);//把文件描述符添加到向量中
	int nready;
	struct sockaddr_in peeraddr;
	socklen_t peerlen;
	int connfd;
	while (1)
	{
		nready = poll(&*pollfds.begin(), pollfds.size(), -1);//timeout = -1表示远远等待，不设定超时
		if (nready == -1)//直到发生事件才返回
		{
			if (errno == EINTR)
				continue;
			ERR_EXIT("poll");
		}
		if (nready == 0)//什么事件都没发生
			continue;
		if (pollfds[0].revents && POLLIN)
		{
			peerlen = sizeof(peeraddr);
			connfd = accept4(listenfd, (struct sockaddr*)&peeraddr, &peerlen, SOCK_NONBLOCK | SOCK_CLOEXEC);
			//accept4比accept函数更新，可设置非阻塞和标记功能得到一个已连接套接字
			/*
			 *         if(connfd == -1)			ERR_EXIT("accdpt4");			*/
			if (connfd == -1)
			{
				if (errno == EMFILE)
				{
					close(idleFd);
					idleFd = accept(listenfd, NULL, NULL);
					close(idleFd);
					idleFd = open("/dev/null", O_RDONLY | O_CLOEXEC);
					continue;
				}
				else
					ERR_EXIT("accept4");
			}
			pfd.fd = connfd;
			pfd.events = POLLIN;
			pfd.revents = 0;//返回时间设为0
			pollfds.push_back(pfd);
			--nready;
			//连接成功
			std::cout << "ip=" << inet_ntoa(peeraddr.sin_addr) << " port" << ntohs(peeraddr.sin_port) << std::endl;
			if (nready == 0)//说明事件都处理完了
				continue;
		}
		//      std::cout<<pollfds.size()<<std::endl;
		//      std::cout<<nready<<std::endl;
			for (PollFdList::iterator it = pollfds.begin() + 1; it != pollfds.end() && nready>0; ++it)
				//第一个套接字一般都是监听套接字，所以从begin()+1开始
			{
				if (it->revents & POLLIN)//遍历过程发现可读事件
				{
					--nready;
					connfd = it->fd;
					char buf[1024] = { 0 };
					int ret = read(connfd, buf, 1024);
					if (ret == -1)
						ERR_EXIT("read");
					if (ret == 0)//如果等于0，说明对方关闭了套接字
					{
						std::cout << "client close" << std::endl;
						it = pollfds.erase(it);
						--it;
						close(connfd);
						continue;
					}
					std::cout << buf;
					write(connfd, buf, strlen(buf));
				}
			}
	}
	return 0;
}
