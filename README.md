## contributing

For collaborators:

```bash
git clone git@github.com:ben256dev/sht.git
cd sht
git checkout -b <your new branch name here>
uname -u
# make some changes
# git push origin <your new branch name here>
```

For non-collaborators, fork the repo and make a pull request from there. You can also ask for an invite.

Once you make your pull request, you can yell at me or call me or whatever you have to do to get my attention for a merge.

## build

```bash
$ uname -m # shows your architecture
x86_64
$ make build-
build-arm  build-x86  
$ make build-x86 # in my case its x86 but there is also a target for arm
```

## using the utility

Make it easier to run:

```bash
$ export PATH=$PATH:$(pwd)
```

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
└── refs

3 directories, 0 files
```

I stole the directory structure from .git. To demonstrate how it works, lets track a file.

```bash
$ sht status
sh: b3sum: command not found
...
```

If you get the above output, then you need to install the b3sum utility. Maybe either via cargo or homebrew.

```bash
$ sht status
Untracked Directories:
  (NOTE: sht does not maintain a tree structure for directories)
  (NOTE: use "--recursive" to track contents of directories recursively)
    .sht/
    .git/

Untracked Files:
  sht
  sht.c
  README.md
  .gitignore
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
  sht.c
  README.md
  .gitignore
$ tree .sht
.sht
├── objects
│   └── 94
│       └── e6d169f481b2c07db644d8a675ebf4b7e02b72bdb88e5b6c1180f5e7c7cb94
├── refs
│   └── sht
├── status-directories.sht
├── status.sht
├── status-tracked.sht
├── status-untracked.sht
├── store-track.sht
└── sucks.sht

4 directories, 8 files
```

Now we see the result of ``sht store`` reflected in the directory structure and sht status. The ``94`` directory under ``objects`` is the first two bytes of a hash. The remaining 62 bytes are the name to a file in the ``94`` directory.

The ``refs`` directory also contains a new file ``sht``. In the ``tags`` branch, this directory has been renamed to ``tags`` and instead of simply storing the filename we would see ``filename:sht``. The current state of ``main`` makes this part confusing. We can run some commands to make it clear what is going on.

```bash
$ cat .sht/refs/sht
94e6d169f481b2c07db644d8a675ebf4b7e02b72bdb88e5b6c1180f5e7c7cb94
$ diff .sht/objects/94/e6d16* sht
$ echo $?
0
```

- Firstly, cat'ing the file contents of the newly stored ref gives the full hash stored in ``objects``
- Second, diff'ing the object file with our original file we stored gives nothing and ``0`` is returned.

This is because refs contains our tags for a file. In this case, (and in every case on the ``main`` branch) the tag is a filename that matches to a particular sht object hash. The contents of the file are literally just the hash. The object file has the **blake3** hash of the ``sht`` file as it's filename. The contents are copied over from the file itself into the file, so that our sht object constitutes an exact copy of the file we stored and it can be referenced by the file's blake3 hash.

So what is the behavior for duplicate files?

```bash
$ cp sht shit
$ sht store shit
$ tree .sht
.sht
├── objects
│   └── 94
│       └── e6d169f481b2c07db644d8a675ebf4b7e02b72bdb88e5b6c1180f5e7c7cb94
├── refs
│   ├── shit
│   └── sht
├── status-directories.sht
├── status.sht
├── status-tracked.sht
├── status-untracked.sht
├── store-track.sht
└── sucks.sht

4 directories, 9 files
$ sht status
Untracked Directories:
  (NOTE: sht does not maintain a tree structure for directories)
  (NOTE: use "--recursive" to track contents of directories recursively)
    .sht/
    .git/

Tracked Files:
  sht
  shit

Untracked Files:
  sht.c
  README.md
  .gitignore
```

As you can see, the duplicate file is still "tracked" by sht. But since we only re-hash unique file contents, both of the filename tags map onto the same sht object hash:

```bash
$ cat .sht/refs/sh*t
94e6d169f481b2c07db644d8a675ebf4b7e02b72bdb88e5b6c1180f5e7c7cb94
94e6d169f481b2c07db644d8a675ebf4b7e02b72bdb88e5b6c1180f5e7c7cb94
```

## Autocomplete

To enable autocompletion, run the following command in the base directory

```bash
$ echo "source $(pwd)/sht_complete.sh" >> ~/.bashrc
# Don't forget to re-source your .bashrc also
$ source ~/.bashrc
```

## How to contribute

Here's a bug I ran into while writing this readme:

```bash
$ sht wipe sh*t
Error: file "sht" ref found but object parent dir 94/ unaccessable
$ tree .sht
.sht
├── objects
├── refs
│   └── sht
├── status-directories.sht
├── status.sht
├── status-tracked.sht
├── status-untracked.sht
├── store-track.sht
└── sucks.sht

3 directories, 7 files
$ sht status
Untracked Directories:
  (NOTE: sht does not maintain a tree structure for directories)
  (NOTE: use "--recursive" to track contents of directories recursively)
    .sht/
    .git/

Untracked Files:
  sht
  sht.c
  README.md
  .gitignore
  shit
```

When wiping a "ref" sht assumes that it correctly maps onto an existing object. Probably it should simply check the filename tag for a hash, delete it if one is found, and secondly delete the ref so that one isn't left over. At least right now status correctly shows that the object is untracked.

### Todos

- The ``tags`` branch contains work on the tag system to allow arbitrarily tagging an object with any number of tags.
- There needs to be a mechanism for setting remotes and uploading objects. Probably with **sftp** and ``libssl2``.
- These commands should be more colorful. ``log_colorful`` in the log library on ``g10.app/status`` could be useful for this.
- Work has already been started on ``main`` to search directories recursively for files instead of just ignoring them.
- In the future, we could support branches, trees and commit objects with a directed graph for version control.
- sht could use a fully-commented header for the sake of providing a well documented interface.
- sht has no helpful usage message.
- sht may only work for Linux and Mac.
- There's probably something you can come up that needs improved. If so, please make changes.
