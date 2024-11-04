## contributing

For collaborators:

```bash
git clone git@github.com:ben256dev/sht.git
cd sht
git checkout -b <your new branch name here>
# make some changes
git push origin <your new branch name here>
```

For non-collaborators, fork the repo and make a pull request from there. You can also ask for an invite.

Once you make your pull request, you can yell at me or call me or whatever you have to do to get my attention for a merge.

## build

```bash
$ make install
$ make
```

## using the utility

Try checking the status:
```bash
$ sht status
Not a sht repository
   (run "sht init" to initialize repository)
```

No ``.sht`` directory was found. This should be a hidden directory prefixed with a ``.``. Lets create it and check it's contents with ``tree``.

```bash
$ sht init
sht repository created
$ tree .sht
.sht
├── objects
└── tags

3 directories, 0 files
```

I stole the directory structure from .git. To demonstrate how it works, lets track a file.

```bash
$ sht status
sh: 1: b3sum: not found
b3sum utility not found. proceeding with slower C implementation...
...
```

If you get the above output, then you may want to install the b3sum utility for greater performanc. Maybe either via cargo or homebrew.

```bash
$ sht status
Untracked Directories:
   (NOTE: sht does not maintain a tree structure for directories)
   (NOTE: use "--recursive" to track contents of directories recursively)
         .sht/
         .git/

Untracked Files:
         shtcol.h
         README.md
         .gitignore
         sht_complete.sh
         sht.c
         makefile

$ sht store sht
```
Now sht tracks its own ``sht`` executable. We can check that this was successful in two ways, with ``tree .sht`` or ``sht status``.

```bash
$ sht status
Untracked Directories:
   (NOTE: sht does not maintain a tree structure for directories)
   (NOTE: use "--recursive" to track contents of directories recursively)
         .sht/
         .git/

Tracked Files:
         sht

Untracked Files:
         shtcol.h
         README.md
         .gitignore
         sht_complete.sh
         sht.c
         makefile

$ tree .sht
.sht
├── objects
│   └── ac
│       └── d234175699b3906c5b2d63cb0386d9e0d98e6cad3bb8a1e01e75d004ac879a
├── status-directories.sht
├── status-tracked.sht
├── status-untracked.sht
├── status.sht
├── store-track.sht
├── sucks.sht
└── tags
    └── filename:sht

4 directories, 8 files
```

Now we see the result of ``sht store`` reflected in the directory structure and sht status. The ``94`` directory under ``objects`` is the first two bytes of a hash. The remaining 62 bytes are the name to a file in the ``94`` directory.

The ``tags`` directory also contains a new file ``filename:sht``.

```bash
$ cat .sht/tags/filename\:sht 
acd234175699b3906c5b2d63cb0386d9e0d98e6cad3bb8a1e01e75d004ac879a
$ diff .sht/objects/ac/d23417* sht
$ echo $?
0
```

- Firstly, cat'ing the file contents of the newly stored filename tag gives the full hash stored in ``objects``
- Second, diff'ing the object file with our original file we stored gives nothing and ``0`` is returned.

This is because tags contains our tags for a file. In this case, the tag is a filename that matches to a particular sht object hash. The contents of the file are literally just the hash. The object file has the **blake3** hash of the ``sht`` file as it's filename. The contents are copied over from the file itself into the file, so that our sht object constitutes an exact copy of the file we stored and it can be referenced by the file's blake3 hash.

So what is the behavior for duplicate files?

```bash
$ cp sht shit
$ sht store shit
$ tree .sht
.sht
├── objects
│   └── ac
│       └── d234175699b3906c5b2d63cb0386d9e0d98e6cad3bb8a1e01e75d004ac879a
├── status-directories.sht
├── status-tracked.sht
├── status-untracked.sht
├── status.sht
├── store-track.sht
├── sucks.sht
└── tags
    ├── filename:shit
    └── filename:sht

4 directories, 9 files
ben@vultr:~/sht$ sht status
Untracked Directories:
   (NOTE: sht does not maintain a tree structure for directories)
   (NOTE: use "--recursive" to track contents of directories recursively)
         .sht/
         .git/

Tracked Files:
         sht
         shit

Untracked Files:
         shtcol.h
         README.md
         .gitignore
         sht_complete.sh
         sht.c
         makefile

```

As you can see, the duplicate file is still "tracked" by sht. But since we only re-hash unique file contents, both of the filename tags map onto the same sht object hash:

```bash
$ cat .sht/tags/filename\:sh*t
acd234175699b3906c5b2d63cb0386d9e0d98e6cad3bb8a1e01e75d004ac879a
acd234175699b3906c5b2d63cb0386d9e0d98e6cad3bb8a1e01e75d004ac879a
```

## How to contribute

### Todos

- The ``tags`` branch contains work on the tag system to allow arbitrarily tagging an object with any number of tags.
- There needs to be a mechanism for setting remotes and uploading objects. Probably with **sftp** and ``libssl2``.
- Work has already been started on ``main`` to search directories recursively for files instead of just ignoring them.
- In the future, we could support branches, trees and commit objects with a directed graph for version control.
- sht could use a fully-commented header for the sake of providing a well documented interface.
- sht may only work for Linux and Mac.
- There's probably something you can come up that needs improved. If so, please make changes.
