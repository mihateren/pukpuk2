FROM gcc:14

RUN apt-get update && apt-get install -y \
    cmake \
    build-essential \
    libzmq3-dev \
    && rm -rf /var/lib/apt/lists/*

# Склонировать cppzmq (header-only)
RUN git clone --depth=1 https://github.com/zeromq/cppzmq /usr/local/include/cppzmq \
    && rm -rf /usr/local/include/cppzmq/.git

WORKDIR /workspace
