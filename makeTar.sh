echo -n "Version: "
read ver

#remove backup files
find | grep "~$" | xargs rm 2> /dev/null

mkdir khtml2png-$ver
cp CMakeLists.txt make* configure khtml2png.cpp khtml2png.h README ChangeLog khtml2png-$ver
tar cfzv khtml2png-$ver.tar.gz khtml2png-$ver
rm -r khtml2png-$ver
