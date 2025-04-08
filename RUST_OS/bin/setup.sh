#!/bin/bash

set -e

apt update && apt install -y git
git config --global --add safe.directory "*"

## NOTE: Only change the following two lines
git config --global user.name "Your Name"
git config --global user.email "youremail@yourdomain.com"

TOP=$(git rev-parse --show-toplevel)
BIN=$TOP/bin
DEP=$TOP/.dep
VER=nightly-2025-01-28
PROJ_PKG=(build-essential
     python3
     socat
     wget
     curl
     tar
     screen
     clang
     lld
     linux-image-extra-virtual)
QEMU_DEP=(libglib2.0-dev libpixman-1-dev zlib1g-dev)

# install pkgs
echo "[!] Installing packages"
export DEBIAN_FRONTEND=noninteractive 
export TZ=Etc/UTC
apt install -y ${PROJ_PKG[*]}
apt install -y ${QEMU_DEP[*]}

# install rustup
if ! [ -x "$(command -v rustup)" ]; then
    echo "[!] Installing rustup"

    curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh

    export PATH=$HOME/.cargo/bin:$PATH
fi

rustup default $VER
rustup component add rust-src llvm-tools-preview clippy

echo 'PATH="$HOME/.cargo/bin/:$PATH"' >>~/.bashrc

echo "[!] Setup complete"
echo "[!] Please run 'source ~/.bashrc' to update your environment"
