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
FILE* hash_objects()
{
   check_lit();

   struct dirent** files;
   int file_count = scandir(".", &files, &status_filter, &status_compar);

   int i = 0;
   if (files[i]->d_type == DT_DIR)
   {
      printf("Untracked Directories:\n");
      printf("  (NOTE: lit doesn't support nesting of files)\n");
   }
   for (; i < file_count && files[i]->d_type == DT_DIR; i++)
   {
      printf("    /%s\n", files[i]->d_name);
      free(files[i]);
   }
   if (i > 0)
      printf("\n");

   FILE* fp = freopen(".lit/status.lit", "w", stdout);
   if (fp == NULL)
      return NULL;

   for (; i < file_count && files[i]->d_type == DT_REG; i++)
   {
      size_t command_n = strlen(files[i]->d_name) + 7;
      char* command_s = malloc(command_n);
      if (command_s == NULL)
         exit(EXIT_FAILURE);

      snprintf(command_s, command_n, "b3sum %s", files[i]->d_name);
      int rv = system(command_s);

      free(files[i]);
   }
   freopen("/dev/tty", "w", stdout);
   for (; i < file_count; i++)
   {
      free(files[i]);
   }
   free(files);

   return fp;
}
int main(int argc, char* argv[])
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
      FILE* fp = hash_objects();
      if (fp == NULL)
         return -1;

      fp = fopen(".lit/status.lit", "r");

      char*  pardir  = malloc(512);
      char*  hash    = malloc(65);
      char*  ref     = malloc(128);

      size_t track_s = 512;
      char*  track   = malloc(track_s);
      
      for (; fscanf(fp, "%64s %128s", hash, ref) != EOF; )
      {
         snprintf(pardir, 512, ".lit/objects/%.2s/", hash);
         if (access(pardir, F_OK))
            continue;

         snprintf(pardir, 512, ".lit/objects/%.2s/%.62s/", hash, hash+2);
         if (access(pardir, F_OK))
            continue;

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

      printf("Tracked Files:\n");
      printf("%s", track);

      fclose(fp);

      free(pardir);
      free(hash);
      free(ref);
      free(track);
   }
}
