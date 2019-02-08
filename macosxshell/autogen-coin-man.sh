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

CPPFLAGS="-stdlib=libc++ -std=c++11 -I/usr/local/opt/boost160/include -I/usr/local/BerkeleyDB.4.8/include" \
LDFLAGS="-L/usr/local/opt/boost160/lib -L/usr/local/BerkeleyDB.4.8/lib -lc++" \
./configure \
--disable-upnp-default \
--enable-debug \
--without-gui \
$Modules\
--with-boost-libdir=/usr/local/opt/boost160/lib \
--with-incompatible-bdb
