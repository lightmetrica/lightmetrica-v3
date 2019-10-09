FROM ubuntu:latest
MAINTAINER Hisanari Otsu <hi2p.perim@gmail.com>

# Change default shell to bash
SHELL ["/bin/bash", "-c"]

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

# Install miniconda
WORKDIR /
RUN curl -OJLs https://repo.anaconda.com/miniconda/Miniconda3-latest-Linux-x86_64.sh
RUN bash Miniconda3-latest-Linux-x86_64.sh -p /miniconda -b
ENV PATH /miniconda/bin:$PATH

# Setup lm3_dev environment
COPY environment.yml environment.yml
RUN conda env create -f environment.yml
RUN echo "source activate lm3_dev" > ~/.bashrc
ENV PATH /opt/conda/envs/lm3_dev/bin:$PATH

# Install binary dependencies of imageio
RUN source ~/.bashrc && imageio_download_bin freeimage

# Build and install Lighmetrica
COPY . /lightmetrica-v3
WORKDIR /lightmetrica-v3
RUN source ~/.bashrc && \
    cmake -H. -B_build -DCMAKE_BUILD_TYPE=Release -DLM_BUILD_GUI_EXAMPLES=OFF && \
    cmake --build _build --target install -- -j4

# Run tests
WORKDIR /lightmetrica-v3
RUN source ~/.bashrc && \
    python run_tests.py --lmenv .lmenv_docker

# Default command
CMD ["/bin/bash"]