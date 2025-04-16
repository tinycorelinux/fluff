#makefile for fltk program
CXXFLAGS="-march=i486 -mtune=i686 -Os -pipe -fno-exceptions -fno-rtti"
PROG=fluff
PROG_CAPS=FileMgr
DESC="File Manager"
#UNCOMMENT THE FIRST LINE FOR Tiny Core 3.x, THE SECOND for Core 4.x
#TCEDIR=`cat /opt/.tce_dir`
TCEDIR="/etc/sysconfig/tcedir"
BASEDIR=`pwd`

all: ${PROG}.cpp
	g++ ${CXXFLAGS} `fltk-config --cxxflags` -Wall -c ${PROG}.cpp
	g++ `fltk-config --use-images --ldflags` -lfltk_images -lm ${PROG}.o -o ${PROG}
	strip ${PROG}
	echo `ls -l fluff`

debug: ${PROG}.cpp
#	g++ ${CXXFLAGS} `fltk-config --cxxflags` -g -O0 -c ${PROG}.cpp
	g++ ${CXXFLAGS} `fltk-config --cxxflags` -g -c ${PROG}.cpp
	g++ `fltk-config --use-images --ldflags` -lfltk_images -lm ${PROG}.o -o ${PROG}

clean:
	rm *.o ${PROG}

run:
	fluff &
	
oldpackage: ${PROG}
	./build-tcz_for_tc_2_10.sh

package: all
	./build-tcz.sh ${PROG} ${PROG_CAPS} ${BASEDIR} ${DESC} "FULL"

upgrade_package:  ${PROG}
	./build-tcz.sh ${PROG} ${PROG_CAPS} ${BASEDIR} ${DESC} "UPGRADE"

	
tarball:
	tar -czf ${PROG}_src.tar.gz fluff.cpp fluff_fc.sh fluff.png fluff_help.htm build-tcz.sh fluff.gif test.sh mkdirs.sh Makefile
	
unpack:
	tar -xzf ${PROG}_src.tar.gz
	

install: package
	if grep ${PROG}.tcz ${TCEDIR}/onboot.lst; \
	then echo "Already installed. Use \"make update.\""; \
	else echo "${PROG}.tcz" >> ${TCEDIR}/onboot.lst; \
		sudo cp ${PROG}.tcz ${TCEDIR}/optional; \
		tce-load -ic ${PROG}.tcz; \
	fi
	

update: package
	if grep ${PROG}.tcz ${TCEDIR}/onboot.lst; \
	then sudo cp ${PROG}.tcz ${TCEDIR}/optional; \
	sudo cp ${PROG}          /usr/local/bin/; \
	sudo cp fluff_fc.sh      /usr/local/bin/; \
	sudo cp ${PROG}_help.htm /usr/local/share/doc/fluff/; \
	sudo cp ${PROG}.gif      /usr/local/share/doc/fluff/; \
	sudo cp ${PROG}.png /usr/local/share/pixmaps; \
	else echo "Not installed yet. Use \"make install.\""; \
	fi
	

