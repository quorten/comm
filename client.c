/* client.c -- a stream socket client demo

Public Domain 2012, 2020 Andrew Makousky

See the file "UNLICENSE" in the top level directory for details.

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef _WINDOWS
#include <unistd.h>
#include <errno.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <arpa/inet.h>

#define SOCKET int
#ifndef WATT32
#define INVALID_SOCKET -1
#endif
#endif

#ifdef _WINDOWS
#include <winsock2.h>
#include <ws2tcpip.h>
#define close closesocket
/* #define errno WSAGetLastError() */
#define NEED_ADDRINFO
#define NEED_NTOP
#endif
#ifdef WATT32
#include <tcp.h>
#define close close_s
#endif

#include "netsupp.h"

#define PORT "3490" /* The port client will be connecting to */

#define MAXDATASIZE 100 /* Max number of bytes we can get at once */

/* Get sockaddr, IPv4 or IPv6 */
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET)
		return &(((struct sockaddr_in*)sa)->sin_addr);

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
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

void my_ws_cleanup(void)
{
	WSACleanup();
}
#endif

int main(int argc, char *argv[])
{
	SOCKET sockfd;
	int numbytes;  
	char buf[MAXDATASIZE];
	struct addrinfo hints, *servinfo, *p;
	int rv;
	char s[INET6_ADDRSTRLEN];
#ifdef _WINDOWS
	WSADATA wsaData;
#endif

	if (argc != 2)
	{
	    fprintf(stderr,"usage: client hostname\n");
	    exit(1);
	}

#ifdef _WINDOWS
	if (WSAStartup(MAKEWORD(2, 0), &wsaData) != 0)
	{
		fprintf(stderr, "WSAStartup failed.\n");
		exit(1);
	}
	atexit(my_ws_cleanup);
#endif
#ifdef WATT32
        if (getenv("TFTP_DEBUG"))
			dbug_init();
        sock_init();
#endif

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	if ((rv = getaddrinfo(argv[1], PORT, &hints, &servinfo)) != 0)
	{
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		exit(1);
	}

	/* Loop through all the results and connect to the first we can.  */
	for(p = servinfo; p != NULL; p = p->ai_next)
	{
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == INVALID_SOCKET)
		{
			perror("client: socket");
			continue;
		}

		if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1)
		{
			close(sockfd);
			perror("client: connect");
			continue;
		}

		break;
	}

	if (p == NULL)
	{
		fprintf(stderr, "client: failed to connect\n");
		exit(2);
	}

	inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
			s, sizeof(s));
	printf("client: connecting to %s\n", s);

	freeaddrinfo(servinfo); /* All done with this structure */

	if ((numbytes = recv(sockfd, buf, MAXDATASIZE-1, 0)) == -1)
	{
	    perror("recv");
	    exit(1);
	}

	buf[numbytes] = '\0';

	printf("client: received '%s'\n",buf);

	close(sockfd);

	exit(0);
}
