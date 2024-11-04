% SHT_PARSE_ERROR(3) Version 1.0 | Library calls

NAME
====

**sht_parse_error** — handles parsing error reporting with formatted output.

SYNOPSIS
========

```c
int sht_parse_error(const char* parsed,
                    int i, const char* i_name,
                    const char* format,
                    const char* keyword, size_t keyword_n);
```

DESCRIPTION
===========

The function **sht_parse_error** takes a *parsed* string and formats an error message pointing to the *i*th character in the string. The problematic character is labeled with the *i_name* string. An error message may be provided with a special **%k** format specifier. This specifier is replaced (if it exists) with the *keyword* string, size delimited by *keyword_n*.

RETURN VALUE
============

On success, **sht_parse_error()** returns number of %k format specifiers matched. If an error occurs, **-1** is returned.

EXAMPLES
========
The program below is an excerpt from the sht source code where **sht_parse_error()** is used to indicate where additional filename delimiters of a '.' are provided, and to point out disallowed characters.

Program source
--------------
```c
int dot_found = 0;
for (int i = 0; arg[i] != '\0'; i++)
{
  char c = isalnum(arg[i]) ? 'A' : arg[i];
  switch (c)
  {
     case 'A':
        break;
     case '.':
        dot_found++;
        if (dot_found > 1)
        {
           if (sht_parse_error(arg, i, "i", "additional '.' found in extension of file \"%k\"\n", arg, i) != 1)
           {
              fprintf(stderr, "Error: failed to match arguments while printing parse error\n");
              return -1;
           }
           return -1;
        }
        continue;
     default:
        if (sht_parse_error(arg, i, "i", "Note: file name must only contain alphanumeric or '_'\n", 0, 0) != 1)
        {
           fprintf(stderr, "Error: failed to match arguments while printing parse error\n");
           return -1;
        }
    }
}
```

BUGS
====

See GitHub Issues or report your own: <https://github.com/ben256dev/sht/issues>

AUTHOR
======

Benjamin Blodgett <benjamin@ben256.com>

SEE ALSO
========

**sht(1)**
