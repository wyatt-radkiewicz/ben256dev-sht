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
int check_lit(int complain)
{
   if (access(".lit", F_OK))
   {
      if (complain)
         printf("not a lit repository\n");
      return -1;
   }
   if (access(".lit/objects", F_OK))
   {
      if (complain)
         printf("warning: directory \".lit/objects\" not found\n");
      return -1;
   }
   if (access(".lit/refs", F_OK))
   {
      if (complain)
         printf("warning: directory \".lit/refs\" not found\n");
      return -1;
   }

   return 0;
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

   if (check_lit(1))
   {
      printf("  (run \"lit init\" to reinitialize repository)\n");
      exit(EXIT_FAILURE);
   }

   FILE* status_directories_file = fopen(".lit/status-directories.lit", "w");
   if (status_directories_file == NULL)
      goto DETERMINE_OBJECTS_RET_NULL;

   struct dirent** files;
   int file_count = scandir(".", &files, &status_filter, &status_compar);

   int ent = 0;
   if (files[ent]->d_type == DT_DIR)
   {
      fprintf(status_directories_file, "Untracked Directories:\n  (NOTE: lit doesn't support nesting of files)\n");
   }
   for (; ent < file_count && files[ent]->d_type == DT_DIR; ent++)
   {
      fprintf(status_directories_file, "    /%s\n", files[ent]->d_name);
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
   return status_directories_file;

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
      if (access(".lit/", F_OK))
      {
         mkdir(".lit/", 0777);
         mkdir(".lit/objects/", 0777);
         mkdir(".lit/refs/", 0777);
         printf("lit repository created\n");
      }
      else
      {
         if (check_lit(0))
         {
            mkdir(".lit/objects/", 0777);
            mkdir(".lit/refs/", 0777);
            printf("Missing lit files re-initialzed\n");
         }
         else
            printf("Already a lit repository\n");
      }

      return 0;
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
      if (fp == NULL)
      {
         perror("Error: failed to open status_file");
         return -1;
      }

      char*  pardir  = malloc(512);
      char*  hash    = malloc(65);
      char*  ref     = malloc(128);

      FILE* status_tracked_file = fopen(".lit/status-tracked.lit", "w");
      if (status_tracked_file == NULL)
      {
         perror("Error: failed to open status_tracked_file");
         goto STATUS_FREE_BUFFERS;
      }
      FILE* status_untracked_file = fopen(".lit/status-untracked.lit", "w");
      if (status_untracked_file == NULL)
      {
         perror("Error: failed to open status_untracked_file");
         goto STATUS_FREE_BUFFERS;
      }

      int tracked_count = 0;
      for (; fscanf(fp, "%64s %128s", hash, ref) != EOF; )
      {
         snprintf(pardir, 512, ".lit/objects/%.2s/", hash);
         if (access(pardir, F_OK))
            goto STATUS_FILE_IS_UNTRACKED;

         snprintf(pardir, 512, ".lit/objects/%.2s/%.62s/", hash, hash+2);
         if (access(pardir, F_OK))
            goto STATUS_FILE_IS_UNTRACKED;

         tracked_count++;
         fprintf(status_tracked_file, "  %s\n", ref);
         continue;

STATUS_FILE_IS_UNTRACKED:
         fprintf(status_untracked_file, "  %s\n", ref);
      }
      if (ferror(fp))
         perror("Error while reading status.lit");

      fclose(fp);
      fclose(status_tracked_file);
      fclose(status_untracked_file);

      if (reg_count)
      {
         if (dir_count)
         {
            FILE* status_directories_file = fopen(".lit/status-directories.lit", "r");
            if (status_directories_file == NULL)
            {
               perror("Error: failed to open status_directories_file");
               return -1;
            }
            for (char c; ( c = fgetc(status_directories_file) ) != EOF; putchar(c));
            fclose(status_directories_file);
         }
         if (tracked_count)
         {
            if (dir_count)
               printf("\n");
            printf("Tracked Files:\n");
            status_tracked_file = fopen(".lit/status-tracked.lit", "r");
            if (status_tracked_file == NULL)
            {
               perror("Error: failed to open status_tracked_file");
               return -1;
            }
            for (char c; ( c = fgetc(status_tracked_file) ) != EOF; putchar(c));
            fclose(status_tracked_file);
         }
         if (tracked_count < reg_count)
         {
            if (dir_count + tracked_count)
               printf("\n");
            printf("Untracked Files:\n");
            status_untracked_file = fopen(".lit/status-untracked.lit", "r");
            if (status_untracked_file == NULL)
            {
               perror("Error: failed to open status_untracked_file");
               return -1;
            }
            for (char c; ( c = fgetc(status_untracked_file) ) != EOF; putchar(c));
            fclose(status_untracked_file);
         }
      }

STATUS_FREE_BUFFERS:
      free(pardir);
      free(hash);
      free(ref);

      return 0;
   }
   else if (strcmp(argv[1], "add") == 0)
   {
      if (argc < 3)
      {
         printf("Nothing specified for \"%s add\"\n", argv[0]);
         printf("  try running \"%s add [FILE] ...\" to track files\n", argv[0]);
      }
      char*  hash    = malloc(65);
      char*  ref     = malloc(128);

      FILE* status_file = fopen(".lit/status.lit", "r");
      if (status_file == NULL)
      {
         perror("Error: failed to open status_file");
         return -1;
      }

      const int add_argc = argc - 2;
      char** args_need_match = calloc(add_argc, sizeof(char*));
      if (args_need_match == NULL)
      {
         perror("Error: failed to malloc args_need_match");
         return -1;
      }
      for (int i = 0; i < add_argc; i++)
      {
         args_need_match[i] = malloc(strlen(argv[i + 2]) + 1);
         if (strncmp(argv[i + 2], "--", 2) == 0 || strncmp(argv[i], "-", 2) == 0)
         {
            printf("Warning: skipping argument \"%s\"\n", argv[i + 2]);
            printf("  (NOTE: \"%s %s\" does not support flags)\n", argv[0], argv[1]);
            args_need_match[i] = NULL;
            continue;
         }
         strcpy(args_need_match[i], argv[i + 2]);
      }

      FILE* added_file = fopen("lit.add-track.lit", "w");
      if (added_file == NULL)
      {
         perror("Error: failed to open added file");
         return -1;
      }

      for (; fscanf(status_file, "%64s %128s", hash, ref) != EOF; )
      {
         for (int i = 0; i < add_argc; i++)
         {
            if (args_need_match[i] == NULL)
               continue;
            if (strcmp(ref, args_need_match[i]) == 0)
            {
               fprintf(added_file, "%s\n", args_need_match[i]);
               args_need_match[i] = NULL;
            }
         }

      }

      for (int i = 0; i < add_argc; i++)
      {
         if (args_need_match[i] == NULL)
            continue;
         
         printf("Error: \"%s\" does not match any status ref\n", args_need_match[i]);
      }
      /*
      for (; fscanf(fp, "%64s %128s", hash, ref) != EOF; )
      {
         snprintf(pardir, 512, ".lit/objects/%.2s/", hash);
         if (access(pardir, F_OK))
            goto STATUS_FILE_IS_UNTRACKED;

         snprintf(pardir, 512, ".lit/objects/%.2s/%.62s/", hash, hash+2);
*/
      free(args_need_match);

      return 0;
   }
}
