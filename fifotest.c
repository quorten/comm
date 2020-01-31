/* Test creating a FIFO.

Public Domain 2012, 2013, 2020 Andrew Makousky

See the file "UNLICENSE" in the top level directory for details.

*/
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#define clean_return(value) { retval = value; goto cleanup; }
#define BUFF_SIZE 255
#define NUM_MESSAGES 12

char *fifo_name = "";
unsigned must_quit = 0;

void sigint_handler(int signum)
{
  must_quit = 1;
}

void sigsegv_handler(int signum)
{
  remove(fifo_name);
  signal(signum, SIG_DFL);
  raise(signum);
}

void show_help()
{
  puts(
"Usage: fifotest [-r] [-w] <FIFO>\n"
"Creates a FIFO and tests it by either reading from it, writing to it,\n"
"or both.\n"
"\n"
"Options:\n"
"  -r  read from the FIFO\n"
"  -w  write to the FIFO\n"
"\n"
"Note that you can read and write to the FIFO at the same time.\n"
"However, you most likely do not want to do this, as the program will\n"
"almost certainly interfere with itself.");
}

int main(int argc, char *argv[])
{
  int retval;
  int fifo_fd = -1;
  fd_set fifo_read_set;
  fd_set fifo_write_set;
  char buffer[BUFF_SIZE+1];
  char *messages[NUM_MESSAGES] =
    {"Howdy.\n", "Hi!\n", "How are you doing?\n",
     "Why do you keep asking me for data?\n",
     "Please, I've answered you enough times already.\n",
     "Leave me alone!\n",
     "Alright, alright, have it your way.  I'll tell you a secret, then "
       "please leave me alone.\n",
     "If you press CTRL+ALT+F1, you can get to a text-mode terminal.  "
       "To get back to the graphical terminal, either press ALT+F7 or "
       "ALT+F8.\n",
     "Hey, why are you still bothering me?  Didn't I already tell you my "
       "secret?\n",
     "Please, please, I'm tired of talking to you.  Just go.\n",
     "You know what?  If you keep bothering me, I'm, I'm, I'm going to "
       "do worse to you!\n",
     "Alright, you asked for it, I'm going to make your computer suffer "
       "a spell of amnesia, then it will loose all data that has been "
       "entered into it in the last %f seconds!\n"};
  unsigned current_msg = 0;
  int read_mode = 0, write_mode = 0;
  time_t start_time;

  start_time = time(NULL);

  /* Configure signal handling.  */
  signal(SIGINT, sigint_handler);
  signal(SIGPIPE, sigint_handler);
  signal(SIGSEGV, sigsegv_handler);

  /* Parse command-line arguments.  */
  {
    int option;
    do
      {
	option = getopt(argc, argv, "hrw");
	switch (option)
	  {
	  case (int)'h':
	    show_help();
	    clean_return(0);
	  case (int)'r':
	    read_mode = 1;
	    break;
	  case (int)'w':
	    write_mode = 1;
	    break;
	  }
      } while (option != -1);
  }
  if (!read_mode && !write_mode)
    {
      fputs("Error: You must specify a mode.\n", stderr);
      clean_return(1);
    }
  if (optind >= argc)
    {
      fputs("Error: You must specify a FIFO.\n", stderr);
      clean_return(1);
    }
  fifo_name = argv[optind];

  /* Create the FIFO.  */
  if (mkfifo(fifo_name, S_IRUSR | S_IWUSR) == -1 &&
      errno != EEXIST)
    {
      perror("mkfifo");
      clean_return(1);
    }

  /* Open and setup the FIFO.  */
  {
    int flags;
    if (read_mode && write_mode)
      flags = O_RDWR | O_NONBLOCK;
    else if (read_mode)
      flags = O_RDONLY;
    else if (write_mode)
      flags = O_WRONLY;
    fifo_fd = open(fifo_name, flags, S_IRUSR | S_IWUSR);
  }
  if (fifo_fd == -1)
    {
      perror("open");
      clean_return(1);
    }
  FD_ZERO(&fifo_read_set); FD_ZERO(&fifo_write_set);
  FD_SET(fifo_fd, &fifo_read_set); FD_SET(fifo_fd, &fifo_write_set);

  /* For some odd reason, on Linux, you can delete a FIFO while
     applications are running any they will still work?  I've tested
     it on FreeBSD, and it works there too.  Why?  */
  if (read_mode)
    remove(fifo_name);

  /* Process I/O on the FIFO.  */
  /* Note that the diagnostic messages we echo describe what the other
     program is doing, not what we are doing.  */
  /* Normally, we would understand the protocol of the program that we
     are interfacing with so that we could only use select() for
     reading and write in response.  (Well, how often is bidirectional
     communication done through a FIFO?)  To tell the truth,
     bidirectional communication through a unidirectional IPC
     mechanism doesn't work, but you are free to experiment.  */
  while (!must_quit)
    {
      if (read_mode && write_mode)
	{
	  FD_SET(fifo_fd, &fifo_read_set); FD_SET(fifo_fd, &fifo_write_set);
	  select(FD_SETSIZE, &fifo_read_set, &fifo_write_set, NULL, NULL);
	}
      if (write_mode && FD_ISSET(fifo_fd, &fifo_write_set))
	{
	  ssize_t size_written;
	  ssize_t total_size = 0;
	  int cur_msg_len;
	  if (current_msg == 11)
	    {
	      time_t current_time;
	      /* The next function is always supposed to block until
		 the user gets to this point in the conversation, but
		 it doesn't do anything.  */
	      fsync(fifo_fd);
	      current_time = time(NULL);
	      sprintf(buffer, messages[current_msg],
		      difftime(current_time, start_time));
	      cur_msg_len = strlen(buffer);
	    }
	  else
	    {
	      cur_msg_len = strlen(messages[current_msg]);
	      memcpy(buffer, &messages[current_msg][total_size], cur_msg_len);
	    }
	  do
	    {
	      size_written = write(fifo_fd, buffer, cur_msg_len - total_size);
	      if (size_written == -1)
		{
		  if (errno == EWOULDBLOCK)
		    break;
		  else if (errno == EPIPE)
		    {
		      perror("write");
		      break;
		    }
		  else
		    {
		      perror("write");
		      clean_return(1);
		    }
		}
	      total_size += size_written;
	    } while (total_size < cur_msg_len);
	  current_msg++;
	  if (current_msg >= NUM_MESSAGES)
	    current_msg = 0;
	}
	else if (read_mode && FD_ISSET(fifo_fd, &fifo_read_set))
	{
	  ssize_t size_read;
	  unsigned last_msg;
	  size_read = read(fifo_fd, buffer, BUFF_SIZE);
	  if (size_read == -1)
	    {
	      perror("read");
	      clean_return(1);
	    }
	  buffer[size_read] = '\0';
	  fputs(buffer, stdout);
	  /* Make sure that we do not accidentally read what we just
	     wrote to the FIFO.  If this happens, then we need to redo
	     the writing.  */
	  if (current_msg == 0) last_msg = NUM_MESSAGES - 1;
	  else last_msg = current_msg--;
	  if (strstr(buffer, messages[last_msg]) != NULL)
	    fputs("Accidental reread occured!\n", stderr);
	  if (size_read == 0 || strstr(buffer, "exit") != NULL)
	    {
	      puts("Good bye!");
	      break;
	    }
	}
    }

  retval = 0;
 cleanup:
  if (fifo_fd != -1) close(fifo_fd);
  remove(fifo_name);
  return retval;
}
