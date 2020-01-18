%global qt_module qtwayland

%define docs 1

Summary: Qt5 - Wayland platform support and QtCompositor module
Name:    qt5-%{qt_module}
Version: 5.9.2
Release: 1%{?dist}

License: LGPLv3
Url:     http://www.qt.io
Source0: https://download.qt.io/official_releases/qt/5.9/%{version}/submodules/%{qt_module}-opensource-src-%{version}.tar.xz

# filter qml provides
%global __provides_exclude_from ^%{_qt5_archdatadir}/qml/.*\\.so$

BuildRequires:  qt5-qtbase-devel >= %{version}
BuildRequires:  qt5-qtbase-static
BuildRequires:  qt5-qtbase-private-devel
%{?_qt5:Requires: %{_qt5}%{?_isa} = %{_qt5_version}}
BuildRequires:  qt5-qtdeclarative-devel

BuildRequires:  pkgconfig(xkbcommon)
BuildRequires:  pkgconfig(wayland-scanner)
BuildRequires:  pkgconfig(wayland-server)
BuildRequires:  pkgconfig(wayland-client)
BuildRequires:  pkgconfig(wayland-cursor)
BuildRequires:  pkgconfig(wayland-egl)
BuildRequires:  pkgconfig(egl)
BuildRequires:  pkgconfig(gl)
BuildRequires:  pkgconfig(glesv2)
BuildRequires:  pkgconfig(xcomposite)
BuildRequires:  pkgconfig(xrender)
BuildRequires:  pkgconfig(libudev)
BuildRequires:  pkgconfig(libinput)

ExcludeArch:    s390 s390x

%description
%{summary}.

%if 0%{?docs}
%package doc
Summary: API documentation for %{name}
License: GFDL
Requires: %{name} = %{version}-%{release}
BuildRequires: qt5-qhelpgenerator
BuildRequires: qt5-qdoc
BuildArch: noarch
%description doc
%{summary}.
%endif

%package devel
Summary: Development files for %{name}
Requires: %{name}%{?_isa} = %{version}-%{release}
Requires: qt5-qtbase-devel%{?_isa}
%description devel
%{summary}.

%package examples
Summary: Programming examples for %{name}
Requires: %{name}%{?_isa} = %{version}-%{release}
%description examples
%{summary}.


%prep
%setup -q -n %{qt_module}-opensource-src-%{version}


%build
%{qmake_qt5}
make %{?_smp_mflags}

%if 0%{?docs}
# HACK to avoid multilib conflicts in noarch content
# see also https://bugreports.qt-project.org/browse/QTBUG-42071
QT_HASH_SEED=0; export QT_HASH_SEED
make %{?_smp_mflags} docs
%endif

%install
make install INSTALL_ROOT=%{buildroot}

%if 0%{?docs}
make install_docs INSTALL_ROOT=%{buildroot}
%endif

## .prl/.la file love
# nuke .prl reference(s) to %%buildroot, excessive (.la-like) libs
pushd %{buildroot}%{_qt5_libdir}
for prl_file in libQt5*.prl ; do
  sed -i -e "/^QMAKE_PRL_BUILD_DIR/d" ${prl_file}
  if [ -f "$(basename ${prl_file} .prl).so" ]; then
    rm -fv "$(basename ${prl_file} .prl).la"
    sed -i -e "/^QMAKE_PRL_LIBS/d" ${prl_file}
  fi
done
popd


%post -p /sbin/ldconfig
%postun -p /sbin/ldconfig

%files
%doc README
%license LICENSE.LGPL*
%{_qt5_libdir}/libQt5WaylandCompositor.so.5*
%{_qt5_libdir}/libQt5WaylandClient.so.5*
%{_qt5_plugindir}/wayland-decoration-client/
%{_qt5_plugindir}/wayland-graphics-integration-server
%{_qt5_plugindir}/wayland-graphics-integration-client
%{_qt5_plugindir}/wayland-shell-integration
%{_qt5_plugindir}/platforms/libqwayland-egl.so
%{_qt5_plugindir}/platforms/libqwayland-generic.so
%{_qt5_plugindir}/platforms/libqwayland-xcomposite-egl.so
%{_qt5_plugindir}/platforms/libqwayland-xcomposite-glx.so
%{_qt5_qmldir}/QtWayland/

%if 0%{?docs}
%files doc
%doc LICENSE.FDL
%{_qt5_docdir}/qtwaylandcompositor/
%{_qt5_docdir}/qtwaylandcompositor.qch
%endif

%files devel
%{_qt5_bindir}/qtwaylandscanner
%{_qt5_headerdir}/QtWaylandCompositor/
%{_qt5_headerdir}/QtWaylandClient/
%{_qt5_libdir}/libQt5WaylandCompositor.so
%{_qt5_libdir}/libQt5WaylandClient.so
%{_qt5_libdir}/libQt5WaylandCompositor.prl
%{_qt5_libdir}/libQt5WaylandClient.prl
%{_qt5_libdir}/pkgconfig/*.pc
%{_qt5_archdatadir}/mkspecs/modules/*.pri
%{_qt5_libdir}/cmake/Qt5WaylandCompositor/
%{_qt5_libdir}/cmake/Qt5Gui/Qt5Gui_*.cmake
%{_qt5_libdir}/cmake/Qt5WaylandClient/

%files examples
%{_qt5_examplesdir}/wayland/


%changelog
* Fri Oct 06 2017 Jan Grulich <jgrulich@redhat.com> - 5.9.2-1
- Update to 5.9.2
  Resolves: bz#1317415

* Thu Sep 21 2017 Jan Grulich <jgrulich@redhat.com> - 5.9.1-1
- 5.9.1 - initial RHEL release
  Resolves: bz#1317415

* Fri Jun 16 2017 Rex Dieter <rdieter@fedoraproject.org> - 5.9.0-2
- .spec cosmetics, Source URL, refer to qt5- builddeps directly

* Wed May 31 2017 Helio Chissini de Castro <helio@kde.org> - 5.9.0-1
- Upstream official release

* Fri May 26 2017 Helio Chissini de Castro <helio@kde.org> - 5.9.0-0.1.rc
- Upstream Release Candidate retagged

* Tue May 09 2017 Helio Chissini de Castro <helio@kde.org> - 5.9.0-0.beta.3
- Upstream beta 3

* Mon Jan 30 2017 Helio Chissini de Castro <helio@kde.org> - 5.8.0-1
- New upstream version

* Mon Jan 02 2017 Rex Dieter <rdieter@math.unl.edu> - 5.7.1-3
- filter qml provides, BR: qtbase-private-devel qtdeclarative explicitly

* Sat Dec 10 2016 Rex Dieter <rdieter@fedoraproject.org> - 5.7.1-2
- drop BR: cmake (handled by qt5-rpm-macros now)
- 5.7.1 dec5 snapshot

* Wed Nov 09 2016 Helio Chissini de Castro <helio@kde.org> - 5.7.1-1
- New upstream version

* Mon Jul 04 2016 Helio Chissini de Castro <helio@kde.org> - 5.7.0-2
- Compiled with gcc

* Wed Jun 15 2016 Helio Chissini de Castro <helio@kde.org> - 5.7.0-1
- Qt 5.7.0 release

* Thu Jun 09 2016 Jan Grulich <jgrulich@redhat.com> - 5.6.1-1
- Update to 5.6.1

* Mon Mar 21 2016 Rex Dieter <rdieter@fedoraproject.org> - 5.6.0-11
- rebuild

* Fri Mar 18 2016 Rex Dieter <rdieter@fedoraproject.org> - 5.6.0-10
- rebuild

* Tue Mar 15 2016 Peter Robinson <pbrobinson@fedoraproject.org> 5.6.0-9
- Bump release to 9 so it's higher than the final RC

* Mon Mar 14 2016 Helio Chissini de Castro <helio@kde.org> - 5.6.0-1
- 5.6.0 final release

* Tue Feb 23 2016 Helio Chissini de Castro <helio@kde.org> - 5.6.0-8.rc
- Update to final RC

* Mon Feb 15 2016 Helio Chissini de Castro <helio@kde.org> - 5.6.0-7
- Update RC release

* Thu Feb 04 2016 Fedora Release Engineering <releng@fedoraproject.org> - 5.6.0-6.rc
- Rebuilt for https://fedoraproject.org/wiki/Fedora_24_Mass_Rebuild

* Mon Feb 01 2016 Rex Dieter <rdieter@fedoraproject.org> 5.6.0-5.rc
- use %%qmake_qt5 consistently

* Mon Dec 28 2015 Rex Dieter <rdieter@fedoraproject.org> 5.6.0-4.rc
- BR: cmake, update source URL, use %%license

* Mon Dec 21 2015 Helio Chissini de Castro <helio@kde.org> - 5.6.0-3
- Update to final rc release

* Thu Dec 10 2015 Helio Chissini de Castro <helio@kde.org> - 5.6.0-2
- Official rc release

* Tue Nov 03 2015 Helio Chissini de Castro <helio@kde.org> - 5.6.0-0.1
- Start to implement 5.6.0 rc

* Thu Oct 15 2015 Helio Chissini de Castro <helio@kde.org> - 5.5.1-2
- Update to final release 5.5.1

* Tue Sep 29 2015 Helio Chissini de Castro <helio@kde.org> - 5.5.1-1
- Update to Qt 5.5.1 RC1

* Thu Jul 16 2015 Rex Dieter <rdieter@fedoraproject.org> 5.5.0-3
- tighten qtbase dep (#1233829)

* Sun Jul 05 2015 Pier Luigi Fiorini <pierluigi.fiorini@gmail.com> - 5.5.0-2
- Add xkbcommon to the devel package.

* Wed Jul 1 2015 Helio Chissini de Castro <helio@kde.org> 5.5.0-1
- New final upstream release Qt 5.5.0

* Thu Jun 25 2015 Helio Chissini de Castro <helio@kde.org> - 5.5.0-0.2.rc
- Update for official RC1 released packages

* Wed Jun 17 2015 Daniel Vrátil <dvratil@redhat.com> - 5.5.0-0.1.rc
- Qt5 5.5.0 RC1

* Wed Jun 03 2015 Jan Grulich <jgrulich@redhat.com> - 5.4.2-1
- 5.4.2

* Sat May 02 2015 Kalev Lember <kalevlember@gmail.com> - 5.4.1-2
- Rebuilt for GCC 5 C++11 ABI change

* Fri Feb 27 2015 Rex Dieter <rdieter@fedoraproject.org> 5.4.1-1
- 5.4.1

* Wed Dec 10 2014 Rex Dieter <rdieter@fedoraproject.org> 5.4.0-1
- 5.4.0 (final)

* Fri Nov 28 2014 Rex Dieter <rdieter@fedoraproject.org> 5.4.0-0.3.rc
- 5.4.0-rc

* Mon Nov 03 2014 Rex Dieter <rdieter@fedoraproject.org> 5.4.0-0.2.rc
- use %%qmake_qt5 macro

* Mon Oct 20 2014 Rex Dieter <rdieter@fedoraproject.org> 5.4.0-0.1.rc
- 5.4.0-rc

* Wed Sep 24 2014 Lubomir Rintel <lkundrak@v3.sk> - 5.4.0-0.alpha1
- Switch from a Git snapshot to a pre-release tarball

* Sun Aug 17 2014 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 5.3.0-0.3.20140723git02c499c
- Rebuilt for https://fedoraproject.org/wiki/Fedora_21_22_Mass_Rebuild

* Thu Jul 24 2014 Lubomir Rintel <lkundrak@v3.sk> - 5.3.0-0.2.20140723git02c499c
- Update

* Sun Jun 08 2014 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 5.3.0-0.2.20140529git98dca3b
- Rebuilt for https://fedoraproject.org/wiki/Fedora_21_Mass_Rebuild

* Tue May 27 2014 Lubomir Rintel <lkundrak@v3.sk> - 5.3.0-0.1.20140529git98dca3b
- Update and rebuild for Qt 5.3

* Fri Feb 14 2014 Lubomir Rintel <lkundrak@v3.sk> - 5.1.0-0.6.20140202git6d038fb
- A more recent snapshot
- Disable xcomposite compositor until it builds

* Sat Jan 04 2014 Lubomir Rintel <lkundrak@v3.sk> - 5.1.0-0.6.20131203git6b20dfe
- Enable QtQuick compositor

* Sat Jan 04 2014 Lubomir Rintel <lkundrak@v3.sk> - 5.1.0-0.5.20131203git6b20dfe
- A newer snapshot

* Mon Nov 25 2013 Lubomir Rintel <lkundrak@v3.sk> - 5.1.0-0.5.20131125git4f5985c
- Rebase to a later snapshot, drop our patches
- Add license texts

* Sat Nov 23 2013 Lubomir Rintel <lkundrak@v3.sk> - 5.1.0-0.5.20131120git8cd1a77
- Rebuild with EGL backend

* Fri Nov 22 2013 Lubomir Rintel <lkundrak@v3.sk> - 5.1.0-0.4.20131120git8cd1a77
- Rebase to a later snapshot, drop 5.2 ABI patch
- Enable nogl backend

* Sun Nov 10 2013 Rex Dieter <rdieter@fedoraproject.org> 5.1.0-0.4.20130826git3b0b90b
- rebuild (arm/qreal)

* Thu Oct 24 2013 Lubomir Rintel <lkundrak@v3.sk> - 5.1.0-0.3.20130826git3b0b90b
- Bulk sad and useless attempt at consistent SPEC file formatting

* Sun Oct 06 2013 Lubomir Rintel <lkundrak@v3.sk> - 5.1.0-0.2.20130826git3b0b90b
- Bump platform plugin ABI to 5.2 for Qt 5.2 aplha

* Wed Sep 11 2013 Lubomir Rintel <lkundrak@v3.sk> - 5.1.0-0.1.20130826git3b0b90b
- Initial packaging
- Adjustments from review (Rex Dieter, #1008529)
