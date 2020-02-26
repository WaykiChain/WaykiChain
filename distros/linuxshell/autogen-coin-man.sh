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
    echo "  autogen-coin-man [coin|coin-debug|coin-test|coin-ptest]"
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
        Modules="$Modules --with-daemon"
        ;;
        coin-debug)
        Modules="$Modules --with-daemon --enable-debug"
        ;;
        coin-test)
        Modules="$Modules --enable-tests"
        ;;
        coin-ptest)
        Modules="$Modules --enable-ptests"
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
./configure --disable-upnp-default --without-gui --with-incompatible-bdb $Modules
