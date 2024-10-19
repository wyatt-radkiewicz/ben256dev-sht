#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

typedef char sht_pardir_buff   [512];
typedef char sht_ref_buff      [128];
typedef char sht_hash_buff     [65];

void sht_print_usage()
{
   printf("usage: sht [<command>]\n");
}
int sht_check_tree()
{
   if (access(".sht", F_OK))
   {
      return 1;
   }
   if (access(".sht/objects", F_OK))
   {
      return 2;
   }
   if (access(".sht/refs", F_OK))
   {
      return 3;
   }

   return 0;
}
int sht_check_complain()
{
   int cl;
   if ((cl = sht_check_tree()))
   {
      switch (cl)
      {
         case 1:
            printf("Not a sht repository\n");
            printf("  (run \"sht init\" to initialize repository)\n");
            break;
         case 2:
            fprintf(stderr, "Error: directory \".sht/objects\" not found\n");
            break;
         case 3:
            fprintf(stderr, "Error: directory \".sht/refs\" not found\n");
            break;
      }
      switch (cl)
      {
         case 2:
         case 3:
            printf("  (run \"sht init\" to fix these files)\n");
      }

      return 0;
   }

   return cl;
}
int sht_reg_filter(const struct dirent* ent)
{
   return ent->d_type == DT_REG;
}
int sht_status_filter(const struct dirent* ent)
{
   return ent->d_type == DT_REG || ent->d_type == DT_DIR;
}
int sht_status_compar(const struct dirent** a, const struct dirent** b)
{
   return ((*a)->d_type - (*b)->d_type);
}
int sht_get_ref(sht_ref_buff ref, sht_hash_buff* hash_out)
{
   if (ref == NULL)
      return -1;

   sht_pardir_buff pardir;

   int wb = snprintf(pardir, 512, ".sht/refs/%.128s", ref);
   if (wb >= 512)
   {
      fprintf(stderr, "Error: snprintf write exceeds buffer bounds\n");
      return -1;
   }

   if (access(pardir, F_OK))
      return -1;

   FILE* ref_file = fopen(pardir, "r");
   if (ref_file == NULL)
   {
      perror("Error: failed to open ref file");
      return -1;
   }

   sht_hash_buff hash;
   int frv = fscanf(ref_file, "%64s", hash);

   fclose(ref_file);

   if (frv == EOF)
      return EOF;
   if (frv != 1)
      return -1;

   if (hash_out)
      strcpy(*hash_out, hash);

   return 0;
}
int sht_get_hash(sht_hash_buff hash, sht_pardir_buff* pardir_out)
{
   if (hash == NULL)
      return -1;

   sht_pardir_buff pardir;

   int wb;
   wb = snprintf(pardir, 512, ".sht/objects/%.2s/", hash);
   if (wb >= 512)
   {
      fprintf(stderr, "Error: snprintf write exceeds buffer bounds\n");
      return -1;
   }
   if (access(pardir, F_OK))
      return 1;
   
   wb = snprintf(pardir, 512, ".sht/objects/%.2s/%.62s", hash, hash+2);
   if (wb >= 512)
   {
      fprintf(stderr, "Error: snprintf write exceeds buffer bounds\n");
      return -1;
   }
   if (access(pardir, F_OK))
      return 2;

   if (pardir_out)
      strcpy(*pardir_out, pardir);

   return 0;
}
FILE* sht_determine_objects(int* dir_count_ptr, int* reg_count_ptr)
{
   int dir_count = 0;
   int reg_count = 0;

   FILE* status_directories_file = fopen(".sht/status-directories.sht", "w");
   if (status_directories_file == NULL)
      goto DETERMINE_OBJECTS_RET_NULL;

   struct dirent** files;
   int file_count = scandir(".", &files, &sht_status_filter, &sht_status_compar);

   if (files[0]->d_type == DT_DIR)
   {
      fprintf(status_directories_file, "Untracked Directories:\n");
      fprintf(status_directories_file, "  (NOTE: sht doesn't support nesting of files)\n");
   }

   int ent = 0;
   for (; ent < file_count && files[ent]->d_type == DT_DIR; ent++)
   {
      fprintf(status_directories_file, "    /%s\n", files[ent]->d_name);
      free(files[ent]);
      dir_count++;
   }

   const FILE* status_file = freopen(".sht/status.sht", "w", stdout);
   if (status_file == NULL)
      goto DETERMINE_OBJECTS_RET_NULL;

   for (; ent < file_count && files[ent]->d_type == DT_REG; ent++)
   {
      size_t command_n = strlen(files[ent]->d_name) + 7;
      char* command_s = malloc(command_n);
      if (command_s == NULL)
         exit(EXIT_FAILURE);

      int wb = snprintf(command_s, command_n, "b3sum %s", files[ent]->d_name);
      if (wb >= command_n)
      {
         fprintf(stderr, "Error: snprintf write exceeds buffer bounds\n");
         return NULL;
      }
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

   if (dir_count_ptr)
      *dir_count_ptr      = dir_count;
   if (reg_count_ptr)
      *reg_count_ptr      = reg_count;

   return status_directories_file;

DETERMINE_OBJECTS_RET_NULL:
   return NULL;
}

int sht_check(const char* argv[], char** ref_ptr, char** hash_ptr)
{
   const char* status_file_path;

   sht_pardir_buff pardir;
   sht_ref_buff    ref;
   sht_hash_buff   hash;

   /*
      fprintf(stderr, "unsupported flag \"%s\"\n", argv[3]);
      goto SHT_CHECK_RET_ERROR;
   */

   if (strcmp("--status", argv[3]) == 0)
   {
      FILE* rv = sht_determine_objects(0, 0);
      if (rv == NULL)
      {
         goto SHT_CHECK_RET_ERROR;
      }
      fclose(rv);

      status_file_path = ".sht/status.sht";
   }
   else
   {
      struct stat status_file_stats;
      int sv = stat(argv[3], &status_file_stats);

      if (sv != 0)
      {
         fprintf(stderr, "Error: could not stat provided status file \"%s\"\n", argv[3]);
         goto SHT_CHECK_RET_ERROR;
      }
         
      if (!S_ISREG(status_file_stats.st_mode))
      {
         fprintf(stderr, "Error: provided status file \"%s\" should be regular\n", argv[3]);
         goto SHT_CHECK_RET_ERROR;
      }

      status_file_path = argv[3];
   }

   FILE* status_file = fopen(status_file_path, "r");
   if (status_file == NULL)
   {
      perror("Error: failed to open status file");
      goto SHT_CHECK_RET_ERROR;
   }

   int found_file = 0;
   for (; fscanf(status_file, "%64s %127s", hash, ref) != EOF; )
   {
      if (strcmp(ref, argv[2]) == 0)
      {
         found_file = 1;
         break;
      }
   }

   if (found_file == 0)
   {
      struct stat check_file_stats;
      int sv = stat(argv[2], &check_file_stats);

      if (sv != 0)
      {
         fprintf(stderr, "Error: file %s was neither found in an existing status nor is it present in your directory\n", argv[2]);
         fprintf(stderr, "  Are you trying to check a file which was presently detected by status?\n");
         fprintf(stderr, "  NOTE: running --status overwrites previous detections by status for currently non-existing files\n");

         goto SHT_CHECK_RET_ERROR;
      }

      fprintf(stderr, "Error: could not find file \"%s\" in status\n", argv[2]);
      fprintf(stderr, "  (try running %s %s %s --status or run %s status first)\n", argv[0], argv[1], argv[2], argv[0]);
      goto SHT_CHECK_RET_ERROR;
   }

   int file_track_status = 0;

   int wb;
   wb = snprintf(pardir, 512, ".sht/objects/%.2s/", hash);
   if (wb >= 512)
   {
      fprintf(stderr, "Error: snprintf write exceeds buffer bounds\n");
      goto SHT_CHECK_RET_ERROR;
   }

   if (!access(pardir, F_OK))
   {
      wb = snprintf(pardir, 512, ".sht/objects/%.2s/%.62s", hash, hash+2);
      if (wb >= 512)
      {
         fprintf(stderr, "Error: snprintf write exceeds buffer bounds\n");
         goto SHT_CHECK_RET_ERROR;
      }

      if (!access(pardir, F_OK))
         file_track_status |= 1;
   }

   wb = snprintf(pardir, 512, ".sht/refs/%.128s", ref);
   if (wb >= 512)
   {
      fprintf(stderr, "Error: snprintf write exceeds buffer bounds\n");
      goto SHT_CHECK_RET_ERROR;
   }

   if (!access(pardir, F_OK))
      file_track_status |= 2;

   if (hash_ptr)
      (*hash_ptr) = hash;
   if (ref_ptr)
      (*ref_ptr) = ref;

   return file_track_status;

SHT_CHECK_RET_ERROR:
   if (hash_ptr)
      (*hash_ptr) = hash;
   if (ref_ptr)
      (*ref_ptr) = ref;
   
   return -1;
}

int main(int argc, const char* argv[])
{ 
   if (argc < 2)
   {
      sht_print_usage();
      return -1;
   }

   if (strcmp(argv[1], "init") == 0)
   {
      int cl = sht_check_tree();
      switch (cl)
      {
         case 1:
            if (mkdir(".sht/", 0777))
            {
               perror("Error: failed to make sht directory");
               return -1;
            }
            if (mkdir(".sht/objects/", 0777))
            {
               perror("Error: failed to make sht objects directory");
               return -1;
            }
            if (mkdir(".sht/refs/", 0777))
            {
               perror("Error: failed to make sht refs directory");
               return -1;
            }
            printf("sht repository created\n");
            break;
         case 2:
            if (mkdir(".sht/objects/", 0777))
            {
               perror("Error: failed to make sht refs directory");
               return -1;
            }
            break;
         case 3:
            if (mkdir(".sht/refs/", 0777))
            {
               perror("Error: failed to make sht refs directory");
               return -1;
            }
            break;
         default:
            printf("Already a sht repository\n");
      }
      switch (cl)
      {
         case 2:
         case 3:
            printf("Missing sht files re-initialzed\n");
      }

      return 0;
   }
   else if (strcmp(argv[1], "status") == 0)
   {
      sht_check_complain();

      int dir_count;
      int reg_count;
      FILE* rv = sht_determine_objects(&dir_count, &reg_count);
      if (rv == NULL)
      {
         return -1;
      }
      fclose(rv);

      FILE* fp = fopen(".sht/status.sht", "r");
      if (fp == NULL)
      {
         perror("Error: failed to open status file");
         return -1;
      }

      sht_pardir_buff pardir;
      sht_ref_buff    ref;
      sht_hash_buff   hash;

      FILE* status_tracked_file = fopen(".sht/status-tracked.sht", "w");
      if (status_tracked_file == NULL)
      {
         perror("Error: failed to open status_tracked_file");
         goto STATUS_FREE_BUFFERS;
      }
      FILE* status_untracked_file = fopen(".sht/status-untracked.sht", "w");
      if (status_untracked_file == NULL)
      {
         perror("Error: failed to open status_untracked_file");
         goto STATUS_FREE_BUFFERS;
      }

      int tracked_count = 0;
      for (; fscanf(fp, "%64s %127s", hash, ref) != EOF; )
      {
         int wb;
         wb = snprintf(pardir, 512, ".sht/objects/%.2s/", hash);
         if (wb >= 512)
         {
            fprintf(stderr, "Error: snprintf write exceeds buffer bounds\n");
            return -1;
         }
         if (access(pardir, F_OK))
            goto STATUS_FILE_IS_UNTRACKED;

         wb = snprintf(pardir, 512, ".sht/objects/%.2s/%.62s", hash, hash+2);
         if (wb >= 512)
         {
            fprintf(stderr, "Error: snprintf write exceeds buffer bounds\n");
            return -1;
         }
         if (access(pardir, F_OK))
            goto STATUS_FILE_IS_UNTRACKED;

         tracked_count++;
         fprintf(status_tracked_file, "  %s\n", ref);
         continue;

STATUS_FILE_IS_UNTRACKED:
         fprintf(status_untracked_file, "  %s\n", ref);
      }
      if (ferror(fp))
         perror("Error while reading status.sht");

      fclose(fp);
      fclose(status_tracked_file);
      fclose(status_untracked_file);

      if (reg_count)
      {
         if (dir_count)
         {
            FILE* status_directories_file = fopen(".sht/status-directories.sht", "r");
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
            status_tracked_file = fopen(".sht/status-tracked.sht", "r");
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
            status_untracked_file = fopen(".sht/status-untracked.sht", "r");
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
      return 0;
   }
   else if (strcmp(argv[1], "store") == 0)
   {
      sht_check_complain();

      if (argc < 3)
      {
         printf("Nothing specified for \"%s store\"\n", argv[0]);
         printf("  try running \"%s store [FILE] ...\" to track files\n", argv[0]);
      }

      FILE* rv = sht_determine_objects(0, 0);
      if (rv == NULL)
      {
         return -1;
      }
      fclose(rv);

      sht_pardir_buff pardir;
      sht_ref_buff    ref;
      sht_hash_buff   hash;

      FILE* status_file = fopen(".sht/status.sht", "r");
      if (status_file == NULL)
      {
         perror("Error: failed to open status file");
         return -1;
      }

      const int store_argc = argc - 2;
      char** args_need_match = calloc(store_argc, sizeof(char*));
      if (args_need_match == NULL)
      {
         perror("Error: failed to malloc args_need_match");
         return -1;
      }
      for (int i = 0; i < store_argc; i++)
      {
         args_need_match[i] = malloc(strlen(argv[i + 2]) + 1);
         if (args_need_match[i] == NULL)
         {
            perror("Error: failed to malloc args_need_match");
            goto SHT_WIPE_RET_ERR;
         }
         if (strncmp(argv[i + 2], "--", 2) == 0 || strncmp(argv[i], "-", 2) == 0)
         {
            printf("Warning: skipping argument \"%s\"\n", argv[i + 2]);
            printf("  (NOTE: \"%s %s\" does not support flags)\n", argv[0], argv[1]);
            args_need_match[i] = NULL;
            continue;
         }
         strcpy(args_need_match[i], argv[i + 2]);
      }

      FILE* stored_file = fopen(".sht/store-track.sht", "w");
      if (stored_file == NULL)
      {
         perror("Error: failed to open store file");
         return -1;
      }

      for (; fscanf(status_file, "%64s %127s", hash, ref) != EOF; )
      {
         for (int i = 0; i < store_argc; i++)
         {
            if (args_need_match[i] == NULL)
               continue;
            if (strcmp(ref, args_need_match[i]) == 0)
            {
               int wb;
               wb = snprintf(pardir, 512, ".sht/objects/%.2s/", hash);
               if (wb >= 512)
               {
                  fprintf(stderr, "Error: snprintf write exceeds buffer bounds\n");
                  return -1;
               }

               struct stat statbuff;
               if (stat(pardir, &statbuff))
               {
                  if (mkdir(pardir, 0777))
                  {
                     perror("Error: failed to make object parent directory");
                     return -1;
                  }
               }

               wb = snprintf(pardir, 512, ".sht/objects/%.2s/%.62s", hash, hash+2);
               if (wb >= 512)
               {
                  fprintf(stderr, "Error: snprintf write exceeds buffer bounds\n");
                  return -1;
               }

               FILE* file_object = fopen(pardir, "w");
               if (file_object == NULL)
               {
                  perror("Error: failed to open object file for write");
                  return -1;
               }
               FILE* file_actual = fopen(ref, "r");
               if (file_actual == NULL)
               {
                  perror("Error: failed to open actual file for read");
                  return -1;
               }

               for (int ch; (ch = fgetc(file_actual)) != EOF; )
                  fputc(ch, file_object);

               fclose(file_object);
               fclose(file_actual);

               wb = snprintf(pardir, 512, ".sht/refs/%s", ref);
               if (wb >= 512)
               {
                  fprintf(stderr, "Error: snprintf write exceeds buffer bounds\n");
                  return -1;
               }

               FILE* file_ref = fopen(pardir, "w");
               if (file_ref == NULL)
               {
                  perror("Error: failed to open ref file for read");
                  return -1;
               }
               fprintf(file_ref, "%s\n", hash);

               fprintf(stored_file, "%s\n", args_need_match[i]);
               args_need_match[i] = NULL;
            }
         }
      }

      for (int i = 0; i < store_argc; i++)
      {
         if (args_need_match[i] == NULL)
         {
            free(args_need_match[i]);
            continue;
         }
         
         fprintf(stderr, "Error: \"%s\" does not match any status ref\n", args_need_match[i]);
      }
      free(args_need_match);

      return 0;
   }
   else if (strcmp(argv[1], "check") == 0)
   {
      if (argc != 4)
      {
         printf("usage: %s %s [REF_FILE] [STATUS_FILE | --status]\n", argv[0], argv[1]);
         return 0;
      }

      char* hash;
      char* ref;
      switch (sht_check(argv, &ref, &hash))
      {
         case -1:
            return -1;
            break;
         case 0:
            printf("file \"%s\" contents are untracked and the name \"%s\" is unused\n", argv[2], ref);
            break;
         case 1:
            printf("file \"%s\" contents are already tracked under different name \"%s\"\n", argv[2], ref);
            break;
         case 2:
            printf("file \"%s\" contents are untracked but the name \"%s\" is already in use for object %.7s\n", argv[2], ref, hash);
            break;
         case 3:
            printf("file \"%s\" contents are already tracked under same name \"%s\"\n", argv[2], ref);
            break;
      }

      return 0;
   }
   else if (strcmp(argv[1], "check-tree") == 0)
   {
      if (argc != 2)
      {
         printf("usage: %s %s\n", argv[1], argv[2]);
         return 0;
      }

      sht_check_complain();
      return 0;
   }
   else if (strcmp(argv[1], "wipe") == 0)
   {
      sht_check_complain();

      if (argc < 3)
      {
         printf("Nothing specified for \"%s wipe\"\n", argv[0]);
         printf("  try running \"%s wipe [FILE] ...\" to erase files\n", argv[0]);
      }

      FILE* rv = sht_determine_objects(0, 0);
      if (rv == NULL)
      {
         return -1;
      }
      fclose(rv);

      const int wipe_argc = argc - 2;
      char** args_need_match = calloc(wipe_argc, sizeof(char*));
      if (args_need_match == NULL)
      {
         perror("Error: failed to malloc args_need_match");
         goto SHT_WIPE_RET_ERR;
      }
      for (int i = 0; i < wipe_argc; i++)
      {
         args_need_match[i] = malloc(strlen(argv[i + 2]) + 1);
         if (args_need_match[i] == NULL)
         {
            perror("Error: failed to malloc args_need_match");
            goto SHT_WIPE_RET_ERR;
         }

         if (strncmp(argv[i + 2], "--", 2) == 0 || strncmp(argv[i], "-", 2) == 0)
         {
            printf("Warning: skipping argument \"%s\"\n", argv[i + 2]);
            printf("  (NOTE: \"%s %s\" does not support flags)\n", argv[0], argv[1]);
            args_need_match[i] = NULL;
            continue;
         }
         strcpy(args_need_match[i], argv[i + 2]);
      }

      sht_pardir_buff pardir;
      sht_ref_buff    ref;
      sht_hash_buff   hash;

      for (int i = 0; i < wipe_argc; i++)
      {
         if (args_need_match[i] == NULL)
            continue;

         strcpy(ref, args_need_match[i]);

         if (sht_get_ref(ref, &hash))
         {
            fprintf(stderr, "Error: ref file \"%s\" not found\n", ref);
            goto SHT_WIPE_RET_ERR;
         }
         args_need_match[i] = NULL;

         int gh = sht_get_hash(hash, &pardir);
         if (gh)
         {
            if (gh == 1)
               fprintf(stderr, "Error: file \"%s\" ref found but object parent dir %.2s/ unaccessable\n", ref, hash);
            else if (gh == 2)
               fprintf(stderr, "Error: file \"%s\" ref found but object file does not exist\n", ref);

            goto SHT_WIPE_RET_ERR;
         }

         if (remove(pardir))
         {
            fprintf(stderr, "Error: failed to remove object file \"%s\" targeted for wipe: ", pardir);
            perror("");
            goto SHT_WIPE_RET_ERR;
         }

         pardir[16] = '\0';
         DIR* dfd = opendir(pardir);
         if (dfd == NULL)
         {
            perror("Error: failed to open pardir to check for sibliings");
            goto SHT_WIPE_RET_ERR;
         }

         int par_dir_is_empty = 1;
         for (const struct dirent* entry; (entry = readdir(dfd)) != NULL; )
         {
            if (entry->d_name[0] != '.')
            {
               par_dir_is_empty = 0;
               break;
            }
         }
         closedir(dfd);

         if (par_dir_is_empty)
         {
            if (remove(pardir))
            {
               fprintf(stderr, "Warning: failed to remove object parent directory \"%s\" targeted for wipe: ", pardir);
               perror("");
            }
         }

         int wb = snprintf(pardir, 512, ".sht/refs/%.128s", ref);
         if (wb >= 512)
         {
            fprintf(stderr, "Error: snprintf write exceeds buffer bounds\n");
            goto SHT_WIPE_RET_ERR;
         }
         if (access(pardir, F_OK))
         {
            fprintf(stderr, "Error: file \"%s\" ref found but tag not found inside refs\n", ref);
            goto SHT_WIPE_RET_ERR;
         }

         if (remove(pardir))
         {
            fprintf(stderr, "Error: failed to remove ref file \"%s\" targeted for wipe: ", pardir);
            perror("");
            goto SHT_WIPE_RET_ERR;
         }
      }

      for (int i = 0; i < wipe_argc; i++)
      {
         if (args_need_match[i] == NULL)
         {
            free(args_need_match[i]);
            continue;
         }
      }
      free(args_need_match);
      return 0;

SHT_WIPE_RET_ERR:
      for (int i = 0; i < wipe_argc; i++)
      {
         if (args_need_match[i] == NULL)
         {
            free(args_need_match[i]);
            continue;
         }
      }
      free(args_need_match);
      return -1;
   }
   else if (strcmp(argv[1], "normalize-files") == 0)
   {
      sht_check_complain();

      FILE* to_norm_file = fopen(".sht/to-normalize.sht", "w");
      if (to_norm_file == NULL)
      {
         perror("Error: failed to open file for writing what needs normalized");
         return -1;
      }

      struct dirent** files;
      int file_count = scandir(".", &files, &sht_reg_filter, &sht_status_compar);
      
      int ent = 0;
      for (; ent < file_count; ent++)
      {
         int ent_sucks = 0;
         for (char* c = files[ent]->d_name; (*c) != '\0'; c++)
         {
            if (files[ent]->d_type == DT_REG && (*c) == ' ')
            {
               if (ent_sucks == 0)
               {
                  int b_printed = fprintf(to_norm_file, "%s'", files[ent]->d_name);
                  ent_sucks = 1;
               }

               (*c) = '_';
            }
         }

         free(files[ent]);

         if (ent_sucks)
            fprintf(to_norm_file, "%s\n", files[ent]->d_name);
      }
      free(files);
      fclose(to_norm_file);

      to_norm_file = fopen(".sht/to-normalize.sht", "r");
      if (to_norm_file == NULL)
      {
         perror("Error: failed to open file for verification of what needs normalized");
         return -1;
      }

      char* line_buff;
      size_t line_len = 0;
      for (ssize_t nread; (nread = getline(&line_buff, &line_len, to_norm_file)) != -1; )
      {
         char* line_buff_delim_byte = strrchr(line_buff, '\'');
         if (line_buff_delim_byte == NULL)
         {
            fprintf(stderr, "Error: could not read filename to be normalized from file\n");
            goto NORMALIZE_RET_ERROR;
         }

         (*line_buff_delim_byte) = '\0';
         line_buff[nread - 1] = '\0';

         printf("Normalize file '%s' to %s?\n", line_buff, line_buff_delim_byte + 1);
         printf("   [Y/n]: ");
         char v[4];
         if (fgets(v, 4, stdin) == NULL)
         {
            fprintf(stderr, "Error: failed to read validation from user\n");
            goto NORMALIZE_RET_ERROR;
         }

         if (v[0] == 'y' || v[0] == 'Y')
         {
            if (rename(line_buff, line_buff_delim_byte + 1))
            {
               fprintf(stderr, "Error: failed to rename file \"%s\" to \"%s\" for normalization\n", line_buff, line_buff_delim_byte + 1);
               perror("   ");
               goto NORMALIZE_RET_ERROR;
            }
         }
      }

      free(line_buff);
      return 0;

NORMALIZE_RET_ERROR:
      free(line_buff);
      return -1;
   }
   else
   {
      printf("%s: unrecognized command \"%s\"\n", argv[0], argv[1]);
      printf("   Try running \"%s\" -h for help\n", argv[0]);
      printf("   JK it doesn't exist yet\n");

      return 0;
   }
}
