FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

RUN useradd -m -s /bin/bash jonas && \
    echo "jonas ALL=(ALL) NOPASSWD:ALL" >> /etc/sudoers

RUN apt update && apt install -y --no-install-recommends \
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
    python3 \
    python3-pip \
    python3-numpy \
    python3-matplotlib \
    python3-tk \
    && apt clean \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /workspaces/Cardiac-Monitoring-System