/*
** server.c -- a stream socket server demo
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef _WINDOWS
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

#define SOCKET int
#ifndef WATT32
#define INVALID_SOCKET -1
#endif
#define THREAD_FUNC void*

#else /* _WINDOWS */
#include <winsock2.h>
#include <ws2tcpip.h>
#define close closesocket
/* #define errno WSAGetLastError() */
#define NEED_ADDRINFO
#define NEED_NTOP
#define THREAD_FUNC DWORD WINAPI
#define sleep(sec) Sleep(sec * 1000)
#endif /* not _WINDOWS */

#ifdef WATT32
#include <tcp.h>
#define close close_s
#endif

#include "netsupp.h"

#include <pthread.h>
#include <glib/garray.h>

#define G_ARRAY_TYPE_ALIAS(typename)			\
	struct typename##_array_tag					\
	{											\
		typename* d;							\
		guint len;								\
	};											\
	typedef struct typename##_array_tag typename##_array;

G_ARRAY_TYPE_ALIAS(pthread_t);

#define PORT "3490"  /* The port users will be connecting to */

#define BACKLOG 10	 /* How many pending connections queue will hold */

int time_to_exit = 0;
int exit_wait = 0;
pthread_t_array *threads;
pthread_mutex_t ti_mutex; /* = PTHREAD_MUTEX_INITIALIZER; */

#ifndef _WINDOWS
void sigint_handler(int s)
{
	time_to_exit = 1;
}
#else
BOOL WINAPI sigint_handler(DWORD ctrlType)
{
	if (exit_wait == 1)
	{
		exit_wait = 0;
		printf("server: not waiting for jobs.\n");
	}
	else
	{
		time_to_exit = 1;
		printf("server: shutting down after next event.\n");
	}
	return TRUE;
}
#endif

/* Get sockaddr, IPv4 or IPv6 */
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET)
		return &(((struct sockaddr_in*)sa)->sin_addr);

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

THREAD_FUNC print_hello(void* data)
{
	SOCKET new_fd;
	pthread_t mythread;
	unsigned i;

	new_fd = (SOCKET)data;
	if (send(new_fd, "Hello, world!", 13, 0) == -1)
		perror("server: send");
	close(new_fd);

	/* Now cleanup the allocated thread information.  */
	mythread = pthread_self();
	pthread_mutex_lock(&ti_mutex);
	for (i = 0; i < threads->len; i++)
	{
		if (pthread_equal(mythread, threads->d[i]))
		{
			g_array_remove_index((GArray*)threads, i);
			break;
		}
	}
	pthread_mutex_unlock(&ti_mutex);
	pthread_exit((void*)0);
}

#ifdef _WINDOWS
void perror(const char *desc)
{
	LPVOID lpMsgBuf;
	FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
        NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR)&lpMsgBuf, 0, NULL);
	fputs(desc, stderr); fputs(": ", stderr);
	fputs(lpMsgBuf, stderr);
	LocalFree(lpMsgBuf);
}
#endif

void my_cleanup(int retval)
{
	exit_wait = 1;
	while (threads.num != 0 && exit_wait == 1)
	{
		printf("server: %i job(s) are still running.  Waiting...\n",
			   threads.num);
		sleep(1);
	}
	if (exit_wait == 1)
		printf("server: all jobs have finished.\n");
	pthread_mutex_destroy(&ti_mutex);
	g_array_free((GArray*)threads, TRUE);
#ifdef _WINDOWS
	WSACleanup();
#endif
	pthread_exit((void*)retval);
}

int main(void)
{
	SOCKET sockfd, new_fd;  /* Listen on sock_fd, new connection on new_fd */
	struct addrinfo hints, *servinfo, *p;
	struct sockaddr_storage their_addr; /* Connector's address information */
	socklen_t sin_size;
	struct sigaction sa;
	int yes = 1;
	char s[INET6_ADDRSTRLEN];
	int rv;
#ifdef _WINDOWS
	WSADATA wsaData;
#endif

#ifdef _WINDOWS
	if (WSAStartup(MAKEWORD(2, 0), &wsaData) != 0)
	{
		fprintf(stderr, "WSAStartup failed.\n");
		exit(1);
	}
#endif
#ifdef WATT32
        if (getenv("TFTP_DEBUG"))
			dbug_init();
        sock_init();
#endif

	threads = (pthread_t_array*)g_array_new(FALSE, FALSE, sizeof(pthread_t));
	pthread_mutex_init(&ti_mutex, NULL);

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE; /* Use my IP */

	if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0)
	{
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		my_cleanup(1);
	}

	/* Loop through all the results and bind to the first we can.  */
	for(p = servinfo; p != NULL; p = p->ai_next)
	{
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == INVALID_SOCKET)
		{
			perror("server: socket");
			continue;
		}

		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
				sizeof(int)) == -1)
		{
			perror("setsockopt");
			my_cleanup(1);
		}

		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1)
		{
			close(sockfd);
			perror("server: bind");
			continue;
		}

		break;
	}

	if (p == NULL)
	{
		fprintf(stderr, "server: failed to bind\n");
		my_cleanup(2);
	}

	freeaddrinfo(servinfo); /* All done with this structure */

	if (listen(sockfd, BACKLOG) == -1)
	{
		perror("listen");
		my_cleanup(1);
	}

#ifndef _WINDOWS
	sa.sa_handler = sigint_handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	if (sigaction(SIGINT, &sa, NULL) == -1)
	{
		perror("sigaction");
		my_cleanup(1);
	}
#else
	SetConsoleCtrlHandler(sigint_handler, TRUE);
#endif

	printf("server: waiting for connections...\n");

	while(!time_to_exit) /* Main accept() loop */
	{
		sin_size = sizeof(their_addr);
		new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
		if (new_fd == INVALID_SOCKET)
		{
			perror("accept");
			continue;
		}

		inet_ntop(their_addr.ss_family,
			get_in_addr((struct sockaddr *)&their_addr),
			s, sizeof(s));
		printf("server: got connection from %s\n", s);

		pthread_mutex_lock(&ti_mutex);
		g_array_set_size((GArray*)threads, threads->len + 1);
		if (pthread_create(&threads->d[threads->len-1], NULL, print_hello,
						   (void*)new_fd) != 0)
		{
			fprintf(stderr, "server: could not spawn thread.\n");
			g_array_set_size((GArray*)threads, threads->len - 1);
			close(new_fd);
		}
		pthread_mutex_unlock(&ti_mutex);
	}

	printf("server: shutting down.\n");
	close(sockfd);

	my_cleanup(0);
}
