#!/bin/bash
if [ -z "$version" ]; then
        echo please export version=7.x
        exit
fi

INSTALL="/opt/droidcam-obs-client"

ls $INSTALL > /dev/null && \
	read -e -p "$INSTALL exists, continue? " choice && \
	[[ "$choice" != [Yy]* ]] && exit 1
#
#
set -ex
touch /opt/.test || sudo chown -R user:user /opt

cmake -S . -B build \
        -G "Unix Makefiles" -DUNIX_STRUCTURE=0 -DDepsPath=`pwd`/../deps \
        -DCMAKE_INSTALL_PREFIX="$INSTALL" -DMIXPANEL_TOKEN=$mixpanel \
        -DDROIDCAM_OVERRIDE=1 -DOBS_VERSION_OVERRIDE=$version -DENABLE_SCRIPTING=False \

LD_LIBRARY_PATH=`pwd`/../deps/lib make -C build -j4
LD_LIBRARY_PATH=`pwd`/../deps/lib make -C build install
find "$INSTALL" -type f -iname *.ini ! -iname locale.ini ! -iname en-us.ini  -delete
rm -rf "${INSTALL}"/{cmake*,lib*,include*}
#
#
cp UI/forms/images/obs.png "${INSTALL}/icon.png"
unzip v4l2loopback-ctl.zip -d /
#unzip droidcam-obs-plugin-prebuilt.zip -d /
find ${INSTALL} -type f -name droidcam-obs.so
find ${INSTALL} -type f -name v4l2loopback-ctl

spec=/tmp/droidcam.spec
sed -e "s/{VERSION}/$version/" droidcam.spec > $spec
rpmbuild -bb $spec
rm $spec
mv ~/rpmbuild/RPMS/*/*.rpm /tmp/
