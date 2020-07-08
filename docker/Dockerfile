FROM phusion/baseimage:18.04-1.0.0
MAINTAINER coredev@waykichainhk.com
ENV SOURCE_BRANCH "release"
ARG debug='-debug'

# Install prrequisite components
RUN echo exit 0 > /usr/sbin/policy-rc.d
RUN add-apt-repository ppa:bitcoin/bitcoin -y && apt-get update
RUN DEBIAN_FRONTEND=noninteractive apt-get install -y --force-yes build-essential libtool autotools-dev automake \
    pkg-config libssl-dev libevent-dev bsdmainutils python3 \
    libboost-system-dev libboost-filesystem-dev libboost-chrono-dev libboost-program-options-dev \
    libboost-test-dev libboost-thread-dev software-properties-common libdb4.8-dev libdb4.8++-dev git-core

# Build WaykiChain source on its release branch
RUN mkdir -p /opt/src && cd /opt/src && git clone --recurse-submodules -b $SOURCE_BRANCH \
    'https://github.com/WaykiChain/WaykiChain.git' && cd WaykiChain \
    && git submodule update --init --recursive \
    && git pull --recurse-submodules \
    && echo 'Build at source commit:' && git log | head -n 3

RUN cd /opt/src/WaykiChain/distros/linuxshell && sh ./linux.sh \
    && cd /opt/src/WaykiChain/ && sh ./autogen-coin-man.sh "coin${debug}" \
    && make && strip /opt/src/WaykiChain/src/coind \
    && mkdir /opt/wicc && mv /opt/src/WaykiChain/src/coind /opt/wicc/ \
    && rm -rf /opt/src

RUN apt-get clean && rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/*

ENV PATH="/opt/wicc:${PATH}"
WORKDIR /opt/wicc/
EXPOSE 6968 8920 18920
CMD ["./coind"]
