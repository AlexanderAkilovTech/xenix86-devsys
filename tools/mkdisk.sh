#!/bin/sh
# mkdisk.sh - pack files into a 360K floppy image for XENIX: a version 7 tar,
# padded to 368640 bytes. On XENIX:  tar xvf /dev/fd0
# usage: tools/mkdisk.sh image.img directory
# Every file under the directory goes in, by its path relative to it. Only files
# are listed, not directories: XENIX tar makes the directories it needs, and it
# reports "cannot create" for a directory entry. Every file is dated 1 January
# 1986: XENIX 2.1.3 cannot be set to a year past 1999, so a file dated today
# would look decades newer than anything built on it, and make would rebuild
# everything every time.
set -e
img=$1; dir=$2
[ -n "$img" ] && [ -d "$dir" ] || { echo "usage: $0 image.img directory" >&2; exit 2; }
case "$img" in /*) ;; *) img="$(pwd)/$img" ;; esac
(cd "$dir" && find . -type f | sed 's|^\./||' | sort |
	tar --format=v7 --owner=0 --group=0 --mtime='1986-01-01 00:00:00 UTC' --no-recursion -cf "$img" -T -)
size=$(wc -c < "$img")
[ "$size" -le 368640 ] || { echo "mkdisk.sh: $size bytes, more than a 360K floppy" >&2; rm -f "$img"; exit 1; }
truncate -s 368640 "$img"
echo "$img: $size bytes of tar on a 360K image"
