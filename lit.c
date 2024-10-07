#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

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
FILE* determine_objects(int* dir_count_ptr, int* reg_count_ptr)
{
   if (dir_count_ptr == NULL || reg_count_ptr == NULL)
      goto DETERMINE_OBJECTS_RET_NULL;
   int dir_count = 0;
   int reg_count = 0;

   check_lit();

   FILE* status_untracked_file = fopen(".lit/status-untracked.lit", "w");
   if (status_untracked_file == NULL)
      goto DETERMINE_OBJECTS_RET_NULL;

   struct dirent** files;
   int file_count = scandir(".", &files, &status_filter, &status_compar);

   int ent = 0;
   if (files[ent]->d_type == DT_DIR)
   {
      fprintf(status_untracked_file, "Untracked Directories:\n  (NOTE: lit doesn't support nesting of files)\n");
   }
   for (; ent < file_count && files[ent]->d_type == DT_DIR; ent++)
   {
      fprintf(status_untracked_file, "    /%s\n", files[ent]->d_name);
      free(files[ent]);
      dir_count++;
   }

   const FILE* status_file = freopen(".lit/status.lit", "w", stdout);
   if (status_file == NULL)
      goto DETERMINE_OBJECTS_RET_NULL;

   for (; ent < file_count && files[ent]->d_type == DT_REG; ent++)
   {
      size_t command_n = strlen(files[ent]->d_name) + 7;
      char* command_s = malloc(command_n);
      if (command_s == NULL)
         exit(EXIT_FAILURE);

      snprintf(command_s, command_n, "b3sum %s", files[ent]->d_name);
      int rv = system(command_s);
      if (rv == -1)
         perror("b3sum sys call failed");

      free(files[ent]);

      reg_count++;
   }
   const FILE* stdfp = freopen("/dev/tty", "w", stdout);
   if (stdfp == NULL)
      goto DETERMINE_OBJECTS_RET_NULL;

   for (; ent < file_count; ent++)
   {
      free(files[ent]);
   }
   free(files);

   *dir_count_ptr      = dir_count;
   *reg_count_ptr      = reg_count;
   return status_untracked_file;

DETERMINE_OBJECTS_RET_NULL:
   return NULL;
}
int main(int argc, const char* argv[])
{ 
   if (argc < 2)
   {
      print_usage();
      return -1;
   }

   if (strcmp(argv[1], "init") == 0)
   {
      mkdir(".lit/", 777);
      mkdir(".lit/objects/", 777);
      mkdir(".lit/refs/", 777);
   }
   else if (strcmp(argv[1], "status") == 0)
   {
      int    dir_count;
      int    reg_count;
      FILE* rv = determine_objects(&dir_count, &reg_count);
      if (rv == NULL)
      {
         return -1;
      }
      fclose(rv);

      FILE* fp = fopen(".lit/status.lit", "r");

      char*  pardir  = malloc(512);
      char*  hash    = malloc(65);
      char*  ref     = malloc(128);

      FILE* status_tracked_file = fopen(".lit/status-tracked.lit", "w");

      int tracked_count = 0;
      for (; fscanf(fp, "%64s %128s", hash, ref) != EOF; )
      {
         snprintf(pardir, 512, ".lit/objects/%.2s/", hash);
         if (access(pardir, F_OK))
            continue;

         snprintf(pardir, 512, ".lit/objects/%.2s/%.62s/", hash, hash+2);
         if (access(pardir, F_OK))
            continue;

         tracked_count++;
         fprintf(status_tracked_file, "  %s\n", ref);
      }
      if (ferror(fp))
         perror("Error while reading status.lit");

      fclose(status_tracked_file);

      //printf("%d:%d:%d\n", dir_count, reg_count, tracked_count);
      if (reg_count)
      {
         if (tracked_count)
         {
            if (dir_count)
            {
               FILE* status_untracked_file = fopen(".lit/status-untracked.lit", "r");
               for (char c; ( c = fgetc(status_untracked_file) ) != EOF; putchar(c));
               fclose(status_untracked_file);
               printf("\n");
            }

            printf("Tracked Files:\n");
            status_tracked_file = fopen(".lit/status-tracked.lit", "r");
            for (char c; ( c = fgetc(status_tracked_file) ) != EOF; putchar(c));
            fclose(status_tracked_file);
         }
      }

      fclose(fp);

      free(pardir);
      free(hash);
      free(ref);
   }
}
