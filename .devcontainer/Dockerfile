FROM ubuntu:22.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt update && apt install -y \
    build-essential \
    make \
    cmake \
    git \
    wget \
    curl \
    gcc-aarch64-linux-gnu \
    g++-aarch64-linux-gnu \
    ssh \
    libpigpio-dev \
    libi2c-dev \
    && apt clean

ENV CC=gcc-aarch64-linux-gnu
ENV CXX=g++-aarch64-linux-gnu
ENV AR=aarch64-linux-gnu-ar

WORKDIR /workspaces/Cardiac-Monitoring-System