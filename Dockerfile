FROM ubuntu:latest
MAINTAINER Hisanari Otsu <hi2p.perim@gmail.com>

# -----------------------------------------------------------------------------

RUN apt update && apt install -y \
    git \
    git-lfs \
    software-properties-common \
    build-essential \
    cmake \
    curl \
    ninja-build \
    doctest-dev \
    gdb \
    tmux \
    vim \
    xorg-dev \
    libgl1-mesa-dev \
    alien \
    dpkg-dev \
    debhelper \
    libtbb-dev \
    doxygen \
    graphviz

RUN add-apt-repository ppa:ubuntu-toolchain-r/test
RUN apt update && apt install -y gcc-8 g++-8
RUN update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-8 60 \
                        --slave /usr/bin/g++ g++ /usr/bin/g++-8

# -----------------------------------------------------------------------------

# Embree
WORKDIR /
RUN curl -OJLs https://github.com/embree/embree/releases/download/v3.5.2/embree-3.5.2.x86_64.rpm.tar.gz && \
    tar xvf embree-3.5.2.x86_64.rpm.tar.gz && \
    alien embree3-lib-3.5.2-1.x86_64.rpm && \
    alien embree3-devel-3.5.2-1.noarch.rpm && \
    dpkg -i embree3-lib_3.5.2-2_amd64.deb && \
    dpkg -i embree3-devel_3.5.2-2_all.deb

# -----------------------------------------------------------------------------

# miniconda
WORKDIR /
RUN curl -OJLs https://repo.anaconda.com/miniconda/Miniconda3-latest-Linux-x86_64.sh
RUN bash Miniconda3-latest-Linux-x86_64.sh -p /miniconda -b
ENV PATH=${PATH}:/miniconda/bin
RUN conda update -y conda && conda install -y -c conda-forge \
    jupyter \
    nbconvert \
    jupytext \
    numpy \
    imageio \
    pytest \
    tqdm \
    matplotlib \
    pandas \
    sphinx \
    sphinx_rtd_theme
RUN pip install breathe sphinx_tabs nbsphinx
RUN imageio_download_bin freeimage

# -----------------------------------------------------------------------------

ARG GITHUB_TOKEN

WORKDIR /
RUN git clone --recursive https://${GITHUB_TOKEN}@github.com/hi2p-perim/lightmetrica-v3-external.git external

WORKDIR /external/pybind11
RUN cmake -H. -B_build -DCMAKE_BUILD_TYPE=Release -DPYBIND11_TEST=OFF && \
    cmake --build _build --target install

WORKDIR /external/json
RUN cmake -H. -B_build -DCMAKE_BUILD_TYPE=Release -DJSON_BuildTests=OFF && \
    cmake --build _build --target install

WORKDIR /external/glm
RUN cmake -H. -B_build -DCMAKE_BUILD_TYPE=Release -DGLM_TEST_ENABLE=OFF && \
    cmake --build _build --target install

WORKDIR /external/cereal
RUN cmake -H. -B_build -DCMAKE_BUILD_TYPE=Release -DJUST_INSTALL_CEREAL=ON && \
    cmake --build _build --target install

WORKDIR /external/doctest
RUN cmake -H. -B_build -DCMAKE_BUILD_TYPE=Release -DDOCTEST_WITH_TESTS=OFF && \
    cmake --build _build --target install

WORKDIR /external/fmt
RUN cmake -H. -B_build -DCMAKE_BUILD_TYPE=Release -DFMT_DOC=OFF -DFMT_TEST=OFF && \
    cmake --build _build --target install

WORKDIR /external/libzmq
RUN cmake -H. -B_build -DCMAKE_BUILD_TYPE=Release -DENABLE_DRAFTS=OFF -DWITH_PERF_TOOL=OFF -DBUILD_TESTS=OFF -DENABLE_CPACK=OFF && \
    cmake --build _build --target install

WORKDIR /external/cppzmq
RUN cmake -H. -B_build -DCMAKE_BUILD_TYPE=Release -DENABLE_DRAFTS=OFF -DCPPZMQ_BUILD_TESTS=OFF && \
    cmake --build _build --target install

WORKDIR /external/glfw
RUN cmake -H. -B_build -DCMAKE_BUILD_TYPE=Release -DGLFW_BUILD_DOCS=OFF -DGLFW_BUILD_TESTS=OFF -DGLFW_BUILD_EXAMPLES=OFF && \
    cmake --build _build --target install

# -----------------------------------------------------------------------------

COPY . /lightmetrica-v3
WORKDIR /lightmetrica-v3
RUN ln -s /external/nanort ./plugin/accel_nanort/nanort
RUN ln -s /external/tinyobjloader ./plugin/objloader_tinyobjloader/tinyobjloader
RUN cmake -G "Ninja" -H. -B_build -DCMAKE_BUILD_TYPE=Release && \
    cmake --build _build --target install -- -j 4

WORKDIR /lightmetrica-v3/_build/bin
RUN LD_LIBRARY_PATH=. ./lm_test
RUN python -m pytest --lm . ../../pytest

WORKDIR /lightmetrica-v3/example/ext
RUN cmake -G "Ninja" -H. -B_build -DCMAKE_BUILD_TYPE=Release && \
    cmake --build _build

ENV LD_LIBRARY_PATH=/usr/lib64
ENV PYTHONPATH="/lightmetrica-v3:/lightmetrica-v3/_build/bin"
WORKDIR /lightmetrica-v3
