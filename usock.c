/* Create a Unix-domain socket and write everything that gets sent to
   it to standard output.  Stop after 100 bytes are transferred.

Public Domain 2012, 2020 Andrew Makousky

See the file "UNLICENSE" in the top level directory for details.

*/

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

int main()
{
  int retval;
  const char *filename = "/home/andrew/drop-file";
  int sockfd = -1, readfd = -1;
  struct sockaddr_un name;
  socklen_t name_len;
  unsigned size_read;

  /* Create the socket.  */
  sockfd = socket(PF_UNIX, SOCK_STREAM, 0);
  if (sockfd == -1)
    {
      perror("socket");
      retval = 1; goto cleanup;
    }

  /* Bind the socket.  */
  name.sun_family = AF_UNIX;
  strncpy(name.sun_path, filename, sizeof(name.sun_path));
  name.sun_path[sizeof(name.sun_path)-1] = '\0';
  name_len = SUN_LEN(&name);
  if (bind(sockfd, (struct sockaddr *)&name, name_len) == -1)
    {
      perror("bind");
      retval = 1; goto cleanup;
    }

  /* Accept one, and only one, connection to the socket.  */
  if (listen(sockfd, 1) == -1)
    {
      perror("listen");
      retval = 1; goto cleanup;
    }
  puts("Waiting for a connection...");
  readfd = accept(sockfd, (struct sockaddr *)&name, &name_len);
  if (readfd == -1)
    {
      perror("accept");
      retval = 1; goto cleanup;
    }
  puts("Connection received.");

  /* Read 100 bytes from the socket.  */
  size_read = 0;
  fputs("Data received: ", stdout);
  while (size_read < 100)
    {
      char buffer[101];
      int recv_size;
      recv_size = recv(readfd, buffer, 100 - size_read, 0);
      if (recv_size == -1)
	{
	  perror("recv");
	  retval = 1; goto cleanup;
	}
      size_read += recv_size;
      buffer[size_read] = '\0';
      fputs(buffer, stdout);
    }
  retval = 0;
 cleanup:
  if (sockfd != -1)
    close(sockfd);
  if (readfd != -1)
    close(readfd);
  remove(filename);
  return retval;
}
