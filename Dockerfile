FROM docker.io/debian:bookworm-20220801
MAINTAINER Martin Heistermann <martin.heistermann@inf.unibe.ch>

ARG DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y --no-install-recommends \
    binutils \
    ca-certificates \
    ccache \
    clang \
    clang-14 \
    llvm-14 \
    lld-14 \
    cmake \
    curl \
    git \
    libboost-filesystem-dev \
    libboost-system-dev \
    libboost-regex-dev \
    libc-dev \
    libgmm++-dev\
    liblapack-dev \
    libopenblas64-serial-dev \
    libtool \
    locales \
    ninja-build \
    time \
    tzdata \
    wget


RUN mkdir -p /opt && \
    cd /opt && \
    wget -q https://packages.gurobi.com/10.0/gurobi10.0.1_linux64.tar.gz && \
    echo a0b551156df2c94107b3428cae278716a0a6c913f63ac132573852b9725b6c59  gurobi10.0.1_linux64.tar.gz | sha256sum --check && \
    tar xf gurobi10.0.1_linux64.tar.gz && \
    ln -s gurobi1001 gurobi

COPY blossom5-v2.05.src.tar.gz /tmp
COPY . /app

RUN mkdir /app/build && cd /app/build && \
    cmake -G Ninja \
    -D CMAKE_C_COMPILER=clang-14 \
    -D CMAKE_CXX_COMPILER=clang++-14 \
    -D CMAKE_CXX_COMPILER_AR=$(which llvm-ar-14) \
    -D CMAKE_CXX_COMPILER_RANLIB=$(which llvm-ranlib-14) \
    -D CMAKE_BUILD_TYPE=Release \
    -D CMAKE_CXX_FLAGS="-march=native" \
    -D CMAKE_INTERPROCEDURAL_OPTIMIZATION=ON \
    .. && ninja


FROM docker.io/debian:bookworm-20220801-slim
COPY --from=0 /app/build/Build /app/build/Build
COPY --from=0 /opt/gurobi/linux64/lib/libgurobi100.so /opt/gurobi/linux64/lib/
COPY --from=0 /opt/gurobi/linux64/bin/grbgetkey /usr/local/bin

RUN apt-get update && apt-get install -y --no-install-recommends \
      libboost-regex1.74.0 \
      ca-certificates \
      libboost-system1.74.0 \
      libboost-filesystem1.74.0 \
      libopenmpi3 \
      liblapack3 \
      libmumps-5.5 \
      libopenblas64-0-serial \
      time


# todo: is mumps, blas, lapack actually used? can we go without?
#       or just copy the .so's? 160MB from this apt install

RUN ln -s /app/build/Build/bin/* /usr/local/bin
RUN apt-get clean && rm -rf /var/lib/apt/lists/

