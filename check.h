#pragma once

#include "hash.h"
#include "sht.h"
#include "format.h"

int sht_check_tree()
{
   if (access(".sht", F_OK))
      return 1;
   if (access(".sht/objects", F_OK))
      return 2;
   if (access(".sht/tags", F_OK))
      return 3;

   return 0;
}
int sht_check_complain()
{
   int cl = sht_check_tree();
   switch (cl)
   {
      case 1:
         printf("Not a sht repository\n");
         printf("%*s(run \"%ssht init%s\" to initialize repository)\n", SHT_IND_LEVEL, "", SHT_YELLOW_RECCOMEND, SHT_RESET);
         break;
      case 2:
         fprintf(stderr, "Error: directory \".sht/objects\" not found\n");
         break;
      case 3:
         fprintf(stderr, "Error: directory \".sht/tags\" not found\n");
         break;
   }
   switch (cl)
   {
      case 2:
      case 3:
         printf("%*s(run \"%ssht init%s\" to fix these files)\n", SHT_IND_LEVEL, "", SHT_YELLOW_RECCOMEND, SHT_RESET);
   }

   return cl;
}
int sht_get_tag(sht_tag_buff tag, sht_hash_buff* hash_out)
{
   if (tag == NULL)
      return -1;

   sht_pardir_buff pardir;

   int wb = snprintf(pardir, 512, ".sht/tags/%.128s", tag);
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
int sht_determine_sucks(int* sucks_count_ptr)
{
   FILE* to_norm_file = fopen(".sht/sucks.sht", "w");
   if (to_norm_file == NULL)
   {
      perror("Error: failed to open sucks file");
      return -1;
   }

   struct dirent** files;
   int file_count = scandir(".", &files, &sht_reg_filter, &sht_status_compar);
   
   int sucks_count = 0;
   int ent = 0;
   for (; ent < file_count; ent++)
   {
      int ent_sucks = 0;
      int i = 0;
      int dot_i = 0;
      char* c = files[ent]->d_name;
      for (; c[i] != '\0'; i++)
      {
         if (files[ent]->d_type == DT_REG)
         {
            switch (c[i])
            {
               case ' ':
                  if (ent_sucks == 0)
                  {
                     fprintf(to_norm_file, "%s'", files[ent]->d_name);
                     ent_sucks = 1;
                     sucks_count++;
                  }

                  c[i] = '_';
                  break;
               case '.':
                  dot_i = i;
                  break;
            }
            if (i > 128 && ent_sucks == 0)
            {
               fprintf(to_norm_file, "%s'", files[ent]->d_name);
               ent_sucks = 1;
               sucks_count++;
            }
         }
      }

      free(files[ent]);

      if (ent_sucks)
      {
         if (dot_i > 0)
         {
            int extension_s = i - dot_i;
            int write_truncated_name_s = 128 - extension_s;
            fwrite(files[ent]->d_name, 1, write_truncated_name_s, to_norm_file);
            fwrite(&files[ent]->d_name[dot_i], 1, extension_s, to_norm_file);
         }
         else
         {
            fwrite(files[ent]->d_name, 1, 128, to_norm_file);
         }
         fprintf(to_norm_file, "\n");
      }
   }
   free(files);
   fclose(to_norm_file);

   if (sucks_count_ptr)
      (*sucks_count_ptr) = sucks_count;

   return sucks_count;
}

typedef int (*sht_hash_fun_ptr)(const char* filename);

FILE* sht_determine_objects(int* dir_count_ptr, int* reg_count_ptr, int opt_flag)
{
   int dir_count = 0;
   int reg_count = 0;

   struct dirent** files;
   int file_count = scandir(".", &files, &sht_status_filter, &sht_status_compar);

   FILE* status_directories_file = fopen(".sht/status-directories.sht", "w");
   if (status_directories_file == NULL)
      goto DETERMINE_OBJECTS_RET_NULL;

   int ent = 0;
   if (files[0]->d_type == DT_DIR && opt_flag & SHT_DETERMINE_OBJECTS_API_NOTIFY)
   {
      fprintf(status_directories_file, "Untracked Directories:\n");
      fprintf(status_directories_file, "%*s(NOTE: sht does not maintain a tree structure for directories)\n", SHT_IND_LEVEL, "");
      fprintf(status_directories_file, "%*s%s(NOTE: use \"--recursive\" to track contents of directories recursively)%s\n", SHT_IND_LEVEL, "",
                                    SHT_STRIKETHROUGH,                                                     SHT_STRIKETHROUGH_END);
   }
   fprintf(status_directories_file, "%s", SHT_DARK);
   for (; ent < file_count && files[ent]->d_type == DT_DIR; ent++)
   {
      fprintf(status_directories_file, "%*s%s/\n", SHT_IND_ITEM, "", files[ent]->d_name);
      free(files[ent]);
      dir_count++;
   }
   fprintf(status_directories_file, "%s", SHT_RESET);

   FILE* status_file = freopen(".sht/status.sht", "w", stdout);
   if (status_file == NULL)
      goto DETERMINE_OBJECTS_RET_NULL;

   int use_c_blake_imp = 0;
   for (; ent < file_count && files[ent]->d_type == DT_REG; ent++)
   {
      if (use_c_blake_imp)
      {
         if (sht_hash(files[ent]->d_name))
         {
            fprintf(stderr, "Error: failed to hash file \"%s\"\n", files[ent]->d_name);
            exit(EXIT_FAILURE);
         }
      }
      else
      {
         int rv = sht_b3sum_hash(files[ent]->d_name);
         fflush(status_file);
         if (rv)
         {
            if (rv == -1)
            {
               fprintf(stderr, "Error: failed to hash file \"%s\"\n", files[ent]->d_name);
               exit(EXIT_FAILURE);
            }
            else
            {
               fprintf(stderr, "b3sum utility not found. proceeding with slower C implementation...\n");
               use_c_blake_imp = 1;
               ent--;
            }
         }
      }

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

int sht_check(const char* argv[], char** tag_ptr, char** hash_ptr)
{
   const char* status_file_path;

   sht_pardir_buff pardir;
   sht_tag_buff    tag;
   sht_hash_buff   hash;

   if (strcmp("--status", argv[3]) == 0)
   {
      FILE* rv = sht_determine_objects(0, 0, 0);
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
   for (; fscanf(status_file, "%64s %127s", hash, tag) != EOF; )
   {
      if (strcmp(tag, argv[2]) == 0)
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
         fprintf(stderr, "%*sAre you trying to check a file which was presently detected by status?\n", SHT_IND_ITEM, "");
         fprintf(stderr, "%*sNOTE: running --status overwrites previous detections by status for currently non-existing files\n", SHT_IND_ITEM, "");

         goto SHT_CHECK_RET_ERROR;
      }

      fprintf(stderr, "Error: could not find file \"%s\" in status\n", argv[2]);
      fprintf(stderr, "%*s(try running %s %s %s --status or run %s status first)\n", SHT_IND_ITEM, "", argv[0], argv[1], argv[2], argv[0]);
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

   wb = snprintf(pardir, 512, ".sht/tags/%.128s", tag);
   if (wb >= 512)
   {
      fprintf(stderr, "Error: snprintf write exceeds buffer bounds\n");
      goto SHT_CHECK_RET_ERROR;
   }

   if (!access(pardir, F_OK))
      file_track_status |= 2;

   if (hash_ptr)
      (*hash_ptr) = hash;
   if (tag_ptr)
      (*tag_ptr) = tag;

   return file_track_status;

SHT_CHECK_RET_ERROR:
   if (hash_ptr)
      (*hash_ptr) = hash;
   if (tag_ptr)
      (*tag_ptr) = tag;
   
   return -1;
}
