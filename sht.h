#pragma once

#include "format.h"

typedef char sht_pardir_buff   [512];
typedef char sht_tag_buff      [128];
typedef char sht_hash_buff     [65];

void sht_print_usage(const char* commname)
{
   printf("usage: %s [-h | --help] [-v | --version] <command>\n", commname);
   printf("\n");
   printf("commonly used commands:\n");
   printf("%*sinit     initialize sht repository by creating .sht directory and internals\n", SHT_IND_LEVEL, "");
   printf("%*sstatus   show which files are tracked\n", SHT_IND_LEVEL, "");
   printf("%*sstore    track a file by adding a corresponding filename tag and object copy\n", SHT_IND_LEVEL, "");
   printf("%*swipe     untrack a file by removing corresponding filename tag and object copy\n", SHT_IND_LEVEL, "");
   printf("%*stag      map given tags onto specified objects with 'filename:foo' tag\n", SHT_IND_LEVEL, "");
   printf("\n");
   printf("useful plumbing      commands:\n");
   printf("%*stree     verify sht repository structure\n", SHT_IND_LEVEL, "");
   printf("%*scheck    show tracking status for specific file\n", SHT_IND_LEVEL, "");
   printf("%*sunsuck   check every provided filename for non-compliant characters and prompt for automatic rename\n", SHT_IND_LEVEL, "");
   printf("%*shash     take filename as input and output blake3 hash and filename\n", SHT_IND_LEVEL, "");
   printf("\n");
   printf("see \"man sht\" for more info\n");
}
int sht_reg_filter(const struct dirent* ent)
{
   return ent->d_type == DT_REG;
}
int sht_status_filter(const struct dirent* ent)
{
   return ent->d_type == DT_REG || ent->d_type == DT_DIR && strcmp(ent->d_name, ".") && strcmp(ent->d_name, "..");
}
int sht_status_compar(const struct dirent** a, const struct dirent** b)
{
   return ((*a)->d_type - (*b)->d_type);
}
