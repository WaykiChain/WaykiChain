#!/bin/sh
set -e

ShowHelp() {
    echo "\033[40;33m"
    echo "USAGE:"
    echo 
    echo "  autogen-coin-man [MODULE NAME]"
    echo
    echo "EXAMPLE:"
    echo
    echo "  autogen-coin-man [coin|coin-test|coin-ptest]"
    echo "\033[0m"
}

if [ $# = 0 ]; then
    ShowHelp
    exit 1
fi

Modules=""
for Option in $@
do
    case $Option in
        coin)
        Modules=$Modules" --with-daemon"
        ;;
        coin-test)
        Modules=$Modules" --enable-tests"
        ;;
        coin-ptest)
        Modules=$Modules" --enable-ptests"
        ;;
        *)
        echo "\033[40;31mERROR: Unsupported Module Name!\033[0m"

        ShowHelp
        exit 1
        ;;
    esac
done

srcdir="$(dirname $0)"
cd "$srcdir"
autoreconf --install --force

CPPFLAGS="-I/c/deps/boost_1_55_0 \
-I/c/deps/db-4.8.30.NC/build_unix \
-I/c/deps/openssl-1.0.1g/include \
-I/c/deps \
-I/c/deps/libpng-1.6.9 \
-I/c/deps/qrencode-3.4.3 \
-std=c++11 \
" \
CXXFLAGS="-Wall" \
LDFLAGS="-L/c/deps/boost_1_55_0/stage/lib \
-L/c/deps/db-4.8.30.NC/build_unix \
-L/c/deps/openssl-1.0.1g \
-L/c/deps/miniupnpc \
-L/c/deps/protobuf-2.5.0/src/.libs \
-L/c/deps/libpng-1.6.9/.libs \
-L/c/deps/qrencode-3.4.3/.libs" \
./configure \
--disable-upnp-default \
--enable-debug \
--without-gui \
$Modules\
--with-qt-incdir=/c/Qt/5.2.1/include \
--with-qt-libdir=/c/Qt/5.2.1/lib \
--with-qt-bindir=/c/Qt/5.2.1/bin \
--with-qt-plugindir=/c/Qt/5.2.1/plugins \
--with-boost-libdir=/c/deps/boost_1_55_0/stage/lib \
--with-boost-system=mgw48-mt-s-1_55 \
--with-boost-filesystem=mgw48-mt-s-1_55 \
--with-boost-program-options=mgw48-mt-s-1_55 \
--with-boost-thread=mgw48-mt-s-1_55 \
--with-boost-chrono=mgw48-mt-s-1_55
