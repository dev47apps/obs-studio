Summary: Use your phone as a webcam
Name: droidcam-client
Version: {VERSION}
Release: 1
License: GPLv2
URL: https://droidcam.app
AutoReqProv: no
Requires: qt5-qtbase, qt5-qtbase-gui, qt5-qtsvg
Requires: speexdsp, libcurl, jansson, libwayland-egl

%define _buildshell /bin/bash
%define _build_id_links none

%description
Use your phone as a webcam.

%post
ldconfig

%prep
version={VERSION}
INSTALL="/opt/droidcam-obs-client"

mkdir -p "$RPM_BUILD_ROOT/opt/"
mv "${INSTALL}" "$RPM_BUILD_ROOT/opt/"

install -D "${OLDPWD}/UI/xdg-data/droidcam.sh" "$RPM_BUILD_ROOT/usr/local/bin/droidcam"
install -D -t"$RPM_BUILD_ROOT/usr/share/applications/" "${OLDPWD}/UI/xdg-data/com.dev47apps.droidcam.desktop"
install -D -t"$RPM_BUILD_ROOT/etc/ld.so.conf.d/"       "${OLDPWD}/UI/xdg-data/com.dev47apps.droidcam.ld.conf"

# cut a tar file
#tar -cjf /tmp/droidcam-client_${version}.tar.bz2 -C "$RPM_BUILD_ROOT" $(ls $RPM_BUILD_ROOT)

# cut the deb file
cp -R "${OLDPWD}/DEBIAN" "$RPM_BUILD_ROOT"
sed -i -e "s/{VER}/$version/" "$RPM_BUILD_ROOT/DEBIAN/control"
dpkg-deb --build --root-owner-group "$RPM_BUILD_ROOT" "/tmp/droidcam_client_${version}_amd64.deb"
rm -rf "$RPM_BUILD_ROOT/DEBIAN"

# Fedora hack (TODO build on Fedora)
mkdir -p "$RPM_BUILD_ROOT/usr/lib64"
ln -sf libbz2.so.1 "$RPM_BUILD_ROOT/usr/lib64/libbz2.so.1.0"

%files
/opt/droidcam-obs-client*
/etc/ld.so.conf.d/com.dev47apps*
/usr/share/applications/com.dev47apps*
/usr/local/bin/droidcam
/usr/lib64/libbz2.so.1.0

%clean
mv $RPM_BUILD_ROOT/opt/* /opt/
rm -rf $RPM_BUILD_ROOT/*
