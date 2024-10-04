#include <stdlib.h>
#include <stdio.h>

int main(int argc, char* argv[])
{
   if (argc < 2)
      goto EXIT_NO_COMMAND;

   char argbuff[256];

   for (int i = 0; i < argc; i++)
      snprintf(argbuff, 256, "%s", argv[i]);

   int status = system(argbuff);

   return status;

EXIT_NO_COMMAND:
	return 0;

EXIT_COMMAND_NOT_FOUND:
   return -1;
}
