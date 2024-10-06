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
FILE* determine_objects(int* dir_count, int* reg_count, char* restrict untracked_buff, size_t* buff_size)
{
   int dc = 0;
   int rc = 0;
   buff_size = NULL;
   untracked_buff = NULL;

   check_lit();

   int buff_s = 0;
   char* mv = malloc(buff_s);
   if (mv == NULL)
      return NULL;
   untracked_buff = mv;

   struct dirent** files;
   int file_count = scandir(".", &files, &status_filter, &status_compar);

   int ent = 0;
   if (files[ent]->d_type == DT_DIR)
   {
      snprintf(untracked_buff, buff_s, "Untracked Directories:\n");
      snprintf(untracked_buff, buff_s, "  (NOTE: lit doesn't support nesting of files)\n");
   }
   for (; ent < file_count && files[ent]->d_type == DT_DIR; ent++)
   {
      snprintf(untracked_buff, buff_s, "    /%s\n", files[ent]->d_name);
      free(files[ent]);

      if (dir_count)
         (*dir_count)++;
   }

   FILE* fp = freopen(".lit/status.lit", "w", stdout);
   if (fp == NULL)
      return NULL;

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

      if (reg_count)
         (*reg_count)++;
   }
   const FILE* stdfp = freopen("/dev/tty", "w", stdout);
   if (stdfp == NULL)
      return NULL;

   for (; ent < file_count; ent++)
   {
      free(files[ent]);
   }
   free(files);

   if (dir_count)
      *dir_count = dc;
   if (reg_count)
      *reg_count = rc;
   buff_size = buff_s;
   untracked_buff = ;
   return fp;
}
int main(int argc, const char* argv[])
{ 
   if (argc < 2)
      print_usage();

   if (strcmp(argv[1], "init") == 0)
   {
      mkdir(".lit/", 777);
      mkdir(".lit/objects/", 777);
      mkdir(".lit/refs/", 777);
   }
   else if (strcmp(argv[1], "status") == 0)
   {
      int dir_count;
      int reg_count;
      FILE* fp = determine_objects(&dir_count, &reg_count, );
      if (fp == NULL)
         return -1;

      fp = fopen(".lit/status.lit", "r");

      char*  pardir  = malloc(512);
      char*  hash    = malloc(65);
      char*  ref     = malloc(128);

      size_t track_s = 512;
      char*  track   = malloc(track_s);
      
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
         int wouldve = snprintf(track, track_s, "  %s\n", ref);
         if (wouldve > track_s)
         {
            track_s = wouldve * 2;
            char* t = realloc(track, track_s);
            if (t == NULL)
               return -1;

            track = t;
         }
      }
      if (ferror(fp))
         perror("Error while reading status.lit");

      //printf("%d:%d:%d\n", dir_count, reg_count, tracked_count);
      if (reg_count)
      {
         if (tracked_count)
         {
            if (dir_count)
               printf("\n");

            printf("Tracked Files:\n");
            printf("%s", track);
         }
      }

      fclose(fp);

      free(pardir);
      free(hash);
      free(ref);
      free(track);
   }
}
