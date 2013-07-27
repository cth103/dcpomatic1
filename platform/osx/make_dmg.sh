#!/bin/bash

version=`cat wscript | egrep ^VERSION | awk '{print $3}' | sed -e "s/'//g"`

# DMG size in megabytes
DMG_SIZE=256
WORK=build/platform/osx
ENV=/Users/carl/Environments/osx
ROOT=/Users/carl/cdist

appdir="DVD-o-matic.app"
approot=$appdir/Contents
libs=$approot/lib
macos=$approot/MacOS
resources=$approot/Resources

rm -rf $WORK/$appdir
mkdir -p $WORK/$macos
mkdir -p $WORK/$libs
mkdir -p $WORK/$resources

function universal_copy {
    echo $2
    for f in $1/32/$2; do
        if [ -h $f ]; then
	    ln -s $(readlink $f) $3/`basename $f`
        else
          g=`echo $f | sed -e "s/\/32\//\/64\//g"`
	  mkdir -p $3
          lipo -create $f $g -output $3/`basename $f`
        fi
    done
}

universal_copy $ROOT src/dvdomatic/build/src/tools/dvdomatic $WORK/$macos
universal_copy $ROOT src/dvdomatic/build/src/lib/libdvdomatic.dylib $WORK/$libs
universal_copy $ROOT src/dvdomatic/build/src/wx/libdvdomatic-wx.dylib $WORK/$libs
universal_copy $ROOT lib/libcxml.dylib $WORK/$libs
universal_copy $ROOT lib/libdcp.dylib $WORK/$libs
universal_copy $ROOT lib/libasdcp-libdcp.dylib $WORK/$libs
universal_copy $ROOT lib/libkumu-libdcp.dylib $WORK/$libs
universal_copy $ROOT lib/libopenjpeg*.dylib $WORK/$libs
universal_copy $ROOT lib/libavformat*.dylib $WORK/$libs
universal_copy $ROOT lib/libavfilter*.dylib $WORK/$libs
universal_copy $ROOT lib/libavutil*.dylib $WORK/$libs
universal_copy $ROOT lib/libavcodec*.dylib $WORK/$libs
universal_copy $ROOT lib/libswscale*.dylib $WORK/$libs
universal_copy $ROOT lib/libpostproc*.dylib $WORK/$libs
universal_copy $ROOT lib/libswresample*.dylib $WORK/$libs
universal_copy $ROOT bin/ffprobe $WORK/$macos
universal_copy $ENV lib/libboost_system.dylib $WORK/$libs
universal_copy $ENV lib/libboost_filesystem.dylib $WORK/$libs
universal_copy $ENV lib/libboost_thread.dylib $WORK/$libs
universal_copy $ENV lib/libboost_date_time.dylib $WORK/$libs
universal_copy $ENV lib/libxml++-2.6*.dylib $WORK/$libs
universal_copy $ENV lib/libxml2*.dylib $WORK/$libs
universal_copy $ENV lib/libglibmm-2.4*.dylib $WORK/$libs
universal_copy $ENV lib/libgobject*.dylib $WORK/$libs
universal_copy $ENV lib/libgthread*.dylib $WORK/$libs
universal_copy $ENV lib/libgmodule*.dylib $WORK/$libs
universal_copy $ENV lib/libsigc*.dylib $WORK/$libs
universal_copy $ENV lib/libglib-2*.dylib $WORK/$libs
universal_copy $ENV lib/libintl*.dylib $WORK/$libs
universal_copy $ENV lib/libsndfile*.dylib $WORK/$libs
universal_copy $ENV lib/libMagick++*.dylib $WORK/$libs
universal_copy $ENV lib/libMagickCore*.dylib $WORK/$libs
universal_copy $ENV lib/libMagickWand*.dylib $WORK/$libs
universal_copy $ENV lib/libssh*.dylib $WORK/$libs
universal_copy $ENV lib/libwx*.dylib $WORK/$libs
universal_copy $ENV lib/libfontconfig*.dylib $WORK/$libs
universal_copy $ENV lib/libfreetype*.dylib $WORK/$libs
universal_copy $ENV lib/libexpat*.dylib $WORK/$libs
universal_copy $ENV lib/libxmlsec1*.dylib $WORK/$libs

for obj in $WORK/$macos/dvdomatic $WORK/$libs/*.dylib; do
  deps=`otool -L $obj | awk '{print $1}' | egrep "(/Users/carl|libboost|libssh)"`
  changes=""
  for dep in $deps; do
    base=`basename $dep`
    changes="$changes -change $dep @executable_path/../lib/$base"
  done
  if test "x$changes" != "x"; then
    install_name_tool $changes $obj
  fi  
done

cp build/platform/osx/Info.plist $WORK/$approot
cp icons/dvdomatic.icns $WORK/$resources/DVD-o-matic.icns

tmp_dmg=$WORK/dvdomatic_tmp.dmg
dmg="$WORK/DVD-o-matic $version.dmg"
vol_name=DVD-o-matic-$version

mkdir -p $WORK/$vol_name

rm -f $tmp_dmg "$dmg"
hdiutil create -megabytes $DMG_SIZE $tmp_dmg
device=$(hdid -nomount $tmp_dmg | grep Apple_HFS | cut -f 1 -d ' ')
newfs_hfs -v ${vol_name} $device
mount -t hfs "$device" $WORK/$vol_name

cp -r $WORK/$appdir $WORK/$vol_name

echo '
  tell application "Finder"
    tell disk "'$vol_name'"
           open
           set current view of container window to icon view
           set toolbar visible of container window to false
           set statusbar visible of container window to false
           set the bounds of container window to {400, 200, 790, 410}
           set theViewOptions to the icon view options of container window
           set arrangement of theViewOptions to not arranged
           set icon size of theViewOptions to 64
           make new alias file at container window to POSIX file "/Applications" with properties {name:"Applications"}
           set position of item "DVD-o-matic.app" of container window to {90, 80}
           set position of item "Applications" of container window to {310, 80}
           close
           open
           update without registering applications
           delay 5
           eject
     end tell
   end tell
' | osascript

chmod -Rf go-w $WORK/mnt
sync

umount $device
hdiutil eject $device
hdiutil convert -format UDZO $tmp_dmg -imagekey zlib-level=9 -o "$dmg"
sips -i $WORK/$resources/DVD-o-matic.icns
DeRez -only icns $WORK/$resources/DVD-o-matic.icns > $WORK/$resources/DVD-o-matic.rsrc
Rez -append $WORK/$resources/DVD-o-matic.rsrc -o "$dmg"
SetFile -a C "$dmg"

