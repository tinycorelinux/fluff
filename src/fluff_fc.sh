#!/bin/sh
# Assess size and recursive filecount within a specified directory

diskspace=`du -sh $1 2> /dev/null`
space_incomplete=$?
filecount=`find $1 -type f -o -type l $2 2> /dev/null > /dev/null`
count_incomplete=$?
filecount=`find $1 -type f -o -type l $2 2> /dev/null | wc -l`

incomplete=0

if [ $space_incomplete -ne 0 ]; then
    incomplete=1
fi
if [ $count_incomplete -ne 0 ]; then
    incomplete=1
fi

echo "incomplete?: $incomplete; filecount: $filecount; space: $diskspace" 
