Name:      akcom-udpecho
Summary:   UDP echo server/client with support for TR-143 UDPEchoPlus
Version:   0.3
Release:   1%{?dist}
License:   Alaska Communications UDP Echo Tools License
Group:     System Environment/Daemons
URL:       https://github.com/alaskacommunications/akcom-udpecho/releases/download/v0.3/akcom-udpecho-0.3.0.tar.gz
Source0:   %{name}-%{version}.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root
BuildRequires: make gcc which libtool autoconf automake file diffutils gzip git

%description 
This package contains a UDP echo server and client. Both the client and
the server support RFC 862 (default) and support TR-143 UDPEchoPlus if
passed the --echoplus flag.

%prep
%setup -q

%build
echo "build"
cd src
make all-progs PREFIX=%{_prefix}
#gzip ./docs/akcom-udpecho.1
#gzip ./docs/akcom-udpechod.8
find

%install
echo "install"
cd src
find
rm -rf $RPM_BUILD_ROOT
#mkdir -p $RPM_BUILD_ROOT%{_bindir}/
#mkdir -p $RPM_BUILD_ROOT%{_sbindir}/
#mkdir -p $RPM_BUILD_ROOT%{_mandir}/man1/
#mkdir -p $RPM_BUILD_ROOT%{_mandir}/man8/
make install-progs DESTDIR=$RPM_BUILD_ROOT PREFIX=%{_prefix}
#find $RPM_BUILD_ROOT

%files
%defattr(0644,root,root,-)
%attr(0755,root,root) %{_bindir}/akcom-udpecho
%attr(0755,root,root) %{_sbindir}/akcom-udpechod
#%attr(0755,root,root) %{_mandir}/man1/akcom-udpecho.1.gz
#%attr(0755,root,root) %{_mandir}/man8/akcom-udpechod.8.gz

%clean
rm -rf $RPM_BUILD_ROOT

%changelog
