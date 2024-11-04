#pragma once

#include "sht.h"

int sht_hash(const char* filename)
{
   struct stat statbuff;
   if (stat(filename, &statbuff))
   {
      perror("Error: failed to stat file for hashing");
      return -1;
   }

   FILE* fp = fopen(filename, "r");
   if (fp == NULL)
   {
      perror("Error: failed to open file for hashing");
      return -1;
   }

   blake3_hasher hasher;
   blake3_hasher_init(&hasher);

   uint8_t* buff = malloc(65536);
   if (buff == NULL)
   {
      perror("Error: failed to allocate chunk buffer");
      fclose(fp);
      return -1;
   }

   size_t bytes_read;
   for ( ; (bytes_read = fread(buff, 1, 65536, fp)) > 0; )
      blake3_hasher_update(&hasher, buff, bytes_read);

   if (ferror(fp))
   {
      fprintf(stderr, "Error: failed to read stat'd file for hashing\n");
      free(buff);
      fclose(fp);
      return -1;
   }

   uint8_t output[BLAKE3_OUT_LEN];
   blake3_hasher_finalize(&hasher, output, BLAKE3_OUT_LEN);

   for (size_t i = 0; i < BLAKE3_OUT_LEN; i++)
      printf("%02x", output[i]);
   printf(" %.127s", filename);
   printf("\n");

   free(buff);
   fclose(fp);

   return 0;
}
#define SHT_DETERMINE_OBJECTS_API_NOTIFY 1
int sht_determine_objects_recursive(const char* dir_path_rec)
{
   int reg_count = 0;

   struct dirent** files;
   int file_count = scandir(dir_path_rec, &files, &sht_status_filter, &sht_status_compar);

   int ent = 0;
   for (; ent < file_count && files[ent]->d_type == DT_DIR; ent++)
   {
      int rc = sht_determine_objects_recursive(files[ent]->d_name);
      if (rc == -1)
         return -1;
      reg_count += rc;
      free(files[ent]);
   }
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
         return -1;
      }
      int rv = system(command_s);
      if (rv == -1)
         perror("b3sum sys call failed");

      free(files[ent]);

      reg_count++;
   }

   free(files);

   return reg_count;
}

int sht_b3sum_hash(const char* filename)
{
   static char*  command_s = NULL;
   static size_t command_n = 0;

   size_t command_n_new = strlen(filename) + 7;
   if (command_n_new > command_n)
   {
      command_n = command_n_new;
      command_s = realloc(command_s, command_n);
      if (command_s == NULL)
      {
         perror("Error: failed to alloc command buffer");
         exit(EXIT_FAILURE);
      }
   }

   if (command_s == NULL)
      return -1;

   int wb = snprintf(command_s, command_n, "b3sum %s", filename);
   if (wb >= command_n)
   {
      fprintf(stderr, "Error: snprintf write exceeds buffer bounds\n");
      return -1;
   }
   int rv = system(command_s);
   return rv;
}
