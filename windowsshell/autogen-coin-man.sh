#!/bin/sh
set -e

if [ $# == 0 ]; then	
  	echo -e "\033[40;33m"
		echo warming  your had not inputed assemble model
		echo "$PWD"
		echo autogen-coin-man [MODEL NAME]
		echo
		echo	EXAMPLE:
		echo
		echo	autogen-coin-man ["coin|coin-test|coin-ptest"]
	echo -e "\033[40;37m"
		exit 1
elif [ $# == 1 ]; then
	case $1 in 
		coin)
		flag1=--with-daemon
		;;
		coin-test)
		flag1=--enable-tests
		;;
		coin-ptest)
		flag1=--enable-ptests
		;;
		*)
		echo -e "\033[40;32m"
		echo warming:error para!
		echo -e "\033[40;37m"
		exit 1
		;;
	esac
elif [ $# == 2 ]; then
	case $1 in 
		coin)
		flag1=--with-daemon
		;;
		coin-test)
		flag1=--enable-tests
		;;
		coin-ptest)
		flag1=--enable-ptests
		;;
		*)
		echo -e "\033[40;32m"
		echo warming:error para!
		echo -e "\033[40;37m"
		exit 1
		;;
	esac
	case $2 in 
		coin)
		flag2=--with-daemon
		;;
		coin-test)
		flag2=--enable-tests
		;;
		coin-ptest)
		flag2=--enable-ptests
		;;
		*)
		echo -e "\033[40;32m"
		echo warming:error para!
		echo -e "\033[40;37m"
		exit 1
		;;
	esac
elif [ $# == 3 ]; then
	case $1 in 
		coin)
		flag1=--with-daemon
		;;
		coin-test)
		flag1=--enable-tests
		;;
		coin-ptest)
		flag1=--enable-ptests
		;;
		*)
		echo -e "\033[40;32m"
		echo warming:error para!
		echo -e "\033[40;37m"
		exit 1
		;;
	esac
	case $2 in 
		coin)
		flag2=--with-daemon
		;;
		coin-test)
		flag2=--enable-tests
		;;
		coin-ptest)
		flag2=--enable-ptests
		;;
		*)
		echo -e "\033[40;32m"
		echo warming:error para!
		echo -e "\033[40;37m"
		exit 1
		;;
	esac
	case $3 in 
		coin)
		flag3=--with-daemon
		;;
		coin-test)
		flag3=--enable-tests
		;;
		coin-ptest)
		flag3=--enable-ptests
		;;
		*)
		echo -e "\033[40;32m"
		echo warming:error para!
		echo -e "\033[40;37m"
		exit 1
		;;
	esac
else
	echo -e "\033[40;32m"
	echo warming  your had inputed illegal params
   	echo please insure the params in [coin|coin-test|coin-ptest]
	echo -e "\033[40;37m" 
	exit 1
fi

srcdir="$(dirname $0)"
cd "$srcdir"
autoreconf --install --force

CPPFLAGS="-I/c/deps/boost_1_55_0 \
-I/c/deps/db-4.8.30.NC/build_unix \
-I/c/deps/openssl-1.0.1g/include \
-I/c/deps \
-I/c/deps/protobuf-2.5.0/src \
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
$flag1 \
$flag2 \
$flag3 \
$flag4 \
--with-qt-incdir=/c/Qt/5.2.1/include \
--with-qt-libdir=/c/Qt/5.2.1/lib \
--with-qt-bindir=/c/Qt/5.2.1/bin \
--with-qt-plugindir=/c/Qt/5.2.1/plugins \
--with-boost-libdir=/c/deps/boost_1_55_0/stage/lib \
--with-boost-system=mgw48-mt-s-1_55 \
--with-boost-filesystem=mgw48-mt-s-1_55 \
--with-boost-program-options=mgw48-mt-s-1_55 \
--with-boost-thread=mgw48-mt-s-1_55 \
--with-boost-chrono=mgw48-mt-s-1_55 \
--with-protoc-bindir=/c/deps/protobuf-2.5.0/src