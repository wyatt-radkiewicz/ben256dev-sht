#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char* argv[])
{
   if (argc < 2)
      goto EXIT_NO_COMMAND;

   int status = -1;
   if (strcmp(argv[1], "init") == 0)
      status = system("./sh/init.sh");

   return status;

EXIT_NO_COMMAND:
	return 0;

EXIT_COMMAND_NOT_FOUND:
   return -1;
}
