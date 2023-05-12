# WITH_GUROBI must be 0 or 1
FROM docker.io/debian:bookworm-20230502 as debian-base
MAINTAINER Martin Heistermann <martin.heistermann@inf.unibe.ch>

FROM debian-base as base-packages

ARG DEBIAN_FRONTEND=noninteractive
RUN apt-get update && apt-get install -y --no-install-recommends \
    binutils \
    build-essential \
    ca-certificates \
    ccache \
    clang \
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

ARG WITH_GUROBI=0

RUN if [ $WITH_GUROBI = 1 ]; then \
            echo "Downloading Gurobi..." \
            mkdir -p /opt && \
            cd /opt && \
            wget -q https://packages.gurobi.com/10.0/gurobi10.0.1_linux64.tar.gz && \
            echo a0b551156df2c94107b3428cae278716a0a6c913f63ac132573852b9725b6c59  gurobi10.0.1_linux64.tar.gz | sha256sum --check && \
            tar xf gurobi10.0.1_linux64.tar.gz && \
            ln -s gurobi1001 gurobi; \
        elif [ $WITH_GUROBI = 0 ]; then \
            echo "Building without Gurobi."; \
        else \
            echo "\n\nDocker build-arg WITH_GUROBI must be 0 or 1\n\n"; \
            exit 1; \
        fi


FROM base-packages AS build

COPY . /app

ARG WITH_BLOSSOM5=0

RUN mkdir /app/build && cd /app/build && \
    cmake -G Ninja \
    -D QUADRETOPOLOGY_WITH_GUROBI=${WITH_GUROBI} \
    -D SATSUMA_ENABLE_BLOSSOM5=${WITH_BLOSSOM5} \
    -D CMAKE_BUILD_TYPE=Release \
    -D CMAKE_CXX_FLAGS="-march=native" \
    -D CMAKE_INTERPROCEDURAL_OPTIMIZATION=ON \
    .. && ninja

RUN ln -s /app/build/Build/bin/* /usr/local/bin
RUN apt-get clean && rm -rf /var/lib/apt/lists/

