%define name gnokii
%define version @@VERSION@@
%define release 1

Summary: Linux/Unix tool suite for Nokia mobile phones
Name: %{name}
Version: %{version}
Release: %{release}
Copyright: GPL
Packager: Pavel Janik ml. <Pavel.Janik@linux.cz>
Group: Applications/Communications
Source: ftp://multivac.fatburen.org/pub/gnokii/%{name}-%{version}.tar.gz
BuildRoot: /var/tmp/%{name}-%{version}-root

%description
Gnokii is a Linux/Unix tool suite and (eventually) modem/fax driver for Nokia
mobile phones, released under the GPL.

%prep

%setup -q

%build
make

cp gnokii gnokii3810
cp gnokiid gnokiid3810
cp xgnokii xgnokii3810

cat <<EOM |patch -p1
--- Makefile.orig	Sun May  9 21:15:17 1999
+++ Makefile	Sun May  9 21:15:25 1999
@@ -32,7 +32,7 @@
 # For Nokia 6110/5110 uncomment the next line
 #
 
-# MODEL=-DMODEL="\"6110\""
+MODEL=-DMODEL="\"6110\""
 
 #
 # Serial port for communication
EOM

make clean
make

cp gnokii gnokii6110
cp gnokiid gnokiid6110
cp xgnokii xgnokii6110

%install
rm -rf $RPM_BUILD_ROOT

mkdir -p $RPM_BUILD_ROOT/usr/bin

cp gnokii3810 gnokii6110 gnokiid3810 gnokiid6110 xgnokii3810 xgnokii6110 $RPM_BUILD_ROOT/usr/bin

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
%doc BUGS COPYING MANIFEST README TODO-6110
/usr/bin/*

%changelog
* Mon Jun 28 1999 Pavel Janik ml. <Pavel.Janik@linux.cz>
- Nokia auth. protocol is there now
- xgnokii and gnokiid added to RPM

* Sun May  9 1999 Pavel Janik ml. <Pavel.Janik@linux.cz>
- included 6110-patch updated to my prepatches

* Thu May  6 1999 Pavel Janik ml. <Pavel.Janik@linux.cz>
- TODO-6110 file added to documentation files

* Sun Mar 28 1999 Pavel Janik ml. <Pavel.Janik@linux.cz>
- upgraded to gnokii-0.2.4
- 6110 version of gnokii added

* Fri Mar  5 1999 Pavel Janik ml. <Pavel.Janik@linux.cz>
- the first SPEC file for gnokii
