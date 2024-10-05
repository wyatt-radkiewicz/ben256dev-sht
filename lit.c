#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

void print_usage()
{
   printf("usage: lit [<command>]\n");
}
void check_lit()
{
   if (access(".lit", F_OK))
   {
      printf("not a lit repository\n");
      printf("  (run \"lit init\" to initialize repository)\n");
      exit(EXIT_FAILURE);
   }
   if (access(".lit/objects", F_OK))
   {
      printf("Error: directory \".lit/objects\" not found\n");
      printf("  (run \"lit init\" to reinitialize repository)\n");
      exit(EXIT_FAILURE);
   }
   if (access(".lit/refs", F_OK))
   {
      printf("Error: directory \".lit/refs\" not found\n");
      printf("  (run \"lit init\" to reinitialize repository)\n");
      exit(EXIT_FAILURE);
   }
}
int status_filter(const struct dirent* ent)
{
   return ent->d_type == DT_REG || ent->d_type == DT_DIR;
}
int status_compar(const struct dirent** a, const struct dirent** b)
{
   return ((*a)->d_type - (*b)->d_type);
}
int main(int argc, char* argv[])
{ 
   if (argc < 2)
      print_usage();

   if (strcmp(argv[1], "init") == 0)
   {
      mkdir(".lit/", 0777);
      mkdir(".lit/objects/", 0777);
      mkdir(".lit/refs/", 0777);
   }
   else if (strcmp(argv[1], "status") == 0)
   {
      check_lit();

      struct dirent** files;
      int file_count = scandir(".", &files, &status_filter, &status_compar);

      for (int i = 0; i < file_count; i++)
      {
         if (files[i]->d_type == DT_REG)
         {
            size_t command_n = strlen(files[i]->d_name) + 7;
            char* command_s = malloc(command_n);
            if (command_s == NULL)
               return -1;

            snprintf(command_s, command_n, "b3sum %s", files[i]->d_name);

            int rv = system(command_s);
         }
         else
            printf("/%s\n", files[i]->d_name);

         free(files[i]);
      }
      free(files);
   }
}
