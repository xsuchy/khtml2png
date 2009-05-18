#!/bin/sh

newestTar=`ls ../khtml2png-* | sort | tail -1`
version=`echo $newestTar | cut -d'-' -f2 | sed 's/.tar.gz//g'`

/mdk/m23update/sf-upload/sf-upload -pr khtml2png -p khtml2png2 -r $version -f $newestTar -type .gz