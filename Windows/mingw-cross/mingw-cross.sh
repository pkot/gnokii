#!/bin/sh

##### config begin #####
HOST=i386-mingw32
##### config end #######

./configure --host=$HOST --prefix=/ --enable-security --disable-nls --enable-win --without-x

HOST_CC="cc"
export HOST_CC


cat <<EOF >confsubst
s%^soname_spec=.*$%soname_spec="\\\\\${name}.dll"%
EOF
cp libtool libtool.orig
sed -f confsubst <libtool.orig >libtool
rm confsubst
chmod +x libtool

make
