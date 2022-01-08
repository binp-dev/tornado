FROM debian:bullseye

RUN dpkg --add-architecture i386 && \
    apt-get update && \
    apt-get -y install \
        lib32z1 \
        git \
        g++ \
        cmake \
        make \
        python3 \
        python3-pip \
        perl \
        tar \
        gzip \
        libgtest-dev \
        libzmq3-dev \
    && \
    apt-get clean

RUN pip3 install \
    conan \
    zmq
