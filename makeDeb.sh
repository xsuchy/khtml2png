#!/bin/sh

#enter package directory
cd debian

cd DEBIAN
if test -f control
then
	nano control

	ver=`grep Version control | cut -d' ' -f2`
	package=`grep Package control | cut -d' ' -f2`
	arch=`grep Architecture control | cut -d' ' -f2`
else
	echo "the file \"control\" is missing!"
	exit 1
fi
cd ..

mkdir -p usr/bin
cp ../khtml2png2 usr/bin

dpkg-deb --build . ../$package"_"$ver"_"$arch.deb