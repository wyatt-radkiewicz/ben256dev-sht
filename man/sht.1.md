% SHT(1) Version 1.0 | Sht Manual

NAME
====

**sht** — shell tag utility for resources

SYNOPSIS
========
| ***sht*** \[**-h** | **--help** | **-v** | **--version**] <**command**> \[<**args**>]

DESCRIPTION
===========

A ripoff of git version control with a faster blake3 hashing function, tagging functionality for files, support for large resources and nesting of tracked repositories.

Options
-------

-h, --help

:   Prints synopsis and all available commands.

-v, --version

:   Prints the current octal version number.

COMMANDS
========

Porcelain
---------

init

:   Initialize sht repository by creating .sht directory and internals.

status

:   Show which files are tracked.

store

:   Track a file by adding a corresponding filename tag and object copy.

wipe

:   Untrack a file by removing corresponding filename tag and object copy.

tag

:   Map given tags onto specified objects with 'filename:foo' tag.

tree

:   Verify sht repository structure.

check

:   Show tracking status for specific file.

unsuck

:   Check every provided filename for non-compliant characters and prompt for automatic rename.

Plumbing
--------

tree

: Verify sht repository structure.

check

: Show tracking status for specific file.

unsuck

: Check every provided filename for non-compliant characters and prompt for automatic rename.

hash

: Take filename as input and output blake3 hash and filename.

BUGS
====

See GitHub Issues or report your own: <https://github.com/ben256dev/sht/issues>

AUTHOR
======

Benjamin Blodgett <benjamin@ben256.com>

SEE ALSO
========

**sht_parse_error(1)**
