/* A series of socket supplementary functions, useful for backporting
   modern sock applications to obsolete systems.

   NOTE: I found out that most of my code was already written into the
   library `libevent'.  Perhaps I should make some sort of effort to
   standardize on it.  */
#ifndef NETSUPP_H
#define NETSUPP_H

#include <malloc.h>
#include <ctype.h>

#ifndef INET6_ADDRSTRLEN
#define INET6_ADDRSTRLEN 46
#endif

#ifdef NEED_ADDRSTRUCT

/*
 * Desired design of maximum size and alignment
 */
#define _SS_MAXSIZE    128
#define _SS_ALIGNSIZE  (sizeof (__int64)) 
/*
 * Definitions used for sockaddr_storage structure paddings design.
 */
#define _SS_PAD1SIZE   (_SS_ALIGNSIZE - sizeof (short))
#define _SS_PAD2SIZE   (_SS_MAXSIZE - (sizeof (short) \
				       + _SS_PAD1SIZE \
				       + _SS_ALIGNSIZE))
struct sockaddr_storage {
    short ss_family;
    char __ss_pad1[_SS_PAD1SIZE];  /* pad to 8 */
    __int64 __ss_align;  	   /* force alignment */
    char __ss_pad2[_SS_PAD2SIZE];  /*  pad to 128 */
};

/* getaddrinfo constants */
#define AI_PASSIVE	1
#define AI_CANONNAME	2
#define AI_NUMERICHOST	4

typedef int socklen_t;

struct addrinfo {
	int     ai_flags;
	int     ai_family;
	int     ai_socktype;
	int     ai_protocol;
	size_t  ai_addrlen;
	char   *ai_canonname;
	struct sockaddr  *ai_addr;
	struct addrinfo  *ai_next;
};

#endif

#ifdef NEED_GAI_STRERROR
char *gai_strerror(int ecode)
{
	return "could not resolve host";
}
#endif

#ifdef NEED_ADDRINFO
/* A partial replacement for a real `getaddrinfo()'.  Right now, this
   only supports PF_INET protocol families.  Also, `service' must be a
   numeric string representing a port number.  */
int getaddrinfo(char *node, const char *service, struct addrinfo *hints,
				struct addrinfo **res)
{
	struct hostent *host_info;
	unsigned long addr;
	struct sockaddr_in *final;
	struct addrinfo *cur_addrinfo;
	unsigned i;

	host_info = NULL;
	addr = INADDR_NONE;
	if (node == NULL || (hints->ai_flags & AI_PASSIVE) != 0)
	{
		/* char loc_host[128];
		if (gethostname(loc_host, 128) == SOCKET_ERROR)
			return 1;
		node = loc_host; */
		addr = INADDR_ANY;
	}
	else if (isalpha(node[0]))
		host_info = gethostbyname(node);
	else
		addr = inet_addr(node);

	if (host_info == NULL && addr == INADDR_NONE)
		return 1;

	(*res) = (struct addrinfo*)malloc(sizeof(struct addrinfo));
	cur_addrinfo = (*res);

	if (addr != INADDR_NONE)
	{
		final = (struct sockaddr_in*)malloc(sizeof(struct sockaddr_in));
		memset(final, 0, sizeof(struct sockaddr_in));
		final->sin_family = AF_INET;
		final->sin_port = htons((short)atoi(service));
		memcpy(&final->sin_addr, &addr, sizeof(addr));
		cur_addrinfo->ai_flags = hints->ai_flags;
		cur_addrinfo->ai_family = PF_INET;
		cur_addrinfo->ai_socktype = hints->ai_socktype;
		cur_addrinfo->ai_protocol = 0;
		cur_addrinfo->ai_addrlen = sizeof(struct sockaddr_in);
		cur_addrinfo->ai_canonname = "example.com";
		cur_addrinfo->ai_addr = (struct sockaddr*)final;
		cur_addrinfo->ai_next = NULL;
		return 0;
	}

	for (i = 0; ((struct in_addr**)host_info->h_addr_list)[i] != NULL; i++)
	{
		final = (struct sockaddr_in*)malloc(sizeof(struct sockaddr_in));
		memset(final, 0, sizeof(struct sockaddr_in));
		final->sin_family = AF_INET;
		final->sin_port = htons((short)atoi(service));
		memcpy(&final->sin_addr, ((struct in_addr**)host_info->h_addr_list)[i],
			   sizeof(struct in_addr));
		cur_addrinfo->ai_flags = hints->ai_flags;
		cur_addrinfo->ai_family = PF_INET;
		cur_addrinfo->ai_socktype = hints->ai_socktype;
		cur_addrinfo->ai_protocol = 0;
		cur_addrinfo->ai_addrlen = sizeof(struct sockaddr_in);
		cur_addrinfo->ai_canonname = host_info->h_name;
		cur_addrinfo->ai_addr = (struct sockaddr*)final;
		if (((struct in_addr**)host_info->h_addr_list)[i+1] != NULL)
			cur_addrinfo->ai_next =
				(struct addrinfo*)malloc(sizeof(struct addrinfo));
		else
			cur_addrinfo->ai_next = NULL;
		cur_addrinfo = cur_addrinfo->ai_next;
	}

	return 0;
}

void freeaddrinfo(struct addrinfo *ai)
{
	struct addrinfo *ai_next;
	do
	{
		ai_next = ai->ai_next;
		free(ai->ai_addr);
		free(ai);
		ai = ai_next;
	} while (ai_next != NULL);
}
#endif

#ifdef NEED_NTOP
/* Warning: on some systems, this thin-wrapper function may not be
   thread safe.  However, it is thread safe on Winsock 2
   implementations at least.  */
const char *inet_ntop(int af, const void *src, char *dst, socklen_t size)
{
	strncpy(dst, inet_ntoa(*((struct in_addr*)src)), size - 1);
	return dst;
}

int inet_pton(int af, const char *src, void *dst)
{
	dst = (void*)inet_addr(src);
	return 1;
}
#endif

#endif /* not NETSUPP_H */
