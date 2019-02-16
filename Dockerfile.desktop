FROM dorowu/ubuntu-desktop-lxde-vnc
MAINTAINER Hisanari Otsu <hi2p.perim@gmail.com>

ENV BUILD_CORES -j

# -----------------------------------------------------------------------------

# Install some packages
RUN apt update && apt install -y \
    git \
    git-lfs \
    software-properties-common \
    build-essential \
    cmake \
    curl \
    ninja-build \
    python3-dev \
    python3-distutils \
    python3-pip \
    python3-numpy \
    doctest-dev \
    gdb \
    tmux \
    vim \
    xorg-dev \
    libgl1-mesa-dev

RUN pip3 install --upgrade pip
RUN pip install pytest tqdm
RUN pip install imageio && imageio_download_bin freeimage

RUN add-apt-repository ppa:ubuntu-toolchain-r/test
RUN apt update && apt install -y gcc-8 g++-8
RUN update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-8 60 \
                        --slave /usr/bin/g++ g++ /usr/bin/g++-8

# -----------------------------------------------------------------------------

WORKDIR /root
