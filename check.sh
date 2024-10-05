#!/bin/bash

for d in ".lit/" ".lit/objects/" ".lit/refs/"
do
if [ ! -d $d ]; then
   echo "missing \"$d\" file, lit repo does not exist"
   echo "  (run \"lit init\" to initialize lit repository)"
   exit -1
fi
done

exit 0
