Summary: Graphical IRC (chat) client
Name: xchat
Version: 1.8.5
Release: 0
Epoch: 1
Group: Applications/Internet
Copyright: GPL
Url: http://xchat.org
Source: http://xchat.org/files/source/1.8/xchat-%{version}.tar.bz2
Buildroot: %{_tmppath}/%{name}-%{version}-%{release}-root
#BuildRequires: gnome-libs-devel
#BuildRequires: gtk+-devel

%description
A GUI IRC client with DCC, plugin, Perl and Python scripting
capability, mIRC color, shaded transparency, tabbed channels
and more.

%prep

%setup

%build
CFLAGS="$RPM_OPT_FLAGS" ./configure --prefix=%{_prefix} --bindir=%{_bindir} --datadir=%{_datadir} --disable-textfe --enable-openssl --disable-gdk-pixbuf --disable-python %{_target_platform}
make

%install
if [ -d $RPM_BUILD_ROOT ]; then rm -r $RPM_BUILD_ROOT; fi
mkdir -p $RPM_BUILD_ROOT%{_prefix}
mkdir -p $RPM_BUILD_ROOT%{_bindir}
mkdir -p $RPM_BUILD_ROOT%{_datadir}

make prefix=$RPM_BUILD_ROOT%{_prefix} \
    bindir=$RPM_BUILD_ROOT%{_bindir} \
    datadir=$RPM_BUILD_ROOT%{_datadir} \
    install-strip


%find_lang %{name}

%files -f %{name}.lang
%defattr(-,root,root)
%doc README FAQ COPYING ChangeLog
%attr(755,root,root) %{_bindir}/*
%{_datadir}/gnome/apps/Internet/xchat.desktop
%{_datadir}/pixmaps/xchat.png

%clean
rm -rf $RPM_BUILD_ROOT
