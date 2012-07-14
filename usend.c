/* Send data to a Unix-domain socket.  */

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

int main()
{
  int retval;
  char *filename = "/home/andrew/drop-file";
  int sockfd = -1;
  struct sockaddr_un name;
  socklen_t name_len;
  int total_bytes;
  char *transbuf = "Hello there!  I am trying to send 100 bytes of data, and well, it turns out that it is easy enough.\n";

  /* Create a socket.  */
  sockfd = socket(PF_UNIX, SOCK_STREAM, 0);
  if (sockfd == -1)
    {
      perror("socket");
      retval = 1; goto cleanup;
    }

  /* Connect to the socket file.  */
  name.sun_family = AF_UNIX;
  strncpy(name.sun_path, filename, sizeof(name.sun_path));
  name.sun_path[sizeof(name.sun_path)-1] = '\0';
  name_len = SUN_LEN(&name);
  if (connect(sockfd, (struct sockaddr *)&name, name_len) == -1)
    {
      perror("connect");
      retval = 1; goto cleanup;
    }

  /* Send 100 bytes of data.  */
  total_bytes = 0;
  while (total_bytes < 100)
    {
      int bytes_sent;
      bytes_sent = send(sockfd, &transbuf[total_bytes], 100 - total_bytes, 0);
      if (bytes_sent == -1)
	{
	  perror("send");
	  retval = 1; goto cleanup;
	}
      total_bytes += bytes_sent;
    }
  retval = 0;
 cleanup:
  if (sockfd != -1)
    close(sockfd);
  return retval;
}
