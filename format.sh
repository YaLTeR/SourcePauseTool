#!/bin/sh

FILES="$(find spt -regex '.+\.\(c\(pp\)?\|h\(pp\)?\)' | grep -Ev 'utils/md5')"
exec clang-format -i $FILES
