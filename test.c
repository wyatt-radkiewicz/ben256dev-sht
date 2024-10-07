#include <stdlib.h>
#include <stdio.h>
#include <string.h>

char* appstr(char** str_p, size_t* str_s, size_t* str_l, const char* app)
{
   if (str_p == NULL | str_s == NULL | str_l == NULL)
      return NULL;

   char*  str    = *str_p;
   size_t buff_s = *str_s;
   size_t buff_l = *str_l;

   if (str == NULL)
      str = malloc(64);

   int c = strncat(str + buff_l, app, buff_s - buff_l);
   if (c >= (buff_s - buff_l) )
   {
      buff_s = (c + buff_l) * 2;
      char* rv = realloc(str, buff_s);
      if (rv == NULL)
         return NULL;
      str = rv;
      buff_l += memcpy(str + buff_l, buff_s - buff_l, app);
   }
   else
      buff_l += c;

   (*str_s) = buff_s;
   (*str_l) = buff_l;
   return &str;
}

int main(int argc, char* argv[])
{
   size_t buff_s = 0;
   size_t buff_l = 0;
   char* buff = NULL;

   appstr(buff, &buff_s, &buff_l, "obama\n");

   fwrite(buff, 1L, buff_l, stdout);

   free(buff);

	return 0;
}
