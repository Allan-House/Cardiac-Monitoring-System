FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt update && apt install -y \
    build-essential \
    cmake \
    git \
    wget \
    curl \
    gcc-aarch64-linux-gnu \
    g++-aarch64-linux-gnu \
    binutils-aarch64-linux-gnu \
    nano \
    vim \
    ssh \
    pkg-config \
    libpigpio-dev:arm64 \
    libi2c-dev:arm64 \
    && apt clean \
    && rm -rf /var/lib/apt/lists/*

# Cross-compilation environment
ENV CC=aarch64-linux-gnu-gcc
ENV CXX=aarch64-linux-gnu-g++

ENV PKG_CONFIG_PATH=/usr/lib/aarch64-linux-gnu/pkgconfig

WORKDIR /workspaces/Cardiac-Monitoring-System