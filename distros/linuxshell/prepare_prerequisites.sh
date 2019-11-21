#!/bin/sh

printf "Check permission ...\\n"
if [ $UID -ne 0 ]; then
    echo "Superuser privileges are required to run this script."
    echo "e.g. \"sudo $0\""
    exit 1
fi

printf "Prepare prerequisites ...\\n"

ARCH=$( uname )
case "$ARCH" in
  "Linux")
    if [ ! -e /etc/os-release ]; then
      printf "\\nWaykiChain Core currently supports Ubuntu, Centos Linux only.\\n"
      printf "Please install on the latest version of one of these Linux distributions.\\n"
      printf "https://www.centos.org/\\n"
      printf "https://www.ubuntu.com/\\n"
      printf "Exiting now.\\n"
      exit 1
    fi

    OS_NAME=$( cat /etc/os-release | grep ^NAME | cut -d'=' -f2 | sed 's/\"//gI' )
    OS_VER=$( grep -i version_id /etc/os-release | cut -d'=' -f2 | sed 's/[^0-9\.]//gi' )
    OS_MAJ=$( echo "${OS_VER}" | cut -d'.' -f1 )
    OS_MIN=$( echo "${OS_VER}" | cut -d'.' -f2 )

    printf "Environment Info, OS_NAME: ${OS_NAME}, OS_VER: ${OS_VER}, OS_MAJ: ${OS_MAJ}, OS_MIN: ${OS_MIN}\\n"

    case "$OS_NAME" in
      "Ubuntu")
        if [ "${OS_MAJ}" -lt 14 ]; then
          printf "You must be running Ubuntu 14.x or higher to install WaykiChain Core.\\n"
          printf "Exiting now.\\n"
          exit 1
        fi

        if [ $OS_MAJ -gt 18 ]; then
          printf "You must be running Ubuntu 18.x or lower to install WaykiChain Core.\\n"
          printf "Exiting now.\\n"
          exit 1
        fi

        printf "Install software-properties-common ...\\n"
        apt-get install software-properties-common -y

        printf "Add bitcoin PPA ...\\n"
        add-apt-repository ppa:bitcoin/bitcoin -y

        printf "Run apt-get update ...\\n"
        apt-get update

        printf "Install all prerequisite libraries/tools ...\\n"
        apt-get install -y build-essential libtool autotools-dev automake pkg-config libssl-dev \
        libevent-dev bsdmainutils python3 libboost-system-dev libboost-filesystem-dev \
        libboost-chrono-dev libboost-program-options-dev libboost-test-dev libboost-thread-dev \
        libdb4.8-dev libdb4.8++-dev git
      ;;
      "CentOS Linux")
        if [ "${OS_MAJ}" -lt 7 ]; then
          printf "You must be running CentOS 7.x to install WaykiChain Core.\\n"
          printf "Exiting now.\\n"
          exit 1
        fi

        printf "Install epel-release ...\\n"
        yum install -y epel-release

        printf "Install all prerequisite libraries/tools ...\\n"
        yum install -y make cmake autoconf automake boost-devel libdb4-cxx libdb4-cxx-devel \
        libevent-devel libtool openssl-devel gcc-c++ git
      ;;
      *)
        printf "\\nUnsupported Linux Distribution. Exiting now.\\n\\n"
        exit 1
    esac
  ;;
  *)
    printf "Unsupported ARCHITECTUR: $EARCH! Exiting now.\\n\\n"
    exit 1
esac

printf "\\n\\nCongratulations! All prerequisites are ready.\\n\\n"
exit 0
