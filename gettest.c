/* Test getting a line of text from a FIFO.  */

#include <stdio.h>
#include <signal.h>
#include <unistd.h>

unsigned time_to_quit = 0;

void sigint_handler(int signum)
{
  time_to_quit = 1;
}

void show_help()
{
  puts(
"Usage: gettest <FIFO>\n"
"Read a line from a FIFO, one at a time.  Put a newline on standard\n"
"input to read the next line.");
}

int main(int argc, char *argv[])
{
  char *fifo_name;
  FILE *fp;
  signal(SIGINT, sigint_handler);

  /* Parse command-line arguments.  */
  {
    int option;
    do
      {
	option = getopt(argc, argv, "h");
	switch (option)
	  {
	  case (int)'h':
	    show_help();
	    return 0;
	  }
      } while (option != -1);
  }
  if (optind >= argc)
    {
      fputs("Error: You must specify a FIFO.\n", stderr);
      return 1;
    }
  fifo_name = argv[optind];

  fp = fopen(fifo_name, "r");
  if (fp == NULL)
    {
      fputs("Error: Could not open FIFO.\n", stderr);
      return 1;
    }
  setvbuf(fp, NULL, _IONBF, BUFSIZ);
  while (!time_to_quit)
    {
      char buffer[256];
      fgets(buffer, 256, fp);
      fputs(buffer, stdout);
      /* Provide a pause mechanism.  */
      fgets(buffer, 256, stdin);
    }
  fclose(fp);
  return 0;
}
