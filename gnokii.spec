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

%install
rm -rf $RPM_BUILD_ROOT

mkdir -p $RPM_BUILD_ROOT/usr/bin

cp gnokii gnokiid xgnokii $RPM_BUILD_ROOT/usr/bin

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
%doc BUGS COPYING MANIFEST README TODO-6110 TODO-gnokiid sample.gnokiirc gettext-howto
/usr/bin/*

%changelog
* Sat Jul 10 1999 Pavel Janik ml. <Pavel.Janik@linux.cz>
- use of ~/.gnokiirc so not magic model stuff
- new doc files

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
