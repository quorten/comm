/*
** server.c -- a stream socket server demo
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

/*
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
*/
#include <winsock2.h>
#include <ws2tcpip.h>
#define NEED_ADDRINFO
#define NEED_NTOP
#include "netsupp.h"
#include <pthread.h>
#include "exparray.h"

#define PORT "3490"  // the port users will be connecting to

#define BACKLOG 10	 // how many pending connections queue will hold

CREATE_EXP_ARRAY_TYPE(pthread_t);

int time_to_exit = 0;
pthread_t_array threads;
pthread_mutex_t ti_mutex = PTHREAD_MUTEX_INITIALIZER;

BOOL WINAPI sigint_handler(DWORD ctrlType)
{
	time_to_exit = 1;
	printf("server: shutting down after next event.\n");
	return TRUE;
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

void* print_hello(void* data)
{
	SOCKET new_fd;
	pthread_t mythread;
	unsigned i;

	new_fd = (SOCKET)data;
	if (send(new_fd, "Hello, world!", 13, 0) == SOCKET_ERROR)
		fprintf(stderr, "send error %ld\n", WSAGetLastError());
	closesocket(new_fd);

	// Now cleanup the allocated thread information
	mythread = pthread_self();
	pthread_mutex_lock(&ti_mutex);
	for (i = 0; i < threads.num; i++)
	{
		if (pthread_equal(mythread, threads.d[i]))
		{
			DELETE_ARRAY_ELEMENT(pthread_t, threads, 16, i);
			break;
		}
	}
	pthread_mutex_unlock(&ti_mutex);
	pthread_exit(NULL);
}

void my_ws_cleanup(void)
{
	if (threads.num == 0)
		printf("server: all jobs have finished.\n");
	else
	{
		printf("server: %i job(s) are still running.  Waiting...\n",
			   threads.num);
	}
	pthread_mutex_destroy(&ti_mutex);
	DESTROY_ARRAY(threads);
	WSACleanup();
}

int main(void)
{
	SOCKET sockfd, new_fd;  // listen on sock_fd, new connection on new_fd
	struct addrinfo hints, *servinfo, *p;
	struct sockaddr_storage their_addr; // connector's address information
	socklen_t sin_size;
	char yes = 1;
	char s[INET6_ADDRSTRLEN];
	int rv;

	/* Windows stuff */
	WSADATA wsaData;

	INIT_ARRAY(pthread_t, threads, 16);

	if (WSAStartup(MAKEWORD(2, 0), &wsaData) != 0)
	{
		fprintf(stderr, "WSAStartup failed.\n");
		exit(1);
	}
	atexit(my_ws_cleanup);

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE; // use my IP

	if ((rv = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		pthread_exit((void*)1);
	}

	// loop through all the results and bind to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == SOCKET_ERROR) {
			fprintf(stderr, "server: socket error %ld\n", WSAGetLastError());
			continue;
		}

		/* if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
				sizeof(int)) == SOCKET_ERROR) {
			fprintf(stderr, "setsockopt error %ld\n", WSAGetLastError());
			pthread_exit((void*)1);
		} */

		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == SOCKET_ERROR) {
			closesocket(sockfd);
			fprintf(stderr, "server: bind error %ld\n", WSAGetLastError());
			continue;
		}

		break;
	}

	if (p == NULL)  {
		fprintf(stderr, "server: failed to bind\n");
		pthread_exit((void*)2);
	}

	freeaddrinfo(servinfo); // all done with this structure

	if (listen(sockfd, BACKLOG) == SOCKET_ERROR) {
		fprintf(stderr, "listen error %ld\n", WSAGetLastError());
		pthread_exit((void*)1);
	}

	// Provide a safe shutdown mechanism.
	SetConsoleCtrlHandler(sigint_handler, TRUE);

	printf("server: waiting for connections...\n");

	while(!time_to_exit) {  // main accept() loop
		sin_size = sizeof(their_addr);
		new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
		if (new_fd == INVALID_SOCKET) {
			fprintf(stderr, "accept error %ld\n", WSAGetLastError());
			continue;
		}

		inet_ntop(their_addr.ss_family,
			get_in_addr((struct sockaddr *)&their_addr),
			s, sizeof(s));
		printf("server: got connection from %s\n", s);

		pthread_mutex_lock(&ti_mutex);
		if (pthread_create(&threads.d[threads.num], NULL, print_hello, (void*)new_fd) != 0) {
			pthread_mutex_unlock(&ti_mutex);
			fprintf(stderr, "server: could not spawn thread.\n");
			closesocket(new_fd);
		}
		ADD_ARRAY_ELEMENT(pthread_t, threads, 16);
		pthread_mutex_unlock(&ti_mutex);
	}

	printf("server: shutting down.\n");
	closesocket(sockfd);

	pthread_exit((void*)0);
}
