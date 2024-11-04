#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <stdint.h>

#include <blake3.h>

#include "check.h"
#include "format.h"
#include "hash.h"
#include "parse.h"
#include "sht.h"

#define SHT_VERSION "0.0.0"

int main(int argc, const char* argv[])
{ 
   if (argc < 2 || strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0)
   {
      sht_print_usage(argv[0]);
      return 0;
   }
   else if (strcmp(argv[1], "-v") == 0 || strcmp(argv[1], "--version") == 0)
   {
      puts(SHT_VERSION);
      return 0;
   }
   else if (strcmp(argv[1], "init") == 0)
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
            if (mkdir(".sht/tags/", 0777))
            {
               perror("Error: failed to make sht tags directory");
               return -1;
            }
            printf("sht repository created\n");
            break;
         case 2:
            if (mkdir(".sht/objects/", 0777))
            {
               perror("Error: failed to make sht objects directory");
               return -1;
            }
            break;
         case 3:
            if (mkdir(".sht/tags/", 0777))
            {
               perror("Error: failed to make sht tags directory");
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
      if (sht_check_complain())
         return 0;

      int suck_count;
      sht_determine_sucks(&suck_count);
      if (suck_count == -1)
      {
         fprintf(stderr, "Error: failed to determine sucky status for files\n");
         return -1;
      }

      if (suck_count)
      {
         printf("%d of your filenames suck. Refusing to perform \"%s status\"\n", suck_count, argv[0]);
         printf("%*s(Try running \"%s unsuck\" to automatically rename files one-by-one)\n", SHT_IND_LEVEL, "", argv[0]);
         printf("%*s(NOTE: run \"%s unsuck --force\" to force rename all files)\n", SHT_IND_LEVEL, "", argv[0]);
         return 0;
      }

      int dir_count;
      int reg_count;
      FILE* rv = sht_determine_objects(&dir_count, &reg_count, SHT_DETERMINE_OBJECTS_API_NOTIFY);
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
      sht_tag_buff    tag;
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
      int matched;
      for (; (matched = fscanf(fp, "%64s %127s", hash, tag)) != EOF; )
      {
         if (matched != 2)
         {
            fprintf(stderr, "Error: failed to match out of status file\n");
            goto STATUS_FREE_BUFFERS;
         }
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
         fprintf(status_tracked_file, "%*s%s\n", SHT_IND_ITEM, "", tag);
         continue;

STATUS_FILE_IS_UNTRACKED:
         fprintf(status_untracked_file, "%*s%s\n", SHT_IND_ITEM, "", tag);
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
            printf("Tracked Files:%s\n", SHT_BRIGHT_GREEN);
            status_tracked_file = fopen(".sht/status-tracked.sht", "r");
            if (status_tracked_file == NULL)
            {
               perror("Error: failed to open status_tracked_file");
               return -1;
            }
            for (char c; ( c = fgetc(status_tracked_file) ) != EOF; putchar(c));
            printf("%s", SHT_RESET);
            fclose(status_tracked_file);
         }
         if (tracked_count < reg_count)
         {
            if (dir_count + tracked_count)
               printf("\n");
            printf("Untracked Files:%s\n", SHT_ORANGE);
            status_untracked_file = fopen(".sht/status-untracked.sht", "r");
            if (status_untracked_file == NULL)
            {
               perror("Error: failed to open status_untracked_file");
               return -1;
            }
            for (char c; ( c = fgetc(status_untracked_file) ) != EOF; putchar(c));
            puts(SHT_RESET);
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
         printf("%*stry running \"%s store [FILE] ...\" to track files\n", SHT_IND_LEVEL, "", argv[0]);
      }

      FILE* rv = sht_determine_objects(0, 0, 0);
      if (rv == NULL)
      {
         return -1;
      }
      fclose(rv);

      sht_pardir_buff pardir;
      sht_tag_buff    tag;
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
            printf("%*s(NOTE: \"%s %s\" does not support flags)\n", SHT_IND_LEVEL, "", argv[0], argv[1]);
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

      for (; fscanf(status_file, "%64s %127s", hash, tag) != EOF; )
      {
         for (int i = 0; i < store_argc; i++)
         {
            if (args_need_match[i] == NULL)
               continue;
            if (strcmp(tag, args_need_match[i]) == 0)
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
               FILE* file_actual = fopen(tag, "r");
               if (file_actual == NULL)
               {
                  perror("Error: failed to open actual file for read");
                  return -1;
               }

               for (int ch; (ch = fgetc(file_actual)) != EOF; )
                  fputc(ch, file_object);

               fclose(file_object);
               fclose(file_actual);

               wb = snprintf(pardir, 512, ".sht/tags/filename:%s", tag);
               if (wb >= 512)
               {
                  fprintf(stderr, "Error: snprintf write exceeds buffer bounds\n");
                  return -1;
               }

               FILE* file_tag = fopen(pardir, "w");
               if (file_tag == NULL)
               {
                  perror("Error: failed to open tag file for read");
                  return -1;
               }
               fprintf(file_tag, "%s\n", hash);

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
      char* tag;
      switch (sht_check(argv, &tag, &hash))
      {
         case -1:
            return -1;
            break;
         case 0:
            printf("file \"%s\" contents are untracked and the name \"%s\" is unused\n", argv[2], tag);//todotag
            break;
         case 1:
            printf("file \"%s\" contents are already tracked under different name \"%s\"\n", argv[2], tag);
            break;
         case 2:
            printf("file \"%s\" contents are untracked but the name \"%s\" is already in use for object %.7s\n", argv[2], tag, hash);
            break;
         case 3:
            printf("file \"%s\" contents are already tracked under same name \"%s\"\n", argv[2], tag);
            break;
      }

      return 0;
   }
   else if (strcmp(argv[1], "tree") == 0)
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
         printf("%*stry running \"%s wipe [FILE] ...\" to erase files\n", SHT_IND_LEVEL, "", argv[0]);
         return 0;
      }

      FILE* rv = sht_determine_objects(0, 0, 0);
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
            printf("%*s(NOTE: \"%s %s\" does not support flags)\n", SHT_IND_LEVEL, "", argv[0], argv[1]);
            args_need_match[i] = NULL;
            continue;
         }
         strcpy(args_need_match[i], argv[i + 2]);
      }

      sht_pardir_buff pardir;
      sht_tag_buff    tag;
      sht_hash_buff   hash;

      for (int i = 0; i < wipe_argc; i++)
      {
         if (args_need_match[i] == NULL)
            continue;

         strcpy(tag, args_need_match[i]);

         if (sht_get_tag(tag, &hash))
         {
            fprintf(stderr, "Error: tag file \"%s\" not found\n", tag);
            goto SHT_WIPE_RET_ERR;
         }
         args_need_match[i] = NULL;

         int gh = sht_get_hash(hash, &pardir);
         if (gh)
         {
            if (gh == 1)
               fprintf(stderr, "Error: file \"%s\" tag found but object parent dir %.2s/ unaccessable\n", tag, hash);
            else if (gh == 2)
               fprintf(stderr, "Error: file \"%s\" tag found but object file does not exist\n", tag);

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

         int wb = snprintf(pardir, 512, ".sht/tags/%.128s", tag);
         if (wb >= 512)
         {
            fprintf(stderr, "Error: snprintf write exceeds buffer bounds\n");
            goto SHT_WIPE_RET_ERR;
         }
         if (access(pardir, F_OK))
         {
            fprintf(stderr, "Error: file \"%s\" tag found but tag not found inside tags\n", tag);//todotags
            goto SHT_WIPE_RET_ERR;
         }

         if (remove(pardir))
         {
            fprintf(stderr, "Error: failed to remove tag file \"%s\" targeted for wipe: ", pardir);
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
   else if (strcmp(argv[1], "unsuck") == 0)
   {
      int force_flag = 0;
      if (argc > 2)
      {
         for (int i = 2; i < argc; i++)
         {
            if (strcmp(argv[2], "-f") == 0 || strcmp(argv[2], "--force") == 0)
            {
               force_flag = 1;
               break;
            }
         }
      }

      if (sht_normalize_files(force_flag))
         fprintf(stderr, "Error: Failed to normalize files\n");
   }
   else if (strcmp(argv[1], "tag") == 0)
   {
      if (argc < 3)
      {
         printf("Nothing specified for \"%s%s tag%s\"\n", SHT_YELLOW_RECCOMEND, argv[0], SHT_RESET);
         printf("%*sTry running \"%s%s tag [tag-name | keyword:tag-name] ...%s\" to tag files\n",
            SHT_IND_LEVEL, "", SHT_YELLOW_RECCOMEND, argv[0],           SHT_RESET);
         return 0;
      }

      const char* filename_tag;
      for (int arg_i = 2; arg_i < argc; arg_i++)
      {
         sht_parse_tag(argv[arg_i], &filename_tag);
      }
      if (filename_tag)
         printf("Filename specified as \"%s\"\n", filename_tag);
   }
   else if (strcmp(argv[1], "hash") == 0)
   {
      if (argc < 3)
      {
         printf("No files specified for \"%s hash\"\n", argv[0]);
         return 0;
      }

      for (int i = 2; i < argc; i++)
         sht_hash(argv[i]);

      return 0;
   }
   else
   {
      printf("%s: unrecognized command \"%s\"\n", argv[0], argv[1]);
      printf("%*s Try running \"%s%s -h%s\" for help\n",
            SHT_IND_LEVEL, "", SHT_YELLOW_RECCOMEND, argv[0],           SHT_RESET);

      return 0;
   }
}
