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
strip gnokii gnokiid xgnokii xlogos xkeyb mgnokiidev

%install
rm -rf $RPM_BUILD_ROOT

mkdir -p $RPM_BUILD_ROOT/usr/bin \
         $RPM_BUILD_ROOT/usr/sbin \
         $RPM_BUILD_ROOT/etc \
         $RPM_BUILD_ROOT/usr/lib/gnokii


cp gnokii gnokiid xgnokii xkeyb xlogos $RPM_BUILD_ROOT/usr/bin
cp mgnokiidev $RPM_BUILD_ROOT/usr/sbin
cp sample.gnokiirc $RPM_BUILD_ROOT/etc/gnokiirc
cp pixmaps/6110.xpm pixmaps/6150.xpm $RPM_BUILD_ROOT/usr/lib/gnokii

%pre
/usr/sbin/groupadd -r -f gnokii >/dev/null 2>&1

%postun
/usr/sbin/groupdel gnokii >/dev/null 2>&1

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
%doc COPYING CREDITS MANIFEST README README-3810 README-6110 sample.gnokiirc gettext-howto gnokii.nol gnokii-ir-howto.txt
/usr/bin/*
/usr/lib/gnokii/*
%attr(4754, root, gnokii) /usr/sbin/*
%config /etc/gnokiirc

%changelog
* Thu Aug  5 1999 Pavel Janik ml. <Pavel.Janik@linux.cz>
- documentation files changed

* Thu Aug  5 1999 Pavel Janik ml. <Pavel.Janik@linux.cz>
- xkeyb and xlogos added to RPM package

* Sat Jul 24 1999 Pavel Janik ml. <Pavel.Janik@linux.cz>
- stripping of binaries

* Thu Jul 22 1999 Pavel Janik ml. <Pavel.Janik@linux.cz>
- gnokii.nol added - nice example for logo uploading

* Sun Jul 18 1999 Pavel Janik ml. <Pavel.Janik@linux.cz>
- mgnokiidev added to RPM package
- config file in /etc (it is not used now...)

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
