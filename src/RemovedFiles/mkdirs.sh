#!/bin/sh
echo "Setting up dummy directories to test delete functions"
cd /tmp
if [ -e testdirs ] ; 
then
    rm -rf /tmp/testdirs
fi

mkdir testdirs
cd testdirs
mkdir 1dir
mkdir 2dir
mkdir 3dir
mkdir 4dir

cd /tmp/testdirs/1dir
touch 1file
touch 2file
touch 3file
mkdir 1dir1subdir
cd 1dir1subdir
touch 1-1-1file
touch 1-1-2file
touch 1-1-3file
cd ..
mkdir 1dir2subdir
cd 1dir2subdir
touch 1-2-1file
touch 1-2-2file
touch 1-2-3file

cd /tmp/testdirs/3dir
mkdir 3dir1subdir
cd 3dir1subdir
touch 3-1-1file
touch 3-1-2file
touch 3-1-3file

cd /tmp/testdirs/4dir
touch 1file
touch 2file
touch 3file


