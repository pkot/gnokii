#!/bin/sh

##### config begin #####
MINGW_PATH=/usr/mingw
HOST=i386-pc-mingw32
##### config end #######

OLDPATH=$PATH
PATH=$MINGW_PATH/bin:$OLDPATH
export PATH
./configure --host=$HOST --prefix=/ --enable-security --disable-nls --enable-win32

HOST_CC="PATH=$OLDPATH cc"
export HOST_CC


cat <<EOF >confsubst
s%^exeext=.*$%exeext="exe"%
s%^libext=.*$%libext="lib"%
s%^libname_spec=.*$%libname_spec="\\\\\$name"%
EOF
cp libtool libtool.orig
sed -f confsubst <libtool.orig >libtool
rm confsubst
chmod +x libtool

make
