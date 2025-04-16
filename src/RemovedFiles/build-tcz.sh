#!/bin/sh
# Build .tcz extension package for version 3.x and 4.x of Tinycore

PROG=$1
PROG_CAPS=$2
BASEDIR=$3
DESC=$4
PACKAGE_TYPE="FULL"
PACKAGE_FILE=$PROG".tcz"

if [ $5 = "UPGRADE" ]; then
	PACKAGE_TYPE=$5
	PACKAGE_FILE=$PROG"_upgrade.tcz"
fi
echo "Package name: " $PACKAGE_FILE

mkdir /tmp/${PROG}
cd /tmp/${PROG}

# Executable
mkdir -p usr/local/bin
cp ${BASEDIR}/${PROG} usr/local/bin/
chmod 755 usr/local/bin/${PROG}
cp ${BASEDIR}/fluff_fc.sh usr/local/bin/
chmod 755 usr/local/bin/fluff_fc.sh

# Help file
mkdir -p usr/local/share/doc/fluff
cp ${BASEDIR}/${PROG}_help.htm usr/local/share/doc/fluff/
cp ${BASEDIR}/${PROG}.gif      usr/local/share/doc/fluff/

if [ $PACKAGE_TYPE = "FULL" ]; then
	# Icon
	mkdir -p usr/local/share/pixmaps
	cp ${BASEDIR}/${PROG}.png usr/local/share/pixmaps

	# .desktop file for icons on desctop, menu items
	DOTDESKTOP=usr/local/share/applications/${PROG}.desktop
	mkdir usr/local/share/applications
	echo "[Desktop Entry]" > ${DOTDESKTOP}
	echo "Encoding=UTF-8" >> ${DOTDESKTOP}
	echo "Name=${PROG}" >> ${DOTDESKTOP}
	echo "Comment=${DESC}" >> ${DOTDESKTOP}
	echo "GenericName=${PROG_CAPS}" >> ${DOTDESKTOP}
	echo "Exec=${PROG}" >> ${DOTDESKTOP}
	echo "Icon=${PROG}" >> ${DOTDESKTOP}
	echo "Terminal=false" >> ${DOTDESKTOP}
	echo "StartupNotify=true" >> ${DOTDESKTOP}
	echo "Type=Application" >> ${DOTDESKTOP}
	echo "Categories=Utility;" >> ${DOTDESKTOP}
	echo "X-FullPathIcon=/usr/local/share/pixmaps/${PROG}.png" >> ${DOTDESKTOP}
	echo "******************************"
	cat ${DOTDESKTOP}
	echo "******************************"
fi

cd usr/local
cd /tmp/${PROG}
mksquashfs . ../${PACKAGE_FILE}

#cd $TMPDIR
find usr -not -type d > ../${PACKAGE_FILE}.list

# Create md5 file
cd /tmp
md5sum ${PACKAGE_FILE} > ${PACKAGE_FILE}.md5.txt

cp /tmp/${PACKAGE_FILE}* ${BASEDIR}
rm -rf /tmp/${PROG}
rm -f /tmp/${PACKAGE_FILE}*

echo "Build of .tcz for TC 3.0 and higher is complete."
