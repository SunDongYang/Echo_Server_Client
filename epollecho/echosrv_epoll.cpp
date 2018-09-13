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
#include<sys/epoll.h>
#include<stdlib.h>
#include<stdio.h>
#include<errno.h>
#include<string.h>
#include<vector>
#include<iostream>
#include<algorithm>

using namespace std;

#define ERR_EXIT(m)\
	do\
	{\
		perror(m);\
		exit(EXIT_FAILURE);\
	}while(0)

//定义一个向量，用来存储epoll_event

typedef vector<struct epoll_event> EventList;

int main()
{
	signal(SIGPIPE, SIG_IGN);
	signal(SIGCHLD, SIG_IGN);
	int idleFd = open("/dev/null", O_RDONLY | O_CLOEXEC);
	int listenfd;
	//设置套接字为非阻塞
    	if ((listenfd = socket(PF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP))<0)
		ERR_EXIT("socket");
	struct sockaddr_in servaddr;
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(5188);
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	int on = 1;
	if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on))<0)
		ERR_EXIT("setsocketopt");
	if (bind(listenfd, (struct sockaddr*)&servaddr, sizeof(servaddr))<0)
		ERR_EXIT("bind");
	if (listen(listenfd, SOMAXCONN)<0)
		ERR_EXIT("listen");
	vector<int> clients;
	int epollfd;
	epollfd = epoll_create1(EPOLL_CLOEXEC);
	struct epoll_event event;
	event.data.fd = listenfd;
	//监听描述符的可读事件，我们所关心的
	event.events = EPOLLIN;// EPOLLET设置为边缘触发模式；关注他的可读事件，默认是电平触发模式
	epoll_ctl(epollfd, EPOLL_CTL_ADD, listenfd, &event);//把listenfd和event添加到epollfd来进行管理
	EventList events(16);//定义一个事件的列表，初始状态有16个
	struct sockaddr_in peeraddr;
	socklen_t peerlen;
	int connfd;
	int nready;
	while (1)
	{
		nready = epoll_wait(epollfd, &*events.begin(), static_cast<int>(events.size()), -1);//timeout = -1表示远远等待，不设定超时
		//&*events.begin()是一个输出参数，不像poll是一个输入输出参数。关注的事件已经有epoll_ctl负责了，已经添加到epollfd管理了。
		//意味着我们每次并没有把我们关注的事件都拷贝到内核，这也是epoll比poll效率高的一个原因，不需要从用户空间将数据拷贝到内核空间
		////nready为返回的事件数，返回的事件放到events里
		if (nready == -1)//直到发生事件才返回
		{
			if (errno == EINTR)
				continue;
			ERR_EXIT("epoll_wait");
		}
		if (nready == 0)//什么事件都没发生
			continue;
		if ((size_t)nready == events.size())//如果事件向量满了，说明活跃的事件较多，将向量倍增
			events.resize(events.size() * 2);
		//epoll产生的可能是监听套接字事件，也可能是已连接套接字事件，我们统一处理
		for (int i = 0; i < nready; i++)
		{
			if (events[i].data.fd == listenfd)
			{
				peerlen = sizeof(peeraddr);
				connfd = accept4(listenfd, (struct sockaddr*)&peeraddr, &peerlen, SOCK_NONBLOCK | SOCK_CLOEXEC);
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
				cout << "ip=" << inet_ntoa(peeraddr.sin_addr) << " port" << ntohs(peeraddr.sin_port) << endl;
				clients.push_back(connfd);
				event.data.fd = connfd;
				event.events = EPOLLIN;// | EPOLLET
				epoll_ctl(epollfd, EPOLL_CTL_ADD, connfd, &event);
			}
			else if (events[i].events && EPOLLIN)
			{
				connfd = events[i].data.fd;
				if (connfd < 0)
					continue;
				char buf[1024] = { 0 };
				int ret = read(connfd, buf, 1024);
				if (ret == -1)
					ERR_EXIT("read");
				if (ret == 0)
				{
					cout << "client close" << endl;
					close(connfd);
					event = events[i];
					epoll_ctl(epollfd, EPOLL_CTL_DEL, connfd, &event);
					clients.erase(remove(clients.begin(), clients.end(), connfd), clients.end());
					continue;
				}
				cout << buf;
				write(connfd, buf, strlen(buf));
			}
		}
	}
	return 0;
}
