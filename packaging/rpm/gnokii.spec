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

cat <<EOM |patch -p1
diff -u gnokii-0.2.4-orig/Makefile gnokii-0.2.4/Makefile
--- gnokii-0.2.4-orig/Makefile	Sun Mar 28 12:17:00 1999
+++ gnokii-0.2.4/Makefile	Sun Mar 28 12:18:06 1999
@@ -16,7 +16,7 @@
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

%install
rm -rf $RPM_BUILD_ROOT

mkdir -p $RPM_BUILD_ROOT/usr/bin

cp gnokii3810 gnokii6110 $RPM_BUILD_ROOT/usr/bin

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
%doc BUGS COPYING MANIFEST README
/usr/bin/*

%changelog
* Sun Mar 28 1999 Pavel Janik ml. <Pavel.Janik@linux.cz>
- upgraded to gnokii-0.2.4
- 6110 version of gnokii added

* Fri Mar  5 1999 Pavel Janik ml. <Pavel.Janik@linux.cz>
- the first SPEC file for gnokii
