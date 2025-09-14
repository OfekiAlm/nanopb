# Use the main Dockerfile as base
FROM ubuntu:22.04

# Avoid warnings by switching to noninteractive
ENV DEBIAN_FRONTEND=noninteractive

# Install necessary packages for development
RUN apt-get update && apt-get install -y \
    # Base build tools
    build-essential \
    make \
    gcc \
    g++ \
    cmake \
    ninja-build \
    # Python and pip
    python3 \
    python3-pip \
    python3-dev \
    # Protobuf
    protobuf-compiler \
    libprotobuf-dev \
    # Version control
    git \
    # Debugging tools
    gdb \
    valgrind \
    # Network tools
    curl \
    wget \
    # Editor
    vim \
    nano \
    # Shell utilities
    zsh \
    htop \
    tree \
    # Documentation tools
    doxygen \
    graphviz \
    && rm -rf /var/lib/apt/lists/*

# Install Python protobuf libraries and development tools
RUN pip3 install --upgrade pip && \
    pip3 install \
    protobuf \
    grpcio-tools \
    pylint \
    black \
    mypy \
    pytest \
    pytest-cov

# Install oh-my-zsh for better shell experience
RUN sh -c "$(curl -fsSL https://raw.githubusercontent.com/ohmyzsh/ohmyzsh/master/tools/install.sh)" "" --unattended

# Set up zsh as default shell
RUN chsh -s /usr/bin/zsh

# Create a non-root user for development (optional, commented out for now)
# ARG USERNAME=developer
# ARG USER_UID=1000
# ARG USER_GID=$USER_UID
# RUN groupadd --gid $USER_GID $USERNAME \
#     && useradd --uid $USER_UID --gid $USER_GID -m $USERNAME \
#     && apt-get update \
#     && apt-get install -y sudo \
#     && echo $USERNAME ALL=\(root\) NOPASSWD:ALL > /etc/sudoers.d/$USERNAME \
#     && chmod 0440 /etc/sudoers.d/$USERNAME

# Set working directory
WORKDIR /workspace

# Copy the nanopb source code
COPY . /workspace/

# Make the generator executable
RUN chmod +x generator/nanopb_generator

# Set environment variables
ENV PYTHONPATH=/workspace/generator
ENV TERM=xterm-256color

# Switch back to dialog for any ad-hoc use of apt-get
ENV DEBIAN_FRONTEND=dialog

# Default to zsh shell
CMD ["/bin/zsh"]
