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
        curl \
    && \
    apt-get clean

RUN pip3 install \
        poetry \
    && \
    rm -rf /root/.cache/pip

RUN curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs -o rustup_install.sh && \
    sh rustup_install.sh -y --profile minimal && \
    rm rustup_install.sh
