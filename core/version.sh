#!/bin/sh
#
# Syntax:
# $ ./version.sh <path>
# if no <path> provided then it works from the current directory
#

if [ "x$1" = "x" ]
then
  git describe --always --dirty --broke
elif [ -d "$1" ]
then
  ( cd -- "$1"; git describe --always --dirty --broke )
else
  echo "unknown"
fi