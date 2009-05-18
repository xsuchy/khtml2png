#!/bin/sh

make distclean
rm -r autom4te.cache* 2> /dev/null
rm `find | grep "~"`
rm `find | grep "tar.gz"` 2> /dev/null

#get version number directly from the source code
version=`grep "KAboutData aboutData" src/khtml2png.cpp -A 1 | tail -1 | cut -d'"' -f2`
#get the name of the directory the source is stored in
find `pwd` -maxdepth 0 -printf "%f\n" > /tmp/dirName
oldDirName=`cat /tmp/dirName`
rm /tmp/dirName
#make a new directory name
newDirName="khtml2png-$version"

cd ..
mv "$oldDirName" "$newDirName"

tar cfvz "$newDirName.tar.gz" "$newDirName"

mv "$newDirName" "$oldDirName"

