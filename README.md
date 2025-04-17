# Fluff
FLTK file manager

# Overview

Fluff is a fast, light utility for files.  Fluff uses
the FLTK user interface library, which makes it
especially efficient on TinyCore Linux. The source code
of Fluff is released under the GNU license.  This file
manager features a directory tree and file details list;
keyboard, menu, and drag-and-drop file manipulations;
automatic configurable program associations; trashbin with
restore; file renaming; file property editing; and more!
Less than 150 KB installed.

# Building
The build script runs in busybox shell and will create .tcz package and 
related files for TinyCore Linux.

./compileit

# Change Log
	2011/05/13 (Initial release as extension)
	2010/05/13 1.0.0
	2014/03/19 v1.0.7 compiled for corepure64-v5.x (coreplayer2)
	2020/01/30 v1.0.7 compiled for tc-11.x and fltk-1.3.5 (gnuser)
	2020/02/07 updated 1.0.7 -> 1.0.8 (gnuser)
	2022/02/28 v1.0.9 addressed some build warnings, reverted some changes (Rich)
	2025/04/16 v1.1.6 - Juanito & Michael A. Losh - FLTK 1.4 updates, new nav buttons, string buffer size fixes
