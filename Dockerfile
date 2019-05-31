FROM ubuntu:latest
MAINTAINER Hisanari Otsu <hi2p.perim@gmail.com>

# -----------------------------------------------------------------------------

# System packages
RUN apt update && apt install -y \
    tmux \
    vim \
    curl \
    git \
    git-lfs \
    software-properties-common \
    build-essential \
    doxygen \
    graphviz

# -----------------------------------------------------------------------------

# Install miniconda & dependencies
WORKDIR /
RUN curl -OJLs https://repo.anaconda.com/miniconda/Miniconda3-latest-Linux-x86_64.sh
RUN bash Miniconda3-latest-Linux-x86_64.sh -p /miniconda -b
ENV PATH=${PATH}:/miniconda/bin
RUN conda update -y conda && conda install -y -c conda-forge -c hi2p-perim \
    python=3.7 \
    cmake \
    ninja \
    jupyter \
    matplotlib \
    pandas \
    sphinx \
    sphinx_rtd_theme \
    jupytext \
    tqdm \
    imageio \
    pytest \
    zeromq \
    cppzmq \
    pybind11 \
    embree3 \
    glm \
    nlohmann_json \
    doctest-cpp \
    fmt \
    stb \
    cereal
RUN pip install breathe sphinx_tabs nbsphinx
RUN imageio_download_bin freeimage

# -----------------------------------------------------------------------------

# Build and install Lighmetrica
COPY . /lightmetrica-v3
WORKDIR /lightmetrica-v3
RUN cmake -G "Ninja" -H. -B_build -DCMAKE_BUILD_TYPE=Release -DLM_BUILD_GUI_EXAMPLES=OFF && \
    cmake --build _build --target install -- -j 4

# Run tests
WORKDIR /lightmetrica-v3/_build/bin
RUN LD_LIBRARY_PATH=. ./lm_test
WORKDIR /lightmetrica-v3
RUN LD_LIBRARY_PATH=./_build/bin PYTHONPATH=./_build/bin python -m pytest ./pytest
